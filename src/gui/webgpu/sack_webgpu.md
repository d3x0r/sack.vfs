

# Interface with Dawn library to JS to host Three.js inbetween.


webgpu.idl from          https://gpuweb.github.io/gpuweb/webgpu.idl   (alt in theory https://www.w3.org/TR/webgpu/webgpu.idl was down)


dawn.json source   should be refreshed from   ${CMAKE_BINARY_DIR}/Dawn_External-prefix/src/Dawn_External/src/dawn/dawn.json

 - v20260423.175430

## Lifetime and Usage

WebGPU functionality itself uses async events, but does not itself keep the process alive; when testing this interface
in isolation, a timeout should be added to keep the process alive long enough to finish the work.  Alternativly the 
native work flow is to open a renderer, which will keep the process alive as long as it is open.


Pure WebGPU test scripts (no sack.Renderer(), no other open handles) may
race the process exit between awaits. The Dawn-event pump only refs the
uv loop during a Dawn async call (begin/end pair). To keep a bare WebGPU
script alive across long idle stretches, either open a Renderer at the
top, or use a setTimeout / setInterval anchor.