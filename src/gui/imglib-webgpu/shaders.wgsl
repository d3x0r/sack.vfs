// sack-gui imglib-webgpu — internal pipeline shaders.
//
// Four pipelines:
//   solid     : pos + per-vertex color                    — lines, rects, BlatColor*
//   textured  : pos + uv + per-vertex tint                — BlotImage COPY / SHADED
//   multi     : pos + uv + 3× per-vertex shade channels   — BlotImage MULTISHADE
//   normal    : pos + uv + tint, normal-mapped + lit      — bordered/framed images
//
// All per-draw uniforms are promoted to per-vertex attributes so render
// bundles can batch indefinitely without break-on-state-change.
//
// Shared bind groups:
//   @group(0) binding(0) : palette uniform   (16 × vec4<f32>, RGBA 0..1)
//   @group(0) binding(1) : lighting uniform  (used by normal pipeline)
//   @group(1) binding(0) : sampler           (textured pipelines)
//   @group(1) binding(1) : diffuse texture   (textured pipelines)
//   @group(1) binding(2) : normal  texture   (normal pipeline only — distinct
//                                              bind-group-layout from textured/multi)
//
// Color encoding in vertices (u32 packed RGBA):
//   - high byte (alpha) == 0  →  low 4 bits index palette, rest reserved.
//   - high byte (alpha) != 0  →  literal RGBA, byte values as-is.
//   resolve_color() centralises the rule so changes happen in one place.

struct Palette {
    colors : array<vec4<f32>, 16>,
};
@group(0) @binding(0) var<uniform> palette : Palette;

// Lighting uniform — used by the normal pipeline. Other pipelines never bind
// the slot but the layout includes it so all four pipelines share one
// pipeline-layout slot 0. Setter API rewrites this buffer; no bundle
// invalidation needed.
struct Lighting {
    light_dir         : vec4<f32>,   // xyz = direction (toward light), w unused
    light_color       : vec4<f32>,   // rgb = colour, a = unused
    ambient_color     : vec4<f32>,   // rgb = ambient, a = intensity scalar
};
@group(0) @binding(1) var<uniform> lighting : Lighting;

fn resolve_color(packed : u32) -> vec4<f32> {
    // alpha != 0 → literal RGBA.
    // alpha == 0 → palette index in low 4 bits. Slot 0 is reserved as
    // fully transparent so CDATA(0) = transparent under the premultiplied
    // blend (a no-op blit, matching "fill(0)" semantics).
    let a : u32 = (packed >> 24u) & 0xFFu;
    if (a == 0u) {
        let idx : u32 = packed & 0x0Fu;
        return palette.colors[idx];
    }
    let r = f32((packed       ) & 0xFFu) / 255.0;
    let g = f32((packed >>  8u) & 0xFFu) / 255.0;
    let b = f32((packed >> 16u) & 0xFFu) / 255.0;
    let af = f32(a) / 255.0;
    return vec4<f32>(r, g, b, af);
}

// ====================================================================
// Pipeline 1 : SOLID — pos + per-vertex color
// ====================================================================

struct VsSolidIn {
    @location(0) pos   : vec2<f32>,
    @location(1) color : u32,
};
struct VsSolidOut {
    @builtin(position) clip_pos : vec4<f32>,
    @location(0)       color    : vec4<f32>,
};

@vertex fn vs_solid(in : VsSolidIn) -> VsSolidOut {
    var out : VsSolidOut;
    out.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    out.color    = resolve_color(in.color);
    return out;
}

@fragment fn fs_solid(in : VsSolidOut) -> @location(0) vec4<f32> {
    return in.color;
}

// ====================================================================
// Pipeline 2 : TEXTURED + TINT — pos + uv + per-vertex tint
// (tint == 0xFFFFFFFF means "no shading" — multiplied passthrough)
// ====================================================================

@group(1) @binding(0) var samp : sampler;
@group(1) @binding(1) var tex  : texture_2d<f32>;

struct VsTexIn {
    @location(0) pos  : vec2<f32>,
    @location(1) uv   : vec2<f32>,
    @location(2) tint : u32,
};
struct VsTexOut {
    @builtin(position) clip_pos : vec4<f32>,
    @location(0)       uv       : vec2<f32>,
    @location(1)       tint     : vec4<f32>,
};

@vertex fn vs_textured(in : VsTexIn) -> VsTexOut {
    var out : VsTexOut;
    out.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    out.uv       = in.uv;
    out.tint     = resolve_color(in.tint);
    return out;
}

@fragment fn fs_textured(in : VsTexOut) -> @location(0) vec4<f32> {
    let texel = textureSample(tex, samp, in.uv);
    return texel * in.tint;
}

// ====================================================================
// Pipeline 3 : TEXTURED + MULTISHADE
// Three per-channel shade colors. Each colour channel of the sampled
// texel multiplies into the matching shade colour.
// ====================================================================

struct VsMultiIn {
    @location(0) pos    : vec2<f32>,
    @location(1) uv     : vec2<f32>,
    @location(2) shadeR : u32,
    @location(3) shadeG : u32,
    @location(4) shadeB : u32,
};
struct VsMultiOut {
    @builtin(position) clip_pos : vec4<f32>,
    @location(0)       uv       : vec2<f32>,
    @location(1)       shadeR   : vec4<f32>,
    @location(2)       shadeG   : vec4<f32>,
    @location(3)       shadeB   : vec4<f32>,
};

@vertex fn vs_multi(in : VsMultiIn) -> VsMultiOut {
    var out : VsMultiOut;
    out.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    out.uv       = in.uv;
    out.shadeR   = resolve_color(in.shadeR);
    out.shadeG   = resolve_color(in.shadeG);
    out.shadeB   = resolve_color(in.shadeB);
    return out;
}

@fragment fn fs_multi(in : VsMultiOut) -> @location(0) vec4<f32> {
    let texel = textureSample(tex, samp, in.uv);
    let lit_r = texel.r * in.shadeR.rgb;
    let lit_g = texel.g * in.shadeG.rgb;
    let lit_b = texel.b * in.shadeB.rgb;
    return vec4<f32>(lit_r + lit_g + lit_b, texel.a);
}

// ====================================================================
// Pipeline 4 : NORMAL-MAPPED + TINT
// Diffuse texture + tangent-space normal map. Single directional light
// (lighting.light_dir) plus ambient. Tangent space for screen-aligned
// quads is taken as world = (+X, +Y, +Z), so the normal sampled from the
// map is treated as a world-space direction directly (after the standard
// rgb*2-1 unpack). Good enough for 2D UI borders; for 3D content with
// rotation/perspective the vertex layout would need TBN per-vertex.
// ====================================================================

@group(1) @binding(2) var tex_normal : texture_2d<f32>;

struct VsNormalOut {
    @builtin(position) clip_pos : vec4<f32>,
    @location(0)       uv       : vec2<f32>,
    @location(1)       tint     : vec4<f32>,
};

@vertex fn vs_normal(in : VsTexIn) -> VsNormalOut {
    var out : VsNormalOut;
    out.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    out.uv       = in.uv;
    out.tint     = resolve_color(in.tint);
    return out;
}

@fragment fn fs_normal(in : VsNormalOut) -> @location(0) vec4<f32> {
    let diffuse  = textureSample(tex, samp, in.uv);
    let n_raw    = textureSample(tex_normal, samp, in.uv).rgb;
    let normal   = normalize(n_raw * 2.0 - vec3<f32>(1.0));
    let l_dir    = normalize(lighting.light_dir.xyz);
    let lambert  = max(dot(normal, l_dir), 0.0);
    let ambient  = lighting.ambient_color.rgb * lighting.ambient_color.a;
    let direct   = lighting.light_color.rgb * lambert;
    let lit_rgb  = diffuse.rgb * (ambient + direct);
    return vec4<f32>(lit_rgb, diffuse.a) * in.tint;
}
