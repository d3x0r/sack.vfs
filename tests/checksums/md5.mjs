
import {sack} from "sack.vfs"

const disk = sack.Volume();
const file = disk.read( "test.dat" );
// md5 ce114e4501d2f4e2dcea3e17b546f339

const md5 = sack.SaltyRNG.md5( file );
const str = buf2hex( md5 );
console.log( "md5 is:", md5, str );

// sha1 a54d88e06612d820bc3be72877c74f257b561b19
const sha1 = sack.SaltyRNG.sha1( file );
const str_sha1 = buf2hex( sha1 );
console.log( "sha1 is:", sha1, str_sha1 );

// sha256 c7be1ed902fb8dd4d48997c6452f5d7e509fbcdbe2808b16bcf4edce4c07d14e
const sha256 = sack.SaltyRNG.sha256( file );
const str_sha256 = buf2hex( sha256 );
console.log( "sha256 is:", sha256, str_sha256 );

function buf2hex(buffer) { // buffer is an ArrayBuffer
  return [...new Uint8Array(buffer)]
      .map(x => x.toString(16).padStart(2, '0'))
      .join('');
}
