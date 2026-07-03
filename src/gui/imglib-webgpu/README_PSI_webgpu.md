# imglib-webgpu — sack image library driver targeting WebGPU/Dawn

A `sack.image` family driver that records PSI / sack imglib drawing operations
into `WGPURenderBundle`s, executed inside the surface's render pass per frame.
Lives alongside the WebGPU/Dawn binding in `sack-gui`, registers under
**`webgpu.image`** / **`webgpu.image.3d`** / **`webgpu.render.3d`**, and is
selected per-renderer by setting an ImageObject's `pii` override (see
**Wire-up** below). CPU `sack.image` continues to work unchanged for any
renderer that hasn't asked for a webgpu context.

The retained-bundle model means PSI controls or JS draw code that doesn't
mutate between frames costs **nothing** on the CPU side once recorded — only
the GPU replays. Static HUD content, font glyphs, frames, decorations, etc.
all settle into bundles that survive across thousands of frames.

## What's in the box

| File | Purpose |
|---|---|
| [`local.h`](local.h) | Driver-state struct, per-image struct, internal API |
| [`driver.cc`](driver.cc) | Lazy init, per-image side-table (PTREEROOT), frame-walker, surface-root tracking, `IMAGE_LIBRARY_SOURCE_MAIN` global |
| [`driver_pipelines.cc`](driver_pipelines.cc) | WGSL shader module + 4 `WGPURenderPipeline`s + bind-group layouts + samplers + globals/lighting/palette uniforms + setters for surface format, depth format, surface size, light state, normal-map binding + source texture upload + bind-group cache |
| [`bundle.cc`](bundle.cc) | `webgpu_op_list`: per-pipeline vert pools + batch list. Append API (`solid_quad` / `solid_tristrip` / `textured_quad` / `multi_quad` / `normal_quad`) and `webgpu_op_list_record` which encodes into a `WGPURenderBundle`. |
| [`interface_image.cc`](interface_image.cc) | Registers `webgpu.image`. Memcpy-then-override of the CPU sack.image interface table. Exports `sack_image_get_webgpu_reverse_for_render` so the renderer can attach this driver as the surface image's `reverse_interface`. |
| [`image_ops.cc`](image_ops.cc) | IMAGE_INTERFACE overrides: `BlatColor*`, `BlotImage*`, `BlotImageSizedEx`, `BlotScaledImageSizedEx`, `ReloadTexture*`, `MarkImageDirty`, `UnmakeImageFileEx`, `ResetImageBuffers`, `IsImageTargetFinal`. Hot blit path (`blot_record`) walks subimage→atlas-root, recomputes UVs in atlas-relative coords, handles `IF_FLAG_INVERTED`. |
| [`line_ops.cc`](line_ops.cc) | `do_line` (perpendicular-offset tristrip), `do_hline`/`do_vline` (axis-aligned quads), alpha variants. |
| [`interface_image_3d.cc`](interface_image_3d.cc), [`image_3d_ops.cc`](image_3d_ops.cc) | Registers `webgpu.image.3d`. Shader compile entries take **WGSL source strings** (not GLSL); callers porting from puregl2 re-author their shaders. |
| [`interface_render_3d.cc`](interface_render_3d.cc), [`render3d_ops.cc`](render3d_ops.cc) | Registers `webgpu.render.3d`. Stub utility functions (GetRenderTransform / ClipPoints / GetViewVolume / SetRendererAnchorSpace / GetRenderPipe / AddShader). Camera state is owned by the renderer/binding, not by this driver. |
| [`shaders.wgsl`](shaders.wgsl) | Reference WGSL — must stay in sync with the raw-string copy in `driver_pipelines.cc::kImglibWGSL`. |

## Wire-up from JS

```js
import sack from 'sack-gui';

const r = new sack.Renderer({ width: 1280, height: 720, flags: ATTRIB_NO_REDIRECT });

const adapter = await sack.gpu.requestAdapter();
const device  = await adapter.requestDevice();

const ctx = r.getContext('webgpu');           // flips display image to FINAL_RENDER,
                                              // installs reverse_interface = us
ctx.configure({
  device,
  format:    'bgra8unorm',                    // BGRA on Windows, RGBA elsewhere
  alphaMode: 'premultiplied',                 // needed for layered windows
});

// Optional: if you'll share the surface pass with depth-attached content
// (e.g. three.js 3D), tell us the depth format so our render bundles
// record with matching attachment state.
ctx.setSurfaceDepthFormat('depth24plus');

r.on('draw', (surface) => {
  surface.fill(0xFF202020);                   // ARGB; 0 = transparent (no-op)
  surface.text("Hello", 50, 50, 0xFFFFFFFF);  // routes through font cache → atlas
  surface.line(0, 0, 800, 600, 0xFF80C0FF);
  surface.drawImage(image, dx, dy, dw, dh, sx, sy, sw, sh);

  // … standard WebGPU encode happens here. pass.end() on a surface-targeting
  // pass triggers our frame walker — bundles re-record where dirty, then
  // executeBundles into the same pass.
});
```

## Architecture notes

### Per-renderer interface binding
ImageObject carries an optional `pii` (per-instance image interface override). On
a renderer with a webgpu context, the surface ImageObject is built with
`pii = render_global.pri_gpu_image`. JS-side draws on that ImageObject route
through us; ImageObjects on plain CPU renderers continue using `g.pii`
(the global default). Both worlds coexist; nothing forces either.

### Surface root + frame walker
Each render pass that the binding sees targeting the active surface triggers
`webgpu_image_dispatch_surface_pass(pass)`. The walker:

1. Starts from `g_surface_root` (set during `getContext('webgpu')`), neutralising
   the root's own `real_x/real_y` so the root viewport always lands at `(0,0)`
   of the surface regardless of where the OS moves the window.
2. Recurses through `pChild → pElder`, accumulating `(abs_x, abs_y)`.
3. For each `IF_FLAG_FINAL_RENDER` non-`IF_FLAG_IN_MEMORY` subimage:
   - Re-records the bundle if `dirty` OR generation counters
     (`depth_format_generation_`, `font_atlas_generation_`,
     `palette_generation_`) have moved.
   - Sets viewport to the subimage's absolute rect.
   - `wgpuRenderPassEncoderExecuteBundles(pass, 1, &pi->bundle_)`.

### Bundle attachment state
Bundles must be encoded with attachment state matching the executing pass.
Driver tracks both `surface_format_` (color) and `surface_depth_format_` (depth).
On a pure 2D HUD pass, `surface_depth_format_ == Undefined` → bundles record
without depth. On a shared pass with three.js's depth attachment,
`setSurfaceDepthFormat('depth24plus')` triggers:

1. Bundle encoder gets `depthStencilFormat = Depth24Plus`, `depthReadOnly = true`.
2. Pipelines are rebuilt with a matching `WGPUDepthStencilState`
   (`depthWriteEnabled = False`, `depthCompare = Always`, no stencil).
3. `depth_format_generation_` bumps so any pre-existing per-image bundles
   re-record on the next walk.

### Reverse-interface for the font path
sack's font system (`fntcache.c` / `fntrender.c` / `font.c`) already does all
the heavy lifting: per-font atlas image, free-region allocator, glyph
rasterisation via FreeType. The driver-specific seam is the *terminal*
`BlotImageSizedEx` of the rendered glyph cell onto the FINAL_RENDER surface.

`getContext('webgpu')` sets the display image's `reverse_interface` to our
IMAGE_INTERFACE pointer. When sack's `PutCharacterFontX` runs and sees
`reverse_interface` non-NULL, it calls
`reverse_interface->_BlotImageSizedEx(cell, …)` — which is **our**
`wgpu_BlotImageSizedEx`. We treat the cell as a subimage source (atlas root
walk, atlas-relative UV math, atlas texture shared across all glyphs of that
font).

Atlas allocation and glyph rasterisation still go through sack's existing
code paths (delegated to CPU since our `MakeImageFileEx` /  `MakeSubImageEx` /
`PutCharacterFont` overrides pass through to `sack.image`).

### Subimage texture sharing
Any subimage source draw (font glyph, sub-region blit) walks `pParent` to the
topmost root. The root's `webgpu_per_image` entry holds the WGPU texture +
diffuse bind group, shared by all subimages. Source coords in
`wgpu_BlotImageSizedEx` are translated to atlas-relative before UV
normalisation. Batches are keyed by root, so different glyphs of the same
font merge into a single draw.

### Premultiplied alpha
Pipelines use `One, OneMinusSrcAlpha` blend for both color and alpha; fragment
shaders multiply `rgb` by `alpha` before output. Pass `alphaMode: 'premultiplied'`
to `ctx.configure(...)` and the desktop compositor blends the surface
correctly through `WS_EX_NOREDIRECTIONBITMAP` layered windows.

`resolve_color` in WGSL is now pure literal RGBA: `0x00000000` is fully
transparent (no-op blit under premultiplied blend), `0xFF000000` is opaque
black, `0xFFFFFFFF` is opaque white. Sack CDATA semantics preserved.

### Reset semantics
`ResetImageBuffers` (called by `ImageObject::reset`) releases the cached
bundle, clears the op-list, sets dirty. Next walk re-records → empty ops
list → record returns NULL → bundle stays NULL → executeBundles is
skipped → the framebuffer contains only the pass's `clearValue`. Want a
visible "blank" surface after reset? Set the pass's `clearValue.a > 0`.

## Driver state cheat sheet

```c
struct webgpu_image_driver_state {
    PIMAGE_INTERFACE     cpu_image_interface;     // delegate target for non-FINAL_RENDER paths
    PIMAGE_3D_INTERFACE  cpu_image_3d_interface;
    WGPUDevice           device_;
    WGPUQueue            queue_;
    WGPURenderPipeline   pipe_solid_, pipe_textured_, pipe_multi_, pipe_normal_;
    WGPUBindGroupLayout  bgl_globals_, bgl_texture_, bgl_texture_normal_;
    WGPUSampler          sampler_linear_, sampler_nearest_;
    WGPUBuffer           palette_buffer_, lighting_buffer_;
    WGPUBindGroup        globals_group_;
    WGPUTextureFormat    surface_format_;           // e.g. BGRA8Unorm
    WGPUTextureFormat    surface_depth_format_;     // Undefined or Depth24Plus
    uint32_t             surface_w_, surface_h_;
    uint32_t             palette_generation_, font_atlas_generation_, depth_format_generation_;
    RCOORD               scale;
};
```

Per-image state lives in a `PTREEROOT` keyed by `Image*` (side-table — see
`webgpu_per_image_get` in `driver.cc`). The struct holds the GPU texture/view,
diffuse + normal bind groups, the retained bundle, the op-list, a spinlock,
a dirty flag, and per-bundle generation snapshots.

## Setters callable from C / JS

| C / native | JS (`GPUCanvasContext`) | Effect |
|---|---|---|
| `webgpu_image_set_surface_size(w, h)` | implicit via `ctx.configure({width, height})` | NDC denominator for px→NDC |
| `webgpu_image_set_surface_format(fmt)` | implicit via `ctx.configure({format})` | Bundle color attachment format |
| `webgpu_image_set_surface_depth_format(fmt)` | `ctx.setSurfaceDepthFormat('depth24plus')` | Bundle/pipeline depth attachment + invalidate-on-change |
| `webgpu_image_set_surface_root(img)` | implicit on `r.getContext('webgpu')` | PSI image tree root for the walker |
| `webgpu_image_set_light_direction(x, y, z)` | (no JS binding yet) | Lighting uniform for normal pipeline |
| `webgpu_image_set_light_color(r, g, b)` | (no JS binding yet) | ditto |
| `webgpu_image_set_ambient(r, g, b, intensity)` | (no JS binding yet) | ditto |
| `webgpu_image_set_palette_entry(idx, r, g, b, a)` | (no JS binding yet) | Palette uniform (currently unused by shaders) |
| `webgpu_image_set_normal_map(diff, normal)` | (no JS binding yet) | Bind normal-map per-source-image |

## Known limitations / TODO

- **Atlas-growth bundle invalidation**. When sack's `UpdateRendererImage`
  doubles a font atlas, UVs of existing characters re-fractionalize. The
  walker should compare per-image `bundle_atlas_generation_` against a
  bumped `font_atlas_generation_`. The mechanism is wired; the bump on
  growth is the remaining hook.
- **Text-only opaque mode**. With `IF_FLAG_HAS_PUTSTRING` set on the dest,
  font code skips background blits — that path isn't exercised yet.
- **Phase-2 world-space sub-surfaces**. Today the walker treats every
  FINAL_RENDER subimage as a 2D HUD with viewport-positioning. For
  3D-placed sub-surfaces (image objects in a 3D scene that need camera
  transforms applied), the vertex shader needs a per-subimage `model_view_proj`
  uniform instead of bake-as-NDC at record time. Pipelines and bundle
  encoding don't need to change; the recording shape does.
- **Off-screen clipping**. `blot_record` doesn't AABB-clip against the
  surface rect, so off-screen draws still emit triangles that get
  hardware-clipped per fragment. Cheap CPU clip would save GPU work for
  PSI controls that draw large off-screen prep regions.
- **Palette feature**. Buffer + bind group still allocated but unreferenced
  by shaders after the `0` ↔ palette[0] ambiguity fix. Either drop the
  binding entirely (minor pipeline-layout cleanup) or wire a high-bit
  palette-index escape into `resolve_color`.
- **WGSL source duplication**. `shaders.wgsl` and the embedded string in
  `driver_pipelines.cc` must be kept in sync by hand. A build-time
  generator that emits an `.inc` file is the proper fix.
- **JS bindings for setter family** (light direction, palette entry,
  normal map). Add to `GPUCanvasContext` next to `setSurfaceDepthFormat`
  when first needed.

## Dependencies

- Dawn (`webgpu_dawn` shared lib), Vulkan backend only (configured in
  [`CMakeLists.dawn.txt`](../../../CMakeLists.dawn.txt) with
  `-DDAWN_ENABLE_VULKAN=ON` and everything else off).
- sack imglib (`sack.image` / `sack.image++` for the CPU fallback the
  driver memcpys + overrides on top of).
- sack font cache (`fntcache.c` / `fntrender.c` / `font.c` — used unmodified
  via `reverse_interface` for the text path).
