
import sack from "@d3x0r/sack-gui"

const r = sack.Renderer( "title", 1600, 800); 

const a = await navigator.gpu.requestAdapter({ powerPreference: "high-performance" });
console.log(a.info);  // should now report NVIDIA 4070 Ti

//setTimeout( ()=>{}, 3000 );

try {
const d = await a.requestDevice();

console.log( "a?", a, d );

const b = await d.createBuffer({
  size: 64,
  usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.STORAGE,
  label: "my-buf"
});
b.destroy();

} catch(err) {
	console.log( err );
}


r.show();