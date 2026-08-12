
// Bind groups with mixed resource types:
const bg = device.createBindGroup({
  layout, entries: [
    { binding: 0, resource: { buffer: uniformBuf, offset: 0, size: 64 } },
    { binding: 1, resource: sampler },
    { binding: 2, resource: texture.createView() }
  ]
});

// Texture data upload:
device.queue.writeTexture(
  { texture, mipLevel: 0, origin: [0,0,0] },
  pixelData,
  { bytesPerRow: 4 * width, rowsPerImage: height },
  [width, height, 1]
);

// Buffer mapping (zero-copy round trip):
await stagingBuf.mapAsync(GPUMapMode.WRITE);
const view = new Float32Array(stagingBuf.getMappedRange());
view[0] = 1.0;  // writes into Dawn's mapped GPU memory directly
stagingBuf.unmap();  // ArrayBuffer detached after this