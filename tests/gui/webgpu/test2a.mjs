
// Now JS-callable end-to-end for the colored triangle:

import {sack} from "sack-gui"

const disk = sack.Volume();

const r = sack.Renderer('hello', 1020, 500, 800, 600, null, 0x2000);

let NN = 0;
r.on( "draw", (a)=>{
	let color = sack.Image.Color( 0, 0, 255, 128 );
		console.log( "C:", color.toString() );
	let j1 = a.png;
	disk.write( "j"+(++NN)+".png", j1 );
	console.log( j1 );
	a.fillOver( 0, 0, 500, 500, color );
const image = a;
	for( let i = 0;i < 255; i++ ) {
		color.a = i;
		color.g = 0;
		image.line( 0, i, 127, i, color );
		color.g = 128;
		image.line( 128, i, 256, i, color );
	}

	j1 = a.png;
	disk.write( "j"+(++NN)+".png", j1 );
	console.log( "a:", a );
} );
r.show();


if(0) {
const a = await navigator.gpu.requestAdapter({ powerPreference: 'high-performance' });
const d = await a.requestDevice();

console.log( "features?", a.features, d.features );

const ctx = r.getContext('webgpu');
ctx.configure({ device: d, format: 'bgra8unorm', alphaMode:'premultiplied' });

const shader = d.createShaderModule({ code: `
  @vertex fn vs(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f {
    let p = array<vec2f,3>(vec2f(0,0.5), vec2f(-0.5,-0.5), vec2f(0.5,-0.5));
    return vec4f(p[i], 0, 1);
  }
  @fragment fn fs() -> @location(0) vec4f {
    return vec4f(0.3, 0.5, 0, 0.3);  // orange
  }
`});


const pipeline = d.createRenderPipeline({
  layout: 'auto',
  vertex:   { module: shader, entryPoint: 'vs' },
  fragment: { module: shader, entryPoint: 'fs', targets: [{ format: 'bgra8unorm' }] },
  primitive: { topology: 'triangle-list' }
});

// per-frame:
const tex  = ctx.getCurrentTexture();
const view = tex.createView();
const enc  = d.createCommandEncoder();
const pass = enc.beginRenderPass({
  colorAttachments: [{ view, clearValue: [0,0,0.2,0.2], loadOp: 'clear', storeOp: 'store' }]
});
pass.setPipeline(pipeline);
pass.draw(3, 1);

pass.end();
d.queue.submit([enc.finish()]);
ctx.present();


       }