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
	pd.multisample.count   = 1;
	pd.multisample.mask    = 0xFFFFFFFF;
	pd.fragment            = &fs;
	return wgpuDeviceCreateRenderPipeline( l.device_, &pd );
}

static void build_all( void )
{
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

	// Palette uniform — 16 × vec4<f32> = 256 B. Defaults: classic 16-colour
	// CGA-ish palette. PSI BASECOLOR setters rewrite individual slots.
	float palette[16][4] = {
		{ 0.00f, 0.00f, 0.00f, 1.0f },  // 0 black
		{ 0.50f, 0.50f, 0.50f, 1.0f },  // 1 dark grey
		{ 0.75f, 0.75f, 0.75f, 1.0f },  // 2 light grey  (3.1 face)
		{ 1.00f, 1.00f, 1.00f, 1.0f },  // 3 white
		{ 0.50f, 0.00f, 0.00f, 1.0f },  // 4 dark red
		{ 0.00f, 0.50f, 0.00f, 1.0f },  // 5 dark green
		{ 0.00f, 0.00f, 0.50f, 1.0f },  // 6 dark blue
		{ 0.50f, 0.50f, 0.00f, 1.0f },  // 7 dark yellow
		{ 1.00f, 0.00f, 0.00f, 1.0f },  // 8 red
		{ 0.00f, 1.00f, 0.00f, 1.0f },  // 9 green
		{ 0.00f, 0.00f, 1.00f, 1.0f },  // 10 blue
		{ 1.00f, 1.00f, 0.00f, 1.0f },  // 11 yellow
		{ 1.00f, 0.00f, 1.00f, 1.0f },  // 12 magenta
		{ 0.00f, 1.00f, 1.00f, 1.0f },  // 13 cyan
		{ 0.50f, 0.25f, 0.00f, 1.0f },  // 14 brown (window border)
		{ 0.10f, 0.10f, 0.20f, 1.0f },  // 15 title bar
	};
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

WGPUTexture webgpu_image_ensure_src_texture( Image src )
{
	if( !src || !src->image )
		return NULL;
	if( !webgpu_image_ensure_init() )
		return NULL;

	struct webgpu_per_image *pi = webgpu_per_image_get( src );
	if( !pi )
		return NULL;

	uint32_t want_w = (uint32_t)src->real_width;
	uint32_t want_h = (uint32_t)src->real_height;
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

	// Upload CPU pixels. sack CDATA is packed little-endian ARGB → BGRA
	// byte order on disk, which is the byte order BGRA8Unorm wants on
	// Windows (the format we use there). Direct memcpy; no swizzle.
	//
	// First-time check: if we just allocated the texture, src_upload_generation_
	// is still 0 — upload regardless of IF_FLAG_UPDATED, because a freshly-
	// loaded image won't have that flag set (sack reserves it for subsequent
	// CPU mutations). Without this fallback we'd bind an empty texture and
	// nothing visible would render.
	if( pi->src_upload_generation_ == 0 || ( src->flags & IF_FLAG_UPDATED ) ) {
		WGPUTexelCopyTextureInfo dst = {};
		dst.texture  = pi->src_texture_;
		dst.aspect   = WGPUTextureAspect_All;
		WGPUTexelCopyBufferLayout layout = {};
		layout.offset       = 0;
		layout.bytesPerRow  = src->pwidth * 4;
		layout.rowsPerImage = want_h;
		WGPUExtent3D ext = { want_w, want_h, 1 };
		wgpuQueueWriteTexture( l.queue_, &dst, src->image, src->pwidth * 4 * want_h, &layout, &ext );
		src->flags &= ~IF_FLAG_UPDATED;
		pi->src_upload_generation_++;
	}

	return pi->src_texture_;
}

WGPUBindGroup webgpu_image_get_diffuse_bg( Image src )
{
	WGPUTexture tex = webgpu_image_ensure_src_texture( src );
	if( !tex ) return NULL;
	struct webgpu_per_image *pi = webgpu_per_image_get( src );
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
	struct webgpu_per_image *pi = webgpu_per_image_get( src );
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
	struct webgpu_per_image *npi = webgpu_per_image_get( pi->normal_image_ );
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
