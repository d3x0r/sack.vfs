"use strict";

import {SaltyRNG} from "./salty_random_generator.js"

const RNG = SaltyRNG( (saltbuf)=>saltbuf.push( Date.now() ), {mode:1} );
var RNG2 = SaltyRNG( getSalt2, {mode:0} );

var salt = null;
function getSalt2 (saltbuf) {
    if( salt ) {
        saltbuf.push( salt );
        salt = null;
    }
}

function generator() {
    // this is an ipv6 + UUID
    return base64ArrayBuffer( RNG.getBuffer(8*(16+16)) );
}

export {generator};

function short_generator() {
    // this is an ipv6 + UUID
    var ID = RNG.getBuffer(8*(12));
    var now = Date.now() / 1000 | 0;
    ID[0] = ( now & 0xFF0000 ) >> 16;
    ID[1] = ( now & 0x00FF00 ) >> 8;
    ID[2] = ( now & 0x0000FF );
    return base64ArrayBuffer( ID );
}
export{ short_generator };
function regenerator(s) {
    salt = s;
    RNG2.reset();
    // this is an ipv6 + UUID
    return base64ArrayBuffer( RNG2.getBuffer(8*(16+16)) );
}
export {regenerator};

 function u16generator() {
    // this is an ipv6 + UUID
    var out = [];
    for( var c = 0; c < 25; c++ ) {
    	var ch = RNG.getBits( 10 ); if( ch < 32 ) ch |= 64;
    	out[c] = String.fromCodePoint( ch );
    }
    return out.join('');
}

export {u16generator};

// Converts an ArrayBuffer directly to base64, without any intermediate 'convert to string then
// use window.btoa' step. According to my tests, this appears to be a faster approach:
// http://jsperf.com/encoding-xhr-image-data/5
// doesn't have to be reversable....
const encodings = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789$_'
var u8 = '';

for( var x = 0; x < 256; x++ ) {
	if( x < 64 ) {
		u8 += String.fromCharCode(x);
	}
	else if( x < 128 ) {
		u8 += String.fromCharCode(x);
	}
	else {
		u8 += String.fromCharCode(x);
	}
}
//console.log( "u8 is...", u8 );

function base64ArrayBuffer(arrayBuffer) {
  var base64    = ''

  var bytes         = new Uint8Array(arrayBuffer)
  var byteLength    = bytes.byteLength
  var byteRemainder = byteLength % 3
  var mainLength    = byteLength - byteRemainder

  var a, b, c, d
  var chunk
  //throw "who's using this?"
  //console.log( "buffer..", arrayBuffer )
  // Main loop deals with bytes in chunks of 3
  for (var i = 0; i < mainLength; i = i + 3) {
    // Combine the three bytes into a single integer
    chunk = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2]

    // Use bitmasks to extract 6-bit segments from the triplet
    a = (chunk & 16515072) >> 18 // 16515072 = (2^6 - 1) << 18
    b = (chunk & 258048)   >> 12 // 258048   = (2^6 - 1) << 12
    c = (chunk & 4032)     >>  6 // 4032     = (2^6 - 1) << 6
    d = chunk & 63               // 63       = 2^6 - 1

    // Convert the raw binary segments to the appropriate ASCII encoding
    base64 += encodings[a] + encodings[b] + encodings[c] + encodings[d]
  }

  // Deal with the remaining bytes and padding
  if (byteRemainder == 1) {
    chunk = bytes[mainLength]
    a = (chunk & 252) >> 2 // 252 = (2^6 - 1) << 2
    // Set the 4 least significant bits to zero
    b = (chunk & 3)   << 4 // 3   = 2^2 - 1
    base64 += encodings[a] + encodings[b] + '=='
  } else if (byteRemainder == 2) {
    chunk = (bytes[mainLength] << 8) | bytes[mainLength + 1]
    a = (chunk & 64512) >> 10 // 64512 = (2^6 - 1) << 10
    b = (chunk & 1008)  >>  4 // 1008  = (2^6 - 1) << 4
    // Set the 2 least significant bits to zero
    c = (chunk & 15)    <<  2 // 15    = 2^4 - 1
    base64 += encodings[a] + encodings[b] + encodings[c] + '='
  }
  //console.log( "dup?", base64)
  return base64
}

// Converts an ArrayBuffer directly to base64, without any intermediate 'convert to string then
// use window.btoa' step. According to my tests, this appears to be a faster approach:
// http://jsperf.com/encoding-xhr-image-data/5
// doesn't have to be reversable....



var xor_code_encodings = {};//'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_'
for( var a = 0; a < encodings.length; a++  ) {
   var r = (xor_code_encodings[encodings[a]]={} );
   for( var b = 0; b < encodings.length; b++  ) {
	r[encodings[b]] = encodings[a^b];
   }
}
xor_code_encodings['='] = {'=': '='};

function xor(a,b) {
  var c = "";
  for( var n = 0; n < a.length; n++ ) {
	c += xor_code_encodings[a[n]][b[n]];
  }
  return c
}
export {xor}

function dexor(a,b,d,e) {
  var r = "";
  var n1 = (d-1)*((a.length/e)|0);
  var n2 = (d)*(a.length/e)|0;

  for( var n = 0; n < n1; n++ )
	r += a[n];
  for( ; n < n2; n++ )
	r += xor_code_encodings[a[n]][b[n]];
  for( ; n < n2; n++ )
	r += a[n];

  return r
}

function txor(a,b) {
	const d = [...b].map( (c)=>c.codePointAt(0) );
	const keylen = d.length;
	return [...a].map( (c,n)=>String.fromCodePoint( c.codePointAt(0)^d[n%keylen] )).join("");
}
export {txor as u16xor}

function makeXKey( key, step ) {
    return { key : key, keybuf: key?toUTF8Array(key):null, step: step?step:0
	, setKey(key,step) { this.key = key; this.keybuf = toUTF8Array(key); this.step = step?step:0; } };
}

function makeU16Key( ) {
    return u16generator();
}

export {makeXKey as xkey};
export {makeU16Key as ukey };
