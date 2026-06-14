import sack from 'sack-gui';

import "./3d-shell/document.mjs";

const r = document.createElement( "canvas" );

let shown = false;
// WebGPU device — sack.gpu is the navigator.gpu equivalent.

const {ctx,device} = await r.configured;

const i = sack.Image( "j3.png" );

r.on('draw', (surface) => {
  // surface is the sack Image backing the renderer.
  // Per frame: acquire the swap-chain texture, encode a single render
  // pass whose loadOp clears to a solid colour, end the pass, submit.
  const tex  = ctx.getCurrentTexture();
  const view = tex.createView();

	//console.log('surface tex size:', tex.width, tex.height);

	surface.fill( 0, 0, 150, 150, 0xffffffff );
	for( let n = 0; n < 200; n+= 20 ) {
		surface.line( n, 0, n, 500, 0x90500000 );
		surface.line( 0, n, 500, n, 0x90000050 );
		surface.line( 200-n, n, 500-n, 150+n, 0x90005000 );
//		surface.line( 200-n, n, 500-n, n, 0x9005000 );
	}
//	surface.drawImage( i, 500, -500);
	surface.drawImage( i, 0, 0, 800, 600, 0, 0, 800, 600);

//	surface.drawImage( i, 200, 300, 500, 500, 0, 0, 800, 600 );


  const enc = device.createCommandEncoder();
  const pass = enc.beginRenderPass({
    colorAttachments: [{
      view,
      loadOp: 'clear',
      storeOp: 'store',
      clearValue: { r: 0.2, g: 0.5, b: 0.9, a: 0.3 },   // ? your colour
    }],
  });
  pass.end();    // ? imglib-webgpu's dispatcher fires here on the surface pass
  device.queue.submit([ enc.finish() ]);
});

// if r.show is called first, a black surface shows first, and then the draw happens.
// if draw is already set, then show will trigger that code and be a clean surface.
r.show()
