// sack-gui imglib-webgpu — real pipeline / bind-group / buffer construction.
//
// All lazy: created on first call to webgpu_image_get_pipe_*, returns NULL
// until a WGPUDevice is available. Once built, the objects live for the
// driver's lifetime. Pipelines aren't cached by format — surface format is
// a platform constant (BGRA on Windows, RGBA elsewhere) per discussion.
//
// The WGSL source for all four pipelines lives in shaders.wgsl and is
// embedded here as a raw string literal so we don't need build-time tooling.

#include "local.h"

// PSI's GetBaseColor — populates palette slots 1..14 from the running
// PSI theme. Slot 0 stays reserved as transparent. Mapping mirrors
// ControlColorTypes from sack/include/controls.h:
//
//   palette[1]  HIGHLIGHT                (PSI 0)
//   palette[2]  NORMAL                   (PSI 1)
//   palette[3]  SHADE                    (PSI 2)
//   palette[4]  SHADOW                   (PSI 3)
//   palette[5]  TEXTCOLOR                (PSI 4)
//   palette[6]  CAPTION                  (PSI 5)
//   palette[7]  CAPTIONTEXTCOLOR         (PSI 6)
//   palette[8]  INACTIVECAPTION          (PSI 7)
//   palette[9]  INACTIVECAPTIONTEXTCOLOR (PSI 8)
//   palette[10] SELECT_BACK              (PSI 9)
//   palette[11] SELECT_TEXT              (PSI 10)
//   palette[12] EDIT_BACKGROUND          (PSI 11)
//   palette[13] EDIT_TEXT                (PSI 12)
//   palette[14] SCROLLBAR_BACK           (PSI 13)
//   palette[15] unused (magenta sentinel)
//
// CDATA encoding for "use palette slot N":  alpha == 0, low 4 bits == N.
// So `0x00000002` from JS = palette[2] = NORMAL background. Convenient
// for theme swap — SetBaseColor + a single re-upload of the palette
// buffer re-skins everything that uses these CDATAs, no bundle re-record.
#include <controls.h>

IMAGE_NAMESPACE

// ----------------------------------------------------------------------
// WGSL source — kept in sync with shaders.wgsl. Edit there first, then
// paste here. (TODO: build-time generator that turns shaders.wgsl into
// an .inc file so the two can't drift.)
// ----------------------------------------------------------------------

static const char *const kImglibWGSL = R"WGSL(
struct Palette {
    colors : array<vec4<f32>, 16>,
};
@group(0) @binding(0) var<uniform> palette : Palette;

struct Lighting {
    light_dir     : vec4<f32>,
    light_color   : vec4<f32>,
    ambient_color : vec4<f32>,
};
@group(0) @binding(1) var<uniform> lighting : Lighting;

fn resolve_color(packed : u32) -> vec4<f32> {
    // alpha != 0 → literal RGBA.
    // alpha == 0 → palette index in low 4 bits. Slot 0 is reserved as
    // fully transparent so the convention CDATA(0) = transparent holds
    // (with the premultiplied blend that's a no-op blit, which is what
    // fill(0) / cleared bundle should produce).
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

struct VsSolidIn  { @location(0) pos : vec2<f32>, @location(1) color : u32, };
struct VsSolidOut { @builtin(position) clip_pos : vec4<f32>, @location(0) color : vec4<f32>, };
@vertex fn vs_solid(in : VsSolidIn) -> VsSolidOut {
    var o : VsSolidOut;
    o.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    o.color    = resolve_color(in.color);
    return o;
}
@fragment fn fs_solid(in : VsSolidOut) -> @location(0) vec4<f32> {
    // Premultiplied output to match the pipeline's blend state.
    return vec4<f32>(in.color.rgb * in.color.a, in.color.a);
}

@group(1) @binding(0) var samp : sampler;
@group(1) @binding(1) var tex  : texture_2d<f32>;

struct VsTexIn  { @location(0) pos : vec2<f32>, @location(1) uv : vec2<f32>, @location(2) tint : u32, };
struct VsTexOut { @builtin(position) clip_pos : vec4<f32>, @location(0) uv : vec2<f32>, @location(1) tint : vec4<f32>, };
@vertex fn vs_textured(in : VsTexIn) -> VsTexOut {
    var o : VsTexOut;
    o.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    o.uv = in.uv; o.tint = resolve_color(in.tint);
    return o;
}
@fragment fn fs_textured(in : VsTexOut) -> @location(0) vec4<f32> {
    let c = textureSample(tex, samp, in.uv) * in.tint;
    // Premultiplied output to match the pipeline's blend state.
    return vec4<f32>(c.rgb * c.a, c.a);
}

struct VsMultiIn  { @location(0) pos : vec2<f32>, @location(1) uv : vec2<f32>, @location(2) shadeR : u32, @location(3) shadeG : u32, @location(4) shadeB : u32, };
struct VsMultiOut { @builtin(position) clip_pos : vec4<f32>, @location(0) uv : vec2<f32>, @location(1) shadeR : vec4<f32>, @location(2) shadeG : vec4<f32>, @location(3) shadeB : vec4<f32>, };
@vertex fn vs_multi(in : VsMultiIn) -> VsMultiOut {
    var o : VsMultiOut;
    o.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    o.uv = in.uv;
    o.shadeR = resolve_color(in.shadeR);
    o.shadeG = resolve_color(in.shadeG);
    o.shadeB = resolve_color(in.shadeB);
    return o;
}
@fragment fn fs_multi(in : VsMultiOut) -> @location(0) vec4<f32> {
    let t = textureSample(tex, samp, in.uv);
    let rgb = t.r * in.shadeR.rgb + t.g * in.shadeG.rgb + t.b * in.shadeB.rgb;
    return vec4<f32>(rgb * t.a, t.a);
}

@group(1) @binding(2) var tex_normal : texture_2d<f32>;
struct VsNormalOut { @builtin(position) clip_pos : vec4<f32>, @location(0) uv : vec2<f32>, @location(1) tint : vec4<f32>, };
@vertex fn vs_normal(in : VsTexIn) -> VsNormalOut {
    var o : VsNormalOut;
    o.clip_pos = vec4<f32>(in.pos, 0.0, 1.0);
    o.uv = in.uv; o.tint = resolve_color(in.tint);
    return o;
}
@fragment fn fs_normal(in : VsNormalOut) -> @location(0) vec4<f32> {
    let diffuse = textureSample(tex,        samp, in.uv);
    let n_raw   = textureSample(tex_normal, samp, in.uv).rgb;
    let normal  = normalize(n_raw * 2.0 - vec3<f32>(1.0));
    let l_dir   = normalize(lighting.light_dir.xyz);
    let lambert = max(dot(normal, l_dir), 0.0);
    let ambient = lighting.ambient_color.rgb * lighting.ambient_color.a;
    let direct  = lighting.light_color.rgb * lambert;
    let lit     = vec4<f32>(diffuse.rgb * (ambient + direct), diffuse.a) * in.tint;
    return vec4<f32>(lit.rgb * lit.a, lit.a);
}
)WGSL";

// ----------------------------------------------------------------------
// Platform default surface format. Both formats are Vulkan-supported;
// pick whichever matches the swapchain Dawn will hand us.
// ----------------------------------------------------------------------

static WGPUTextureFormat default_surface_format( void )
{
#if defined( _WIN32 )
	return WGPUTextureFormat_BGRA8Unorm;
#else
	return WGPUTextureFormat_RGBA8Unorm;
#endif
}

// ----------------------------------------------------------------------
// One-time pipeline construction.
// ----------------------------------------------------------------------

static int g_pipelines_built = 0;
static WGPUShaderModule g_shader_module = NULL;
static WGPUPipelineLayout g_pipeline_layout_simple = NULL;  // bgl_globals_ + bgl_texture_
static WGPUPipelineLayout g_pipeline_layout_normal = NULL;  // bgl_globals_ + bgl_texture_normal_
static WGPUPipelineLayout g_pipeline_layout_solid  = NULL;  // bgl_globals_ only

// Depth-stencil format the pipelines were last built for. If the caller
// changes surface_depth_format_ later (e.g. wiring up three.js's HUD),
// build_all() will detect the mismatch and rebuild the pipeline objects
// so setPipeline inside the bundle encoder doesn't fail validation.
static WGPUTextureFormat g_pipelines_depth_format = WGPUTextureFormat_Undefined;

// Populated by make_depth_stencil_state when surface has depth. Read-only
// + always-compare + no-stencil = "we don't touch depth/stencil but we
// declare the attachment exists so we're pass-compatible."
static WGPUDepthStencilState g_dss;

// Returns &g_dss when depth attachment exists, NULL otherwise. The
// pipeline descriptor's `depthStencil` field is nullable; leaving it
// NULL means "no depth-stencil state" which is what pure-2D wants.
static WGPUDepthStencilState *make_depth_stencil_state( void )
{
	if( l.surface_depth_format_ == WGPUTextureFormat_Undefined )
		return NULL;
	MemSet( &g_dss, 0, sizeof( g_dss ) );
	g_dss.format              = l.surface_depth_format_;
	g_dss.depthWriteEnabled   = WGPUOptionalBool_False;        // read-only
	g_dss.depthCompare        = WGPUCompareFunction_Always;    // never reject
	g_dss.stencilReadMask     = 0xFFFFFFFF;
	g_dss.stencilWriteMask    = 0xFFFFFFFF;
	g_dss.stencilFront.compare     = WGPUCompareFunction_Always;
	g_dss.stencilFront.failOp      = WGPUStencilOperation_Keep;
	g_dss.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
	g_dss.stencilFront.passOp      = WGPUStencilOperation_Keep;
	g_dss.stencilBack              = g_dss.stencilFront;
	return &g_dss;
}

static WGPUSampler make_sampler( WGPUFilterMode filter )
{
	WGPUSamplerDescriptor sd = {};
	sd.addressModeU = WGPUAddressMode_ClampToEdge;
	sd.addressModeV = WGPUAddressMode_ClampToEdge;
	sd.addressModeW = WGPUAddressMode_ClampToEdge;
	sd.magFilter    = filter;
	sd.minFilter    = filter;
	sd.mipmapFilter = WGPUMipmapFilterMode_Nearest;
	sd.maxAnisotropy = 1;
	return wgpuDeviceCreateSampler( l.device_, &sd );
}

static WGPUBindGroupLayout make_bgl_globals( void )
{
	WGPUBindGroupLayoutEntry entries[2] = {};
	entries[0].binding    = 0;
	entries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
	entries[0].buffer.type = WGPUBufferBindingType_Uniform;
	entries[1].binding    = 1;
	entries[1].visibility = WGPUShaderStage_Fragment;
	entries[1].buffer.type = WGPUBufferBindingType_Uniform;
	WGPUBindGroupLayoutDescriptor d = {};
	d.entryCount = 2;
	d.entries    = entries;
	return wgpuDeviceCreateBindGroupLayout( l.device_, &d );
}

static WGPUBindGroupLayout make_bgl_texture( bool with_normal )
{
	WGPUBindGroupLayoutEntry entries[3] = {};
	entries[0].binding         = 0;
	entries[0].visibility      = WGPUShaderStage_Fragment;
	entries[0].sampler.type    = WGPUSamplerBindingType_Filtering;
	entries[1].binding         = 1;
	entries[1].visibility      = WGPUShaderStage_Fragment;
	entries[1].texture.sampleType    = WGPUTextureSampleType_Float;
	entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
	entries[2].binding         = 2;
	entries[2].visibility      = WGPUShaderStage_Fragment;
	entries[2].texture.sampleType    = WGPUTextureSampleType_Float;
	entries[2].texture.viewDimension = WGPUTextureViewDimension_2D;
	WGPUBindGroupLayoutDescriptor d = {};
	d.entryCount = with_normal ? 3 : 2;
	d.entries    = entries;
	return wgpuDeviceCreateBindGroupLayout( l.device_, &d );
}

static WGPUBuffer make_uniform_buffer( size_t bytes, const void *initial )
{
	WGPUBufferDescriptor bd = {};
	bd.size  = bytes;
	bd.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
	WGPUBuffer buf = wgpuDeviceCreateBuffer( l.device_, &bd );
	if( initial )
		wgpuQueueWriteBuffer( l.queue_, buf, 0, initial, bytes );
	return buf;
}

// Premultiplied-alpha color target. Shaders are expected to output
// (r*a, g*a, b*a, a). Final = src + dst*(1-src.a). This matches
// canvas alphaMode='premultiplied' which is what enables see-through
// composition against the desktop on layered windows.
static WGPUColorTargetState color_target( WGPUTextureFormat fmt, WGPUBlendState *blend_storage )
{
	blend_storage->color.operation = WGPUBlendOperation_Add;
	blend_storage->color.srcFactor = WGPUBlendFactor_One;             // src already premultiplied
	blend_storage->color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	blend_storage->alpha.operation = WGPUBlendOperation_Add;
	blend_storage->alpha.srcFactor = WGPUBlendFactor_One;
	blend_storage->alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	WGPUColorTargetState cts = {};
	cts.format    = fmt;
	cts.blend     = blend_storage;
	cts.writeMask = WGPUColorWriteMask_All;
	return cts;
}

static WGPURenderPipeline make_pipeline_solid( WGPUTextureFormat fmt )
{
	WGPUVertexAttribute attrs[2] = {};
	attrs[0].format         = WGPUVertexFormat_Float32x2;
	attrs[0].offset         = 0;
	attrs[0].shaderLocation = 0;
	attrs[1].format         = WGPUVertexFormat_Uint32;
	attrs[1].offset         = 8;
	attrs[1].shaderLocation = 1;
	WGPUVertexBufferLayout vbl = {};
	vbl.arrayStride    = 12;
	vbl.stepMode       = WGPUVertexStepMode_Vertex;
	vbl.attributeCount = 2;
	vbl.attributes     = attrs;

	WGPUBlendState blend = {};
	WGPUColorTargetState ct = color_target( fmt, &blend );

	WGPUFragmentState fs = {};
	fs.module      = g_shader_module;
	fs.entryPoint  = WGPUStringView{ "fs_solid", WGPU_STRLEN };
	fs.targetCount = 1;
	fs.targets     = &ct;

	WGPURenderPipelineDescriptor pd = {};
	pd.layout              = g_pipeline_layout_solid;
	pd.vertex.module       = g_shader_module;
	pd.vertex.entryPoint   = WGPUStringView{ "vs_solid", WGPU_STRLEN };
	pd.vertex.bufferCount  = 1;
	pd.vertex.buffers      = &vbl;
	pd.primitive.topology  = WGPUPrimitiveTopology_TriangleList;
	pd.depthStencil        = make_depth_stencil_state();
	pd.multisample.count   = 1;
	pd.multisample.mask    = 0xFFFFFFFF;
	pd.fragment            = &fs;
	return wgpuDeviceCreateRenderPipeline( l.device_, &pd );
}

static WGPURenderPipeline make_pipeline_textured( WGPUTextureFormat fmt,
                                                   const char *vs_entry, const char *fs_entry,
                                                   WGPUPipelineLayout layout )
{
	WGPUVertexAttribute attrs[3] = {};
	attrs[0].format         = WGPUVertexFormat_Float32x2;
	attrs[0].offset         = 0;
	attrs[0].shaderLocation = 0;
	attrs[1].format         = WGPUVertexFormat_Float32x2;
	attrs[1].offset         = 8;
	attrs[1].shaderLocation = 1;
	attrs[2].format         = WGPUVertexFormat_Uint32;
	attrs[2].offset         = 16;
	attrs[2].shaderLocation = 2;
	WGPUVertexBufferLayout vbl = {};
	vbl.arrayStride    = 20;
	vbl.stepMode       = WGPUVertexStepMode_Vertex;
	vbl.attributeCount = 3;
	vbl.attributes     = attrs;

	WGPUBlendState blend = {};
	WGPUColorTargetState ct = color_target( fmt, &blend );

	WGPUFragmentState fs = {};
	fs.module      = g_shader_module;
	fs.entryPoint  = WGPUStringView{ fs_entry, WGPU_STRLEN };
	fs.targetCount = 1;
	fs.targets     = &ct;

	WGPURenderPipelineDescriptor pd = {};
	pd.layout              = layout;
	pd.vertex.module       = g_shader_module;
	pd.vertex.entryPoint   = WGPUStringView{ vs_entry, WGPU_STRLEN };
	pd.vertex.bufferCount  = 1;
	pd.vertex.buffers      = &vbl;
	pd.primitive.topology  = WGPUPrimitiveTopology_TriangleList;
	pd.depthStencil        = make_depth_stencil_state();
	pd.multisample.count   = 1;
	pd.multisample.mask    = 0xFFFFFFFF;
	pd.fragment            = &fs;
	return wgpuDeviceCreateRenderPipeline( l.device_, &pd );
}

static WGPURenderPipeline make_pipeline_multi( WGPUTextureFormat fmt )
{
	WGPUVertexAttribute attrs[5] = {};
	attrs[0].format = WGPUVertexFormat_Float32x2; attrs[0].offset = 0;  attrs[0].shaderLocation = 0;
	attrs[1].format = WGPUVertexFormat_Float32x2; attrs[1].offset = 8;  attrs[1].shaderLocation = 1;
	attrs[2].format = WGPUVertexFormat_Uint32;    attrs[2].offset = 16; attrs[2].shaderLocation = 2;
	attrs[3].format = WGPUVertexFormat_Uint32;    attrs[3].offset = 20; attrs[3].shaderLocation = 3;
	attrs[4].format = WGPUVertexFormat_Uint32;    attrs[4].offset = 24; attrs[4].shaderLocation = 4;
	WGPUVertexBufferLayout vbl = {};
	vbl.arrayStride    = 28;
	vbl.stepMode       = WGPUVertexStepMode_Vertex;
	vbl.attributeCount = 5;
	vbl.attributes     = attrs;

	WGPUBlendState blend = {};
	WGPUColorTargetState ct = color_target( fmt, &blend );

	WGPUFragmentState fs = {};
	fs.module      = g_shader_module;
	fs.entryPoint  = WGPUStringView{ "fs_multi", WGPU_STRLEN };
	fs.targetCount = 1;
	fs.targets     = &ct;

	WGPURenderPipelineDescriptor pd = {};
	pd.layout              = g_pipeline_layout_simple;
	pd.vertex.module       = g_shader_module;
	pd.vertex.entryPoint   = WGPUStringView{ "vs_multi", WGPU_STRLEN };
	pd.vertex.bufferCount  = 1;
	pd.vertex.buffers      = &vbl;
	pd.primitive.topology  = WGPUPrimitiveTopology_TriangleList;
	pd.depthStencil        = make_depth_stencil_state();
	pd.multisample.count   = 1;
	pd.multisample.mask    = 0xFFFFFFFF;
	pd.fragment            = &fs;
	return wgpuDeviceCreateRenderPipeline( l.device_, &pd );
}

// Release everything build_all created. Used both at full shutdown and
// when the surface depth format changes (which requires rebuilding the
// pipeline objects against the new attachment state).
static void release_pipelines( void )
{
	if( l.pipe_solid_    ) { wgpuRenderPipelineRelease( l.pipe_solid_    ); l.pipe_solid_    = NULL; }
	if( l.pipe_textured_ ) { wgpuRenderPipelineRelease( l.pipe_textured_ ); l.pipe_textured_ = NULL; }
	if( l.pipe_multi_    ) { wgpuRenderPipelineRelease( l.pipe_multi_    ); l.pipe_multi_    = NULL; }
	if( l.pipe_normal_   ) { wgpuRenderPipelineRelease( l.pipe_normal_   ); l.pipe_normal_   = NULL; }
	g_pipelines_built = 0;
}

static void build_all( void )
{
	// Rebuild pipelines if the depth format changed since last build.
	if( g_pipelines_built && g_pipelines_depth_format != l.surface_depth_format_ ) {
		lprintf( "imglib-webgpu: rebuilding pipelines for depth format %d -> %d",
			(int)g_pipelines_depth_format, (int)l.surface_depth_format_ );
		release_pipelines();
	}
	if( g_pipelines_built )
		return;
	if( !l.device_ )
		return;

	// Shader module — one for all four pipelines.
	WGPUShaderSourceWGSL wgsl = {};
	wgsl.chain.sType   = WGPUSType_ShaderSourceWGSL;
	wgsl.code          = WGPUStringView{ kImglibWGSL, WGPU_STRLEN };
	WGPUShaderModuleDescriptor smd = {};
	smd.nextInChain    = &wgsl.chain;
	g_shader_module    = wgpuDeviceCreateShaderModule( l.device_, &smd );

	// Samplers.
	l.sampler_linear_  = make_sampler( WGPUFilterMode_Linear );
	l.sampler_nearest_ = make_sampler( WGPUFilterMode_Nearest );

	// Bind-group layouts.
	l.bgl_globals_         = make_bgl_globals();
	l.bgl_texture_         = make_bgl_texture( false );
	l.bgl_texture_normal_  = make_bgl_texture( true );

	// Pipeline layouts. Solid uses globals only (palette for resolve_color);
	// the other three use globals + a texture layout.
	{
		WGPUBindGroupLayout bgls[1] = { l.bgl_globals_ };
		WGPUPipelineLayoutDescriptor pld = {};
		pld.bindGroupLayoutCount = 1;
		pld.bindGroupLayouts     = bgls;
		g_pipeline_layout_solid  = wgpuDeviceCreatePipelineLayout( l.device_, &pld );
	}
	{
		WGPUBindGroupLayout bgls[2] = { l.bgl_globals_, l.bgl_texture_ };
		WGPUPipelineLayoutDescriptor pld = {};
		pld.bindGroupLayoutCount = 2;
		pld.bindGroupLayouts     = bgls;
		g_pipeline_layout_simple = wgpuDeviceCreatePipelineLayout( l.device_, &pld );
	}
	{
		WGPUBindGroupLayout bgls[2] = { l.bgl_globals_, l.bgl_texture_normal_ };
		WGPUPipelineLayoutDescriptor pld = {};
		pld.bindGroupLayoutCount = 2;
		pld.bindGroupLayouts     = bgls;
		g_pipeline_layout_normal = wgpuDeviceCreatePipelineLayout( l.device_, &pld );
	}

	// Palette uniform — 16 × vec4<f32> = 256 B.
	// Slot 0: reserved transparent — what resolve_color returns for CDATA(0).
	// Slots 1..14: pulled from PSI's GetBaseColor(0..13). PSI may not have
	//   initialised its base colours yet (priority preload ordering with
	//   our driver build); any slot returning alpha=0 falls back to white.
	// Slot 15: magenta sentinel so an out-of-range palette index is obvious.
	float palette[16][4] = { { 0.0f, 0.0f, 0.0f, 0.0f } };  // slot 0
	for( int i = 0; i < 14; i++ ) {
		CDATA c = sack::PSI::GetBaseColor( (INDEX)i );
		float r = (float)( ( c       ) & 0xFF ) / 255.0f;
		float g = (float)( ( c >>  8 ) & 0xFF ) / 255.0f;
		float b = (float)( ( c >> 16 ) & 0xFF ) / 255.0f;
		float a = (float)( ( c >> 24 ) & 0xFF ) / 255.0f;
		if( a == 0.0f ) {
			// PSI hasn't set this slot (or returned a transparent value).
			// Use opaque white so the colour shows up obviously rather
			// than vanishing under premultiplied blend.
			r = g = b = a = 1.0f;
		}
		palette[i + 1][0] = r;
		palette[i + 1][1] = g;
		palette[i + 1][2] = b;
		palette[i + 1][3] = a;
	}
	palette[15][0] = 1.0f; palette[15][1] = 0.0f;
	palette[15][2] = 1.0f; palette[15][3] = 1.0f;   // 15 magenta sentinel
	l.palette_buffer_ = make_uniform_buffer( sizeof( palette ), palette );

	// Lighting uniform — 48 B, but pad to 64 for alignment friendliness.
	float lighting[12] = {
		 0.0f,  1.0f,  0.5f, 0.0f,   // light_dir (xyz) — top-front
		 1.0f,  0.95f, 0.85f, 0.0f,  // light_color (warm white)
		 0.30f, 0.30f, 0.35f, 1.0f,  // ambient_color + intensity (alpha = scalar)
	};
	l.lighting_buffer_ = make_uniform_buffer( sizeof( lighting ), lighting );

	// Globals bind group — combines palette + lighting at group(0).
	WGPUBindGroupEntry gentries[2] = {};
	gentries[0].binding = 0;
	gentries[0].buffer  = l.palette_buffer_;
	gentries[0].size    = sizeof( palette );
	gentries[1].binding = 1;
	gentries[1].buffer  = l.lighting_buffer_;
	gentries[1].size    = sizeof( lighting );
	WGPUBindGroupDescriptor bgd = {};
	bgd.layout     = l.bgl_globals_;
	bgd.entryCount = 2;
	bgd.entries    = gentries;
	l.globals_group_ = wgpuDeviceCreateBindGroup( l.device_, &bgd );

	// Pipelines.
	WGPUTextureFormat fmt = l.surface_format_ ? l.surface_format_ : default_surface_format();
	l.surface_format_     = fmt;
	l.pipe_solid_    = make_pipeline_solid( fmt );
	l.pipe_textured_ = make_pipeline_textured( fmt, "vs_textured", "fs_textured", g_pipeline_layout_simple );
	l.pipe_multi_    = make_pipeline_multi( fmt );
	l.pipe_normal_   = make_pipeline_textured( fmt, "vs_normal",   "fs_normal",   g_pipeline_layout_normal );

	g_pipelines_depth_format = l.surface_depth_format_;
	g_pipelines_built = 1;
}

// ----------------------------------------------------------------------
// Pipeline getters — override the stubs in driver.cc.
// ----------------------------------------------------------------------

WGPURenderPipeline webgpu_image_get_pipe_solid   ( void ) { build_all(); return l.pipe_solid_; }
WGPURenderPipeline webgpu_image_get_pipe_textured( void ) { build_all(); return l.pipe_textured_; }
WGPURenderPipeline webgpu_image_get_pipe_multi   ( void ) { build_all(); return l.pipe_multi_; }
WGPURenderPipeline webgpu_image_get_pipe_normal  ( void ) { build_all(); return l.pipe_normal_; }

// ----------------------------------------------------------------------
// Setters.
// ----------------------------------------------------------------------

void webgpu_image_set_surface_format( WGPUTextureFormat fmt )
{
	// Honoured only before build_all() runs. After pipelines exist this
	// is a no-op — but in practice the format is platform-fixed so there's
	// no real reason to change it post-build.
	l.surface_format_ = fmt;
}

void webgpu_image_set_surface_depth_format( WGPUTextureFormat fmt )
{
	if( l.surface_depth_format_ == fmt )
		return;
	l.surface_depth_format_ = fmt;
	l.depth_format_generation_++;   // forces walker to re-record bundles
	lprintf( "imglib-webgpu: surface depth format -> %d (gen %u)",
		(int)fmt, l.depth_format_generation_ );
}

void webgpu_image_set_light_direction( float x, float y, float z )
{
	if( !l.lighting_buffer_ ) return;
	float dir[4] = { x, y, z, 0.0f };
	wgpuQueueWriteBuffer( l.queue_, l.lighting_buffer_, 0, dir, sizeof( dir ) );
}

void webgpu_image_set_light_color( float r, float g, float b )
{
	if( !l.lighting_buffer_ ) return;
	float col[4] = { r, g, b, 0.0f };
	wgpuQueueWriteBuffer( l.queue_, l.lighting_buffer_, 16, col, sizeof( col ) );
}

void webgpu_image_set_ambient( float r, float g, float b, float intensity )
{
	if( !l.lighting_buffer_ ) return;
	float amb[4] = { r, g, b, intensity };
	wgpuQueueWriteBuffer( l.queue_, l.lighting_buffer_, 32, amb, sizeof( amb ) );
}

void webgpu_image_set_palette_entry( int idx, float r, float g, float b, float a )
{
	if( !l.palette_buffer_ || idx < 0 || idx >= 16 ) return;
	float c[4] = { r, g, b, a };
	wgpuQueueWriteBuffer( l.queue_, l.palette_buffer_, idx * 16, c, sizeof( c ) );
}

// Re-pull PSI base colours into palette slots 1..14. Call this from JS
// after PSI has been set up (its preload may run after ours) or after
// any SetBaseColor / theme swap so the GPU palette mirrors the live PSI
// theme. Cheap — one wgpuQueueWriteBuffer covers all 14 slots.
void webgpu_image_refresh_psi_palette( void )
{
	if( !l.palette_buffer_ ) return;
	float slots[14][4];
	for( int i = 0; i < 14; i++ ) {
		CDATA c = sack::PSI::GetBaseColor( (INDEX)i );
		float r = (float)( ( c       ) & 0xFF ) / 255.0f;
		float g = (float)( ( c >>  8 ) & 0xFF ) / 255.0f;
		float b = (float)( ( c >> 16 ) & 0xFF ) / 255.0f;
		float a = (float)( ( c >> 24 ) & 0xFF ) / 255.0f;
		if( a == 0.0f ) { r = g = b = a = 1.0f; }   // unset → opaque white fallback
		slots[i][0] = r; slots[i][1] = g; slots[i][2] = b; slots[i][3] = a;
	}
	wgpuQueueWriteBuffer( l.queue_, l.palette_buffer_, 1 * 16, slots, sizeof( slots ) );
	// (palette[0] = transparent and palette[15] = magenta sentinel are
	//  left untouched.)
	l.palette_generation_++;
}

void webgpu_image_set_normal_map( Image diffuse, Image normal )
{
	if( !diffuse ) return;
	struct webgpu_per_image *pi = webgpu_per_image_get( diffuse );
	if( pi ) {
		pi->normal_image_ = normal;
		// Invalidate any cached normal bind group.
		if( pi->normal_bg_ ) {
			wgpuBindGroupRelease( pi->normal_bg_ );
			pi->normal_bg_ = NULL;
		}
		pi->normal_bg_for_image_ = NULL;
	}
}

// ----------------------------------------------------------------------
// Source-texture upload + per-source bind groups.
// ----------------------------------------------------------------------

// Walk to the topmost parent. Subimages share their parent's underlying
// pixmap (src->image points into the parent's bytes at the subimage's
// row 0); only the topmost parent has its own buffer. The GPU texture
// always covers the whole atlas — subimages just sample a UV sub-rect
// at draw time (see blot_record's atlas-relative coord conversion).
static Image image_root( Image img )
{
	while( img && img->pParent )
		img = img->pParent;
	return img;
}

WGPUTexture webgpu_image_ensure_src_texture( Image src )
{
	if( !src )
		return NULL;
	// Texture, view, and bind-group cache always live on the root image
	// (the actual pixmap owner). Subimages reuse the same texture.
	Image root = image_root( src );
	if( !root || !root->image )
		return NULL;
	if( !webgpu_image_ensure_init() )
		return NULL;

	struct webgpu_per_image *pi = webgpu_per_image_get( root );
	if( !pi )
		return NULL;

	uint32_t want_w = (uint32_t)root->real_width;
	uint32_t want_h = (uint32_t)root->real_height;
	if( !want_w || !want_h )
		return NULL;

	// If existing texture mismatches dimensions, drop and recreate.
	if( pi->src_texture_ ) {
		uint32_t cur_w = wgpuTextureGetWidth ( pi->src_texture_ );
		uint32_t cur_h = wgpuTextureGetHeight( pi->src_texture_ );
		if( cur_w != want_w || cur_h != want_h ) {
			if( pi->src_view_ )    { wgpuTextureViewRelease( pi->src_view_ );    pi->src_view_ = NULL; }
			wgpuTextureRelease( pi->src_texture_ ); pi->src_texture_ = NULL;
			// Bind groups bound the old view — invalidate.
			if( pi->diffuse_bg_ ) { wgpuBindGroupRelease( pi->diffuse_bg_ ); pi->diffuse_bg_ = NULL; }
			if( pi->normal_bg_  ) { wgpuBindGroupRelease( pi->normal_bg_  ); pi->normal_bg_  = NULL; }
			pi->src_upload_generation_ = 0;
		}
	}

	if( !pi->src_texture_ ) {
		WGPUTextureDescriptor td = {};
		td.usage         = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
		td.dimension     = WGPUTextureDimension_2D;
		td.size.width    = want_w;
		td.size.height   = want_h;
		td.size.depthOrArrayLayers = 1;
		td.format        = l.surface_format_;   // matches swapchain
		td.mipLevelCount = 1;
		td.sampleCount   = 1;
		pi->src_texture_ = wgpuDeviceCreateTexture( l.device_, &td );

		WGPUTextureViewDescriptor vd = {};
		vd.format          = l.surface_format_;
		vd.dimension       = WGPUTextureViewDimension_2D;
		vd.mipLevelCount   = 1;
		vd.arrayLayerCount = 1;
		pi->src_view_ = wgpuTextureCreateView( pi->src_texture_, &vd );
	}

	// Upload from the root image's bytes (not the subimage's — src->image
	// for a subimage already points into root's pixmap at an offset, but
	// we always upload the whole parent atlas).
	if( pi->src_upload_generation_ == 0 || ( root->flags & IF_FLAG_UPDATED ) ) {
		WGPUTexelCopyTextureInfo dst = {};
		dst.texture  = pi->src_texture_;
		dst.aspect   = WGPUTextureAspect_All;
		WGPUTexelCopyBufferLayout layout = {};
		layout.offset       = 0;
		layout.bytesPerRow  = root->pwidth * 4;
		layout.rowsPerImage = want_h;
		WGPUExtent3D ext = { want_w, want_h, 1 };
		wgpuQueueWriteTexture( l.queue_, &dst, root->image,
		                       (size_t)root->pwidth * 4 * want_h, &layout, &ext );
		root->flags &= ~IF_FLAG_UPDATED;
		pi->src_upload_generation_++;
	}

	return pi->src_texture_;
}

WGPUBindGroup webgpu_image_get_diffuse_bg( Image src )
{
	WGPUTexture tex = webgpu_image_ensure_src_texture( src );
	if( !tex ) return NULL;
	// Cache lives on the root image (same place the texture is stored).
	Image root = image_root( src );
	struct webgpu_per_image *pi = webgpu_per_image_get( root );
	if( !pi || !pi->src_view_ ) return NULL;
	if( pi->diffuse_bg_ )
		return pi->diffuse_bg_;
	WGPUBindGroupEntry e[2] = {};
	e[0].binding     = 0;
	e[0].sampler     = l.sampler_linear_;
	e[1].binding     = 1;
	e[1].textureView = pi->src_view_;
	WGPUBindGroupDescriptor bgd = {};
	bgd.layout     = l.bgl_texture_;
	bgd.entryCount = 2;
	bgd.entries    = e;
	pi->diffuse_bg_ = wgpuDeviceCreateBindGroup( l.device_, &bgd );
	return pi->diffuse_bg_;
}

WGPUBindGroup webgpu_image_get_normal_bg( Image src )
{
	if( !src ) return NULL;
	Image root = image_root( src );
	struct webgpu_per_image *pi = webgpu_per_image_get( root );
	if( !pi || !pi->normal_image_ ) return NULL;

	// Re-create if the linked normal image changed.
	if( pi->normal_bg_ && pi->normal_bg_for_image_ != pi->normal_image_ ) {
		wgpuBindGroupRelease( pi->normal_bg_ );
		pi->normal_bg_ = NULL;
	}
	if( pi->normal_bg_ )
		return pi->normal_bg_;

	WGPUTexture diff = webgpu_image_ensure_src_texture( src );
	WGPUTexture norm = webgpu_image_ensure_src_texture( pi->normal_image_ );
	struct webgpu_per_image *npi = webgpu_per_image_get( image_root( pi->normal_image_ ) );
	if( !diff || !norm || !pi->src_view_ || !npi || !npi->src_view_ )
		return NULL;

	WGPUBindGroupEntry e[3] = {};
	e[0].binding     = 0;
	e[0].sampler     = l.sampler_linear_;
	e[1].binding     = 1;
	e[1].textureView = pi->src_view_;
	e[2].binding     = 2;
	e[2].textureView = npi->src_view_;
	WGPUBindGroupDescriptor bgd = {};
	bgd.layout     = l.bgl_texture_normal_;
	bgd.entryCount = 3;
	bgd.entries    = e;
	pi->normal_bg_ = wgpuDeviceCreateBindGroup( l.device_, &bgd );
	pi->normal_bg_for_image_ = pi->normal_image_;
	return pi->normal_bg_;
}

void webgpu_image_set_surface_size( uint32_t w, uint32_t h )
{
	l.surface_w_ = w;
	l.surface_h_ = h;
}

IMAGE_NAMESPACE_END
