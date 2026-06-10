import {sack} from "sack-gui"


const r = sack.Renderer('hello', 800, 600);
const a = await navigator.gpu.requestAdapter({ powerPreference: 'high-performance' });
const d = await a.requestDevice();
const ctx = r.getContext('webgpu');
ctx.configure({ device: d, format: 'bgra8unorm', alphaMode: 'opaque' });

const tex   = ctx.getCurrentTexture();
console.log(tex.format, tex.width, tex.height);   // ? 'bgra8unorm', 800, 600
const view  = tex.createView();

const enc   = d.createCommandEncoder({ label: 'frame' });
const pass  = enc.beginRenderPass({
  colorAttachments: [{
    view,
    clearValue: [1, 0, 0, 1],       // red!
    loadOp:  'clear',
    storeOp: 'store'
  }]
});
pass.end();
const cmd = enc.finish();
d.queue.submit([cmd]);

r.show();

ctx.present();