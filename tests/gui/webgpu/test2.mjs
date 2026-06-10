
// Now JS-callable end-to-end for the colored triangle:

import {sack} from "sack-gui"

const displayMonitor = sack.Task.getDisplays().device.find( d=>d.primary );
console.log( displayMonitor );
const r = sack.Renderer('hello', displayMonitor.width/2 - 800/2, displayMonitor.height/2 - 600/2, 800, 600, sack.Task.style.windowEx.WS_EX_NOREDIRECTIONBITMAP );

r.show();

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


