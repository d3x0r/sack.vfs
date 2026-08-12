

import {sack} from "sack-gui"

const r = sack.Renderer(); 
console.log( "got renderer?", r );


console.log( "Navigator?", navigator, Object.getPrototypeOf( navigator ));

console.log( "Fun:", navigator.hardwareConcurrency, navigator.language, navigator.languages, navigator.userAgent, navigator.platform );

console.log( "Adapter?",  sack.gpu.requestAdapter().then( async (a)=>{
		console.log( "waited and got:", a, a.info )
/*
	const d = await a.requestDevice();
	console.log( 'd:', d );
	const b = d.createBuffer({ size: 64, usage: 8 });  // 8 = COPY_DST
	console.log( "b?", b );
*/
}) );


try {
const a = await navigator.gpu.requestAdapter();

const d = await a.requestDevice();

const b = d.createBuffer({ size: 64, usage: 8 });  // 8 = COPY_DST
console.log( "b?", b );
b.destroy();
console.log( "Destroyed buffer?" );


const texture =  r.getContext('webgpu').configure({device:d, format:'bgra8unorm',
  alphaMode: 'opaque',
  usage: GPUTextureUsage.RENDER_ATTACHMENT,  }).getCurrentTexture()


}catch(err) {
	console.log( "uncaught reject(except):", err );
}

r.show();