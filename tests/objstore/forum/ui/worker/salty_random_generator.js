"use strict";

// usage
//  var RNG = require( "salty_random_generator")( callback }
//    constructor callback is used as a source of salt to the generator
//    the callback is passed an array to which strings are expected to be added
//     ( [] )=>{ [].push( more_salt ); }
//
//    - methods on RNG
//         reset()
//                clear current random state, and restart
//
//         getBits( /* 0-31 */ )
//                return a Number that is that many bits from the random stream
//
//         getBuffer( /* 0-n */ )
//                returns a ArrayBuffer that is that many bits of randomness...
//
//         save()
//                return an object representing the current RNG state
//
//         restore( o )
//                use object to restore RNG state.
//
//          feed( buf )
//                feed a raw uint8array.
//

const compat = true;


function SaltyRNG(f, opt) {

	const K12_SQUEEZE_LENGTH = 32768;

	const shabuf = opt?( opt.mode === 0 )?new SHA256() : null
                          : new SHA256();
	const k12buf = opt?( opt.mode === 1 )?KangarooTwelve() : null
                          : null;

	function MASK_TOP_MASK(length) {
		return (0xFF) >>> (8 - (length))
	};

	function MY_MASK_MASK(n, length) {
		return (MASK_TOP_MASK(length) << ((n) & 0x7)) & 0xFF;
	}
	function MY_GET_MASK(v, n, mask_size) {
		return (v[(n) >> 3] & MY_MASK_MASK(n, mask_size)) >>> (((n)) & 0x7)
	}

	function compute(buf) {

		if( shabuf ) {
			var h = new Array(32)
			shabuf.update(buf).finish(h).clean()
			//console.log( "RESULT HASH?", h );
			return h;
		} else if( k12buf ) {	
			k12buf.update(buf);		
			k12buf.final();
			return k12buf.squeeze( 64 );
		} else  {
			console.log( "no engine for salty generator" );
		}
	}
	var RNG = {
		getSalt: f,
		feed(buf) {
			if( typeof buf === "string" )
				buf = toUTF8Array( buf );
			if( shabuf )
				shabuf.update(buf)
			else
				k12buf.update(buf)
		},
		saltbuf: [],
		entropy: null,
		available: 0,
		used: 0,
		total_bits : 0,
		initialEntropy : "test",
		save() {
			return {
				saltbuf: this.saltbuf.slice(0),
				entropy: this.entropy?this.entropy.slice(0):null,
				available: this.available,
				used: this.used,
				state : shabuf?shabuf.clone():( k12buf ? k12buf.clone():null )
			}
		},
		restore(oldState) {
			this.saltbuf = oldState.saltbuf.slice(0);
			this.entropy = oldState.entropy?oldState.entropy.slice(0):null;
			this.available = oldState.available;
			this.used = oldState.used;
			//throw new Error( "RESTORE STATE IS BROKEN." );
			shabuf && shabuf.copy( oldState.state );
			k12buf && k12buf.copy( oldState.state );
		},
		reset() {
			this.entropy = 
				this.initialEntropy
					?compute(this.initialEntropy)
					:null;
			this.available = 0;
			this.used = 0;
			this.total_bits = 0;
			if( shabuf )
				shabuf.clean();
			if( k12buf ) {
				k12buf.init();
			}
			console
		},
		getByte() {
			if( this.used & 0x7 ) {
				var arr = new Uint8Array(this.getBuffer(8));
				return arr[0];
			} else {
				if(this.available === this.used)
					needBits();
				this.total_bits += 8;
				var result = this.entropy[(this.used) >> 3]
				this.used += 8;
				return result;
			}
		},
		getBits(count, signed) {
			if( !count ) { count = 32; signed = true } 
			if (count > 32)
				throw "Use getBuffer for more than 32 bits.";
			var tmp = this.getBuffer(count);
			if( signed ) {
				var arr_u = new Uint32Array(tmp);
				var arr = new Int32Array(tmp);
				if(  arr_u[0] & ( 1 << (count-1) ) ) {
					var negone = ~0;
					negone <<= (count-1);
					//console.log( "set arr_u = ", arr_u[0].toString(16 ) , negone.toString(16 ) );
					arr_u[0] |= negone;
				}
				return arr[0];
			}
			else {
				var arr = new Uint32Array(tmp);
				return arr[0];
			}
		},
		getBuffer(bits) {
			let _bits = bits;
			let resultIndex = 0;
			let resultBits = 0;
			let resultBuffer = new ArrayBuffer(4 * ((bits + 31) >> 5));
			let result = new Uint8Array(resultBuffer);
			this.total_bits += bits;
			{
				let tmp;
				let partial_tmp;
				let partial_bits = 0;
				let get_bits;

				do {
					if (bits > 8)
						get_bits = 8;
					else
						get_bits = bits;
					// if there were 1-7 bits of data in partial, then can only get 8-partial max.
					if( (8-partial_bits) < get_bits )
						get_bits = (8-partial_bits);
					// if get_bits == 8
					//    but bits_used is 1-7, then it would have to pull 2 bytes to get the 8 required
					//    so truncate get_bits to 1-7 bits
					let chunk = ( 8 - ( this.used & 7) );
					if( chunk < get_bits )
						get_bits = chunk;
					// if resultBits is 1-7 offset, then would have to store up to 2 bytes of value
					//    so have to truncate to just the up to 1 bytes that will fit.
					chunk = ( 8 - ( resultBits & 7) );
					if( chunk < get_bits )
						get_bits = chunk;

					//console.log( "Get bits:", get_bits, " after", this.used, "into", resultBits );
					// only greater... if equal just grab the bits.
					if (get_bits > (this.available - this.used)) {
						if (this.available - this.used) {
							partial_bits = this.available - this.used;
							// partial can never be greater than 8; request is never greater than 8
							//if (partial_bits > 8)
							//	partial_bits = 8;
							partial_tmp = MY_GET_MASK(this.entropy, this.used, partial_bits);
						}
						needBits();
						bits -= partial_bits;
					}
					else {
						tmp = MY_GET_MASK(this.entropy, this.used, get_bits);
						this.used += get_bits;
						if (partial_bits) {
							tmp = partial_tmp | (tmp << partial_bits);
							partial_bits = 0;
						}
						
						result[resultIndex] |= tmp << (resultBits&7);
						resultBits += get_bits;
						// because of input limits, total result bits can only be 8 or less.
						if( resultBits == 8 ) {
							resultIndex++;
							resultBits = 0;
						}
						bits -= get_bits;
					}
				} while (bits);
				//console.log( "output is ", result[0].toString(16), result[1].toString(16), result[2].toString(16), result[3].toString(16) )
				return resultBuffer;
			}
		}
	}
	function needBits() {
		RNG.saltbuf.length = 0;
		if( k12buf ) {
			if( !k12buf.phase() )
				console.trace( "PLEASE INIT THIS USAGE!" );
			//console.log( "BUF IS:", k12buf.absorbing()?"absorbing":"??", k12buf.squeezing()?"squeezing":"!!", k12buf.phase(),( k12buf.absorbing() || ( RNG.total_bits > K12_SQUEEZE_LENGTH ) ) )
			if( k12buf.absorbing() || ( RNG.total_bits >= K12_SQUEEZE_LENGTH ) ) {
				if( k12buf.squeezing() ) {
	                                //console.log( "Need to init with new entropy (BIT FORCE)" );
					k12buf.init();
					k12buf.update( RNG.entropy );
				}
				if (typeof (RNG.getSalt) === 'function') {
					RNG.getSalt(RNG.saltbuf);
					//console.log( "saltbuf.join = ", RNG.saltbuf.join(), RNG.saltbuf.length );
					var newbuf;
					if( RNG.saltbuf.length )
						k12buf.update( RNG.saltbuf );

				}
				k12buf.final();
				RNG.used = 0;
			}
			if( k12buf.squeezing() ) {
				RNG.entropy = k12buf.squeeze(64); // customization is a final pad string.
			}
		}
		if( shabuf ) {
			if (typeof (RNG.getSalt) === 'function')
				RNG.getSalt(RNG.saltbuf);
			//console.log( "saltbuf.join = ", RNG.saltbuf.join(), RNG.saltbuf.length );
			var newbuf;
			if( RNG.saltbuf.length ) {
				if( !RNG.entropy )
					RNG.entropy = new Uint8Array(32)
				newbuf = toUTF8Array( RNG.saltbuf.join() );
	                        if(compat) newbuf = RNG.entropy.concat( newbuf ); 
				shabuf.update(newbuf).finish(RNG.entropy).clean();
				if(!compat) shabuf.update(RNG.entropy);
			}
			else {
				if( !RNG.entropy )
					RNG.entropy = new Uint8Array(32)
				shabuf.finish(RNG.entropy).clean();
				shabuf.update(RNG.entropy);
			}
		}
		RNG.available = RNG.entropy.length * 8;
		RNG.used = 0;
	};
	RNG.reset();
	return RNG;
}

//------------------ SHA256 support

/* Taken from https://github.com/brillout/forge-sha256
 * which itself is taken from https://github.com/digitalbazaar/forge/tree/3b7826f7c2735c42b41b7ceaaadaad570e92d898
 */

// this is just the working bits of the above.

var K = new Uint32Array([
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
	0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
	0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
	0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
	0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
	0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
	0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
	0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
	0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
	0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
])

function blocks(w, v, p, pos, len) {
	var a, b, c, d, e, f, g, h, u, i, j, t1, t2
	while (len >= 64) {
		a = v[0]
		b = v[1]
		c = v[2]
		d = v[3]
		e = v[4]
		f = v[5]
		g = v[6]
		h = v[7]

		for (i = 0; i < 16; i++) {
			j = pos + i * 4
			w[i] = (((p[j] & 0xff) << 24) | ((p[j + 1] & 0xff) << 16) |
				((p[j + 2] & 0xff) << 8) | (p[j + 3] & 0xff))
		}

		for (i = 16; i < 64; i++) {
			u = w[i - 2]
			t1 = (u >>> 17 | u << (32 - 17)) ^ (u >>> 19 | u << (32 - 19)) ^ (u >>> 10)

			u = w[i - 15]
			t2 = (u >>> 7 | u << (32 - 7)) ^ (u >>> 18 | u << (32 - 18)) ^ (u >>> 3)

			w[i] = (t1 + w[i - 7] | 0) + (t2 + w[i - 16] | 0)
		}

		for (i = 0; i < 64; i++) {
			t1 = (((((e >>> 6 | e << (32 - 6)) ^ (e >>> 11 | e << (32 - 11)) ^
				(e >>> 25 | e << (32 - 25))) + ((e & f) ^ (~e & g))) | 0) +
				((h + ((K[i] + w[i]) | 0)) | 0)) | 0

			t2 = (((a >>> 2 | a << (32 - 2)) ^ (a >>> 13 | a << (32 - 13)) ^
				(a >>> 22 | a << (32 - 22))) + ((a & b) ^ (a & c) ^ (b & c))) | 0

			h = g
			g = f
			f = e
			e = (d + t1) | 0
			d = c
			c = b
			b = a
			a = (t1 + t2) | 0
		}

		v[0] += a
		v[1] += b
		v[2] += c
		v[3] += d
		v[4] += e
		v[5] += f
		v[6] += g
		v[7] += h

		pos += 64
		len -= 64
	}
	return pos
}

function SHA256() {
	if( !(this instanceof SHA256) ) return new SHA256();
	this.v = new Uint32Array(8)
	this.w = new Int32Array(64)
	this.buf = new Uint8Array(128)
	this.buflen = 0
	this.len = 0
	this.reset()
}

SHA256.prototype.clone = function (){
	var x = new SHA256();
	x.v = this.v.slice(0);
	x.w = this.w.slice(0);
	x.buf = this.buf.slice(0);
	x.buflen = this.buflen;
	x.len = this.len;
	return x;
}

SHA256.prototype.copy = function (from){

	this.v = from.v;
	this.w = from.w;
	this.buf = from.buf;
	this.buflen = from.buflen;
	this.len = from.len;
	return this;
}

SHA256.prototype.reset = function () {
	this.v[0] = 0x6a09e667
	this.v[1] = 0xbb67ae85
	this.v[2] = 0x3c6ef372
	this.v[3] = 0xa54ff53a
	this.v[4] = 0x510e527f
	this.v[5] = 0x9b05688c
	this.v[6] = 0x1f83d9ab
	this.v[7] = 0x5be0cd19
	this.buflen = 0
	this.len = 0
}

SHA256.prototype.clean = function () {
	var i
	for (i = 0; i < this.buf.length; i++) this.buf[i] = 0
	for (i = 0; i < this.w.length; i++) this.w[i] = 0
	this.reset()
}

SHA256.prototype.update = function (m, len) {
	var mpos = 0, mlen = (typeof len !== 'undefined') ? len : m.length
	this.len += mlen
	if (this.buflen > 0) {
		while (this.buflen < 64 && mlen > 0) {
			this.buf[this.buflen++] = m[mpos++]
			mlen--
		}
		if (this.buflen === 64) {
			blocks(this.w, this.v, this.buf, 0, 64)
			this.buflen = 0
		}
	}
	if (mlen >= 64) {
		mpos = blocks(this.w, this.v, m, mpos, mlen)
		mlen %= 64
		for( var buf_fill = mlen; buf_fill < 64; buf_fill++ )
			this.buf[buf_fill] = m[mpos-64 + buf_fill];
	}
	while (mlen > 0) {
		this.buf[this.buflen++] = m[mpos++]
		mlen--
	}
	return this
}

SHA256.prototype.finish = function (h) {
	var mlen = this.len,
		left = this.buflen,
		bhi = (mlen / 0x20000000) | 0,
		blo = mlen << 3,
		padlen = (mlen % 64 < 56) ? 64 : 128,
		i

	this.buf[left] = 0x80
	for (i = left + 1; i < padlen - 8; i++) this.buf[i] = 0
	this.buf[padlen - 8] = (bhi >>> 24) & 0xff
	this.buf[padlen - 7] = (bhi >>> 16) & 0xff
	this.buf[padlen - 6] = (bhi >>> 8) & 0xff
	this.buf[padlen - 5] = (bhi >>> 0) & 0xff
	this.buf[padlen - 4] = (blo >>> 24) & 0xff
	this.buf[padlen - 3] = (blo >>> 16) & 0xff
	this.buf[padlen - 2] = (blo >>> 8) & 0xff
	this.buf[padlen - 1] = (blo >>> 0) & 0xff

	blocks(this.w, this.v, this.buf, 0, padlen)

	for (i = 0; i < 8; i++) {
		h[i * 4 + 0] = (this.v[i] >>> 24) & 0xff
		h[i * 4 + 1] = (this.v[i] >>> 16) & 0xff
		h[i * 4 + 2] = (this.v[i] >>> 8) & 0xff
		h[i * 4 + 3] = (this.v[i] >>> 0) & 0xff
	}
	if( false )
	{
		var str = '';
		for( i = 0; i < 32; i++ )
			str += h[i].toString(16);
		console.log( "out:", str );
	}

	return this
}

function toUTF8Array(str) {
    var utf8 = [];
    for (var i=0; i < str.length; i++) {
        var charcode = str.charCodeAt(i);
        if (charcode < 0x80) utf8.push(charcode);
        else if (charcode < 0x800) {
            utf8.push(0xc0 | (charcode >> 6),
                      0x80 | (charcode & 0x3f));
        }
        else if (charcode < 0xd800 || charcode >= 0xe000) {
            utf8.push(0xe0 | (charcode >> 12),
                      0x80 | ((charcode>>6) & 0x3f),
                      0x80 | (charcode & 0x3f));
        }
        // surrogate pair
        else {
            i++;
            // UTF-16 encodes 0x10000-0x10FFFF by
            // subtracting 0x10000 and splitting the
            // 20 bits of 0x0-0xFFFFF into two halves
            charcode = 0x10000 + (((charcode & 0x3ff)<<10)
                      | (str.charCodeAt(i) & 0x3ff));
            utf8.push(0xf0 | (charcode >>18),
                      0x80 | ((charcode>>12) & 0x3f),
                      0x80 | ((charcode>>6) & 0x3f),
                      0x80 | (charcode & 0x3f));
        }
    }
    return utf8;
}



const k12Module = {};
var k12 = (function(module){
var Module=typeof Module!=="undefined"?Module:{};var moduleOverrides={};var key;for(key in Module){if(Module.hasOwnProperty(key)){moduleOverrides[key]=Module[key]}}Module["arguments"]=[];Module["thisProgram"]="./this.program";Module["quit"]=(function(status,toThrow){throw toThrow});Module["preRun"]=[];Module["postRun"]=[];var ENVIRONMENT_IS_WEB=false;var ENVIRONMENT_IS_WORKER=false;var ENVIRONMENT_IS_NODE=false;var ENVIRONMENT_IS_SHELL=false;if(Module["ENVIRONMENT"]){if(Module["ENVIRONMENT"]==="WEB"){ENVIRONMENT_IS_WEB=true}else if(Module["ENVIRONMENT"]==="WORKER"){ENVIRONMENT_IS_WORKER=true}else if(Module["ENVIRONMENT"]==="NODE"){ENVIRONMENT_IS_NODE=true}else if(Module["ENVIRONMENT"]==="SHELL"){ENVIRONMENT_IS_SHELL=true}else{throw new Error("Module['ENVIRONMENT'] value is not valid. must be one of: WEB|WORKER|NODE|SHELL.")}}else{ENVIRONMENT_IS_WEB=typeof window==="object";ENVIRONMENT_IS_WORKER=typeof importScripts==="function";ENVIRONMENT_IS_NODE=typeof process==="object"&&typeof require==="function"&&!ENVIRONMENT_IS_WEB&&!ENVIRONMENT_IS_WORKER;ENVIRONMENT_IS_SHELL=!ENVIRONMENT_IS_WEB&&!ENVIRONMENT_IS_NODE&&!ENVIRONMENT_IS_WORKER}
	if(ENVIRONMENT_IS_NODE){var nodeFS;var nodePath;Module["read"]=function shell_read(filename,binary){var ret;ret=tryParseAsDataURI(filename);if(!ret){if(!nodeFS)nodeFS=require("fs");if(!nodePath)nodePath=require("path");filename=nodePath["normalize"](filename);ret=nodeFS["readFileSync"](filename)}return binary?ret:ret.toString()};Module["readBinary"]=function readBinary(filename){var ret=Module["read"](filename,true);if(!ret.buffer){ret=new Uint8Array(ret)}assert(ret.buffer);return ret};if(process["argv"].length>1){Module["thisProgram"]=process["argv"][1].replace(/\\/g,"/")}Module["arguments"]=process["argv"].slice(2);
		if(typeof module!=="undefined"){module["exports"]=Module}/*process["on"]("uncaughtException",(function(ex){if(!(ex instanceof ExitStatus)){throw ex}}));*/process["on"]("unhandledRejection",(function(reason,p){process["exit"](1)}));Module["inspect"]=(function(){return"[Emscripten Module object]"})}
	else if(ENVIRONMENT_IS_SHELL){if(typeof read!="undefined"){Module["read"]=function shell_read(f){var data=tryParseAsDataURI(f);if(data){return intArrayToString(data)}return read(f)}}Module["readBinary"]=function readBinary(f){var data;data=tryParseAsDataURI(f);if(data){return data}if(typeof readbuffer==="function"){return new Uint8Array(readbuffer(f))}data=read(f,"binary");assert(typeof data==="object");return data};if(typeof scriptArgs!="undefined"){Module["arguments"]=scriptArgs}else if(typeof arguments!="undefined"){Module["arguments"]=arguments}if(typeof quit==="function"){Module["quit"]=(function(status,toThrow){quit(status)})}}else 
	if(ENVIRONMENT_IS_WEB||ENVIRONMENT_IS_WORKER){module["exports"]=Module;Module["read"]=function shell_read(url){try{var xhr=new XMLHttpRequest;xhr.open("GET",url,false);xhr.send(null);return xhr.responseText}catch(err){var data=tryParseAsDataURI(url);if(data){return intArrayToString(data)}throw err}};
	if(ENVIRONMENT_IS_WORKER){Module["readBinary"]=function readBinary(url){try{var xhr=new XMLHttpRequest;xhr.open("GET",url,false);xhr.responseType="arraybuffer";xhr.send(null);return new Uint8Array(xhr.response)}catch(err){var data=tryParseAsDataURI(url);if(data){return data}throw err}}}Module["readAsync"]=function readAsync(url,onload,onerror){var xhr=new XMLHttpRequest;xhr.open("GET",url,true);xhr.responseType="arraybuffer";xhr.onload=function xhr_onload(){if(xhr.status==200||xhr.status==0&&xhr.response){onload(xhr.response);return}var data=tryParseAsDataURI(url);if(data){onload(data.buffer);return}onerror()};xhr.onerror=onerror;xhr.send(null)};Module["setWindowTitle"]=(function(title){document.title=title})}Module["print"]=typeof console!=="undefined"?console.log.bind(console):typeof print!=="undefined"?print:null;Module["printErr"]=typeof printErr!=="undefined"?printErr:typeof console!=="undefined"&&console.warn.bind(console)||Module["print"];Module.print=Module["print"];Module.printErr=Module["printErr"];for(key in moduleOverrides){if(moduleOverrides.hasOwnProperty(key)){Module[key]=moduleOverrides[key]}}moduleOverrides=undefined;var STACK_ALIGN=16;function staticAlloc(size){assert(!staticSealed);var ret=STATICTOP;STATICTOP=STATICTOP+size+15&-16;return ret}function dynamicAlloc(size){assert(DYNAMICTOP_PTR);var ret=HEAP32[DYNAMICTOP_PTR>>2];var end=ret+size+15&-16;HEAP32[DYNAMICTOP_PTR>>2]=end;if(end>=TOTAL_MEMORY){var success=enlargeMemory();if(!success){HEAP32[DYNAMICTOP_PTR>>2]=ret;return 0}}return ret}function alignMemory(size,factor){if(!factor)factor=STACK_ALIGN;var ret=size=Math.ceil(size/factor)*factor;return ret}function getNativeTypeSize(type){switch(type){case"i1":case"i8":return 1;case"i16":return 2;case"i32":return 4;case"i64":return 8;case"float":return 4;case"double":return 8;default:{if(type[type.length-1]==="*"){return 4}else if(type[0]==="i"){var bits=parseInt(type.substr(1));assert(bits%8===0);return bits/8}else{return 0}}}}function warnOnce(text){if(!warnOnce.shown)warnOnce.shown={};if(!warnOnce.shown[text]){warnOnce.shown[text]=1;Module.printErr(text)}}var jsCallStartIndex=1;var functionPointers=new Array(0);var funcWrappers={};function dynCall(sig,ptr,args){if(args&&args.length){return Module["dynCall_"+sig].apply(null,[ptr].concat(args))}else{return Module["dynCall_"+sig].call(null,ptr)}}var GLOBAL_BASE=8;var ABORT=0;var EXITSTATUS=0;function assert(condition,text){if(!condition){abort("Assertion failed: "+text)}}function getCFunc(ident){var func=Module["_"+ident];assert(func,"Cannot call unknown function "+ident+", make sure it is exported");return func}var JSfuncs={"stackSave":(function(){stackSave()}),"stackRestore":(function(){stackRestore()}),"arrayToC":(function(arr){var ret=stackAlloc(arr.length);writeArrayToMemory(arr,ret);return ret}),"stringToC":(function(str){var ret=0;if(str!==null&&str!==undefined&&str!==0){var len=(str.length<<2)+1;ret=stackAlloc(len);stringToUTF8(str,ret,len)}return ret})};var toC={"string":JSfuncs["stringToC"],"array":JSfuncs["arrayToC"]};function ccall(ident,returnType,argTypes,args,opts){var func=getCFunc(ident);var cArgs=[];var stack=0;if(args){for(var i=0;i<args.length;i++){var converter=toC[argTypes[i]];if(converter){if(stack===0)stack=stackSave();cArgs[i]=converter(args[i])}else{cArgs[i]=args[i]}}}var ret=func.apply(null,cArgs);if(returnType==="string")ret=Pointer_stringify(ret);else if(returnType==="boolean")ret=Boolean(ret);if(stack!==0){stackRestore(stack)}return ret}function setValue(ptr,value,type,noSafe){type=type||"i8";if(type.charAt(type.length-1)==="*")type="i32";switch(type){case"i1":HEAP8[ptr>>0]=value;break;case"i8":HEAP8[ptr>>0]=value;break;case"i16":HEAP16[ptr>>1]=value;break;case"i32":HEAP32[ptr>>2]=value;break;case"i64":tempI64=[value>>>0,(tempDouble=value,+Math_abs(tempDouble)>=+1?tempDouble>+0?(Math_min(+Math_floor(tempDouble/+4294967296),+4294967295)|0)>>>0:~~+Math_ceil((tempDouble- +(~~tempDouble>>>0))/+4294967296)>>>0:0)],HEAP32[ptr>>2]=tempI64[0],HEAP32[ptr+4>>2]=tempI64[1];break;case"float":HEAPF32[ptr>>2]=value;break;case"double":HEAPF64[ptr>>3]=value;break;default:abort("invalid type for setValue: "+type)}}var ALLOC_STATIC=2;var ALLOC_NONE=4;function allocate(slab,types,allocator,ptr){var zeroinit,size;if(typeof slab==="number"){zeroinit=true;size=slab}else{zeroinit=false;size=slab.length}var singleType=typeof types==="string"?types:null;var ret;if(allocator==ALLOC_NONE){ret=ptr}else{ret=[typeof _malloc==="function"?_malloc:staticAlloc,stackAlloc,staticAlloc,dynamicAlloc][allocator===undefined?ALLOC_STATIC:allocator](Math.max(size,singleType?1:types.length))}if(zeroinit){var stop;ptr=ret;assert((ret&3)==0);stop=ret+(size&~3);for(;ptr<stop;ptr+=4){HEAP32[ptr>>2]=0}stop=ret+size;while(ptr<stop){HEAP8[ptr++>>0]=0}return ret}if(singleType==="i8"){if(slab.subarray||slab.slice){HEAPU8.set(slab,ret)}else{HEAPU8.set(new Uint8Array(slab),ret)}return ret}var i=0,type,typeSize,previousType;while(i<size){var curr=slab[i];type=singleType||types[i];if(type===0){i++;continue}if(type=="i64")type="i32";setValue(ret+i,curr,type);if(previousType!==type){typeSize=getNativeTypeSize(type);previousType=type}i+=typeSize}return ret}function Pointer_stringify(ptr,length){if(length===0||!ptr)return"";var hasUtf=0;var t;var i=0;while(1){t=HEAPU8[ptr+i>>0];hasUtf|=t;if(t==0&&!length)break;i++;if(length&&i==length)break}if(!length)length=i;var ret="";if(hasUtf<128){var MAX_CHUNK=1024;var curr;while(length>0){curr=String.fromCharCode.apply(String,HEAPU8.subarray(ptr,ptr+Math.min(length,MAX_CHUNK)));ret=ret?ret+curr:curr;ptr+=MAX_CHUNK;length-=MAX_CHUNK}return ret}return UTF8ToString(ptr)}var UTF8Decoder=typeof TextDecoder!=="undefined"?new TextDecoder("utf8"):undefined;function UTF8ArrayToString(u8Array,idx){var endPtr=idx;while(u8Array[endPtr])++endPtr;if(endPtr-idx>16&&u8Array.subarray&&UTF8Decoder){return UTF8Decoder.decode(u8Array.subarray(idx,endPtr))}else{var u0,u1,u2,u3,u4,u5;var str="";while(1){u0=u8Array[idx++];if(!u0)return str;if(!(u0&128)){str+=String.fromCharCode(u0);continue}u1=u8Array[idx++]&63;if((u0&224)==192){str+=String.fromCharCode((u0&31)<<6|u1);continue}u2=u8Array[idx++]&63;if((u0&240)==224){u0=(u0&15)<<12|u1<<6|u2}else{u3=u8Array[idx++]&63;if((u0&248)==240){u0=(u0&7)<<18|u1<<12|u2<<6|u3}else{u4=u8Array[idx++]&63;if((u0&252)==248){u0=(u0&3)<<24|u1<<18|u2<<12|u3<<6|u4}else{u5=u8Array[idx++]&63;u0=(u0&1)<<30|u1<<24|u2<<18|u3<<12|u4<<6|u5}}}if(u0<65536){str+=String.fromCharCode(u0)}else{var ch=u0-65536;str+=String.fromCharCode(55296|ch>>10,56320|ch&1023)}}}}function UTF8ToString(ptr){return UTF8ArrayToString(HEAPU8,ptr)}function stringToUTF8Array(str,outU8Array,outIdx,maxBytesToWrite){if(!(maxBytesToWrite>0))return 0;var startIdx=outIdx;var endIdx=outIdx+maxBytesToWrite-1;for(var i=0;i<str.length;++i){var u=str.charCodeAt(i);if(u>=55296&&u<=57343)u=65536+((u&1023)<<10)|str.charCodeAt(++i)&1023;if(u<=127){if(outIdx>=endIdx)break;outU8Array[outIdx++]=u}else if(u<=2047){if(outIdx+1>=endIdx)break;outU8Array[outIdx++]=192|u>>6;outU8Array[outIdx++]=128|u&63}else if(u<=65535){if(outIdx+2>=endIdx)break;outU8Array[outIdx++]=224|u>>12;outU8Array[outIdx++]=128|u>>6&63;outU8Array[outIdx++]=128|u&63}else if(u<=2097151){if(outIdx+3>=endIdx)break;outU8Array[outIdx++]=240|u>>18;outU8Array[outIdx++]=128|u>>12&63;outU8Array[outIdx++]=128|u>>6&63;outU8Array[outIdx++]=128|u&63}else if(u<=67108863){if(outIdx+4>=endIdx)break;outU8Array[outIdx++]=248|u>>24;outU8Array[outIdx++]=128|u>>18&63;outU8Array[outIdx++]=128|u>>12&63;outU8Array[outIdx++]=128|u>>6&63;outU8Array[outIdx++]=128|u&63}else{if(outIdx+5>=endIdx)break;outU8Array[outIdx++]=252|u>>30;outU8Array[outIdx++]=128|u>>24&63;outU8Array[outIdx++]=128|u>>18&63;outU8Array[outIdx++]=128|u>>12&63;outU8Array[outIdx++]=128|u>>6&63;outU8Array[outIdx++]=128|u&63}}outU8Array[outIdx]=0;return outIdx-startIdx}function stringToUTF8(str,outPtr,maxBytesToWrite){return stringToUTF8Array(str,HEAPU8,outPtr,maxBytesToWrite)}function lengthBytesUTF8(str){var len=0;for(var i=0;i<str.length;++i){var u=str.charCodeAt(i);if(u>=55296&&u<=57343)u=65536+((u&1023)<<10)|str.charCodeAt(++i)&1023;if(u<=127){++len}else if(u<=2047){len+=2}else if(u<=65535){len+=3}else if(u<=2097151){len+=4}else if(u<=67108863){len+=5}else{len+=6}}return len}var UTF16Decoder=typeof TextDecoder!=="undefined"?new TextDecoder("utf-16le"):undefined;function demangle(func){return func}function demangleAll(text){var regex=/__Z[\w\d_]+/g;return text.replace(regex,(function(x){var y=demangle(x);return x===y?x:x+" ["+y+"]"}))}function jsStackTrace(){var err=new Error;if(!err.stack){try{throw new Error(0)}catch(e){err=e}if(!err.stack){return"(no stack trace available)"}}return err.stack.toString()}var buffer,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAPF32,HEAPF64;function updateGlobalBufferViews(){Module["HEAP8"]=HEAP8=new Int8Array(buffer);Module["HEAP16"]=HEAP16=new Int16Array(buffer);Module["HEAP32"]=HEAP32=new Int32Array(buffer);Module["HEAPU8"]=HEAPU8=new Uint8Array(buffer);Module["HEAPU16"]=HEAPU16=new Uint16Array(buffer);Module["HEAPU32"]=HEAPU32=new Uint32Array(buffer);Module["HEAPF32"]=HEAPF32=new Float32Array(buffer);Module["HEAPF64"]=HEAPF64=new Float64Array(buffer)}var STATIC_BASE,STATICTOP,staticSealed;var STACK_BASE,STACKTOP,STACK_MAX;var DYNAMIC_BASE,DYNAMICTOP_PTR;STATIC_BASE=STATICTOP=STACK_BASE=STACKTOP=STACK_MAX=DYNAMIC_BASE=DYNAMICTOP_PTR=0;staticSealed=false;function abortOnCannotGrowMemory(){abort("Cannot enlarge memory arrays. Either (1) compile with  -s TOTAL_MEMORY=X  with X higher than the current value "+TOTAL_MEMORY+", (2) compile with  -s ALLOW_MEMORY_GROWTH=1  which allows increasing the size at runtime but prevents some optimizations, (3) set Module.TOTAL_MEMORY to a higher value before the program runs, or (4) if you want malloc to return NULL (0) instead of this abort, compile with  -s ABORTING_MALLOC=0 ")}function enlargeMemory(){abortOnCannotGrowMemory()}var TOTAL_STACK=Module["TOTAL_STACK"]||5242880;var TOTAL_MEMORY=Module["TOTAL_MEMORY"]||16777216;if(TOTAL_MEMORY<TOTAL_STACK)Module.printErr("TOTAL_MEMORY should be larger than TOTAL_STACK, was "+TOTAL_MEMORY+"! (TOTAL_STACK="+TOTAL_STACK+")");if(Module["buffer"]){buffer=Module["buffer"]}else{{buffer=new ArrayBuffer(TOTAL_MEMORY)}Module["buffer"]=buffer}updateGlobalBufferViews();function getTotalMemory(){return TOTAL_MEMORY}HEAP32[0]=1668509029;HEAP16[1]=25459;if(HEAPU8[2]!==115||HEAPU8[3]!==99)throw"Runtime error: expected the system to be little-endian!";function callRuntimeCallbacks(callbacks){while(callbacks.length>0){var callback=callbacks.shift();if(typeof callback=="function"){callback();continue}var func=callback.func;if(typeof func==="number"){if(callback.arg===undefined){Module["dynCall_v"](func)}else{Module["dynCall_vi"](func,callback.arg)}}else{func(callback.arg===undefined?null:callback.arg)}}}var __ATPRERUN__=[];var __ATINIT__=[];var __ATMAIN__=[];var __ATEXIT__=[];var __ATPOSTRUN__=[];var runtimeInitialized=false;var runtimeExited=false;function preRun(){if(Module["preRun"]){if(typeof Module["preRun"]=="function")Module["preRun"]=[Module["preRun"]];while(Module["preRun"].length){addOnPreRun(Module["preRun"].shift())}}callRuntimeCallbacks(__ATPRERUN__)}function ensureInitRuntime(){if(runtimeInitialized)return;runtimeInitialized=true;callRuntimeCallbacks(__ATINIT__)}function preMain(){callRuntimeCallbacks(__ATMAIN__)}function exitRuntime(){callRuntimeCallbacks(__ATEXIT__);runtimeExited=true}function postRun(){if(Module["postRun"]){if(typeof Module["postRun"]=="function")Module["postRun"]=[Module["postRun"]];while(Module["postRun"].length){addOnPostRun(Module["postRun"].shift())}}callRuntimeCallbacks(__ATPOSTRUN__)}function addOnPreRun(cb){__ATPRERUN__.unshift(cb)}function addOnPostRun(cb){__ATPOSTRUN__.unshift(cb)}function writeArrayToMemory(array,buffer){HEAP8.set(array,buffer)}function writeAsciiToMemory(str,buffer,dontAddNull){for(var i=0;i<str.length;++i){HEAP8[buffer++>>0]=str.charCodeAt(i)}if(!dontAddNull)HEAP8[buffer>>0]=0}var Math_abs=Math.abs;var Math_cos=Math.cos;var Math_sin=Math.sin;var Math_tan=Math.tan;var Math_acos=Math.acos;var Math_asin=Math.asin;var Math_atan=Math.atan;var Math_atan2=Math.atan2;var Math_exp=Math.exp;var Math_log=Math.log;var Math_sqrt=Math.sqrt;var Math_ceil=Math.ceil;var Math_floor=Math.floor;var Math_pow=Math.pow;var Math_imul=Math.imul;var Math_fround=Math.fround;var Math_round=Math.round;var Math_min=Math.min;var Math_max=Math.max;var Math_clz32=Math.clz32;var Math_trunc=Math.trunc;var runDependencies=0;var runDependencyWatcher=null;var dependenciesFulfilled=null;function addRunDependency(id){runDependencies++;if(Module["monitorRunDependencies"]){Module["monitorRunDependencies"](runDependencies)}}function removeRunDependency(id){runDependencies--;if(Module["monitorRunDependencies"]){Module["monitorRunDependencies"](runDependencies)}if(runDependencies==0){if(runDependencyWatcher!==null){clearInterval(runDependencyWatcher);runDependencyWatcher=null}if(dependenciesFulfilled){var callback=dependenciesFulfilled;dependenciesFulfilled=null;callback()}}}Module["preloadedImages"]={};Module["preloadedAudios"]={};var memoryInitializer=null;var dataURIPrefix="data:application/octet-stream;base64,";function isDataURI(filename){return String.prototype.startsWith?filename.startsWith(dataURIPrefix):filename.indexOf(dataURIPrefix)===0}STATIC_BASE=GLOBAL_BASE;STATICTOP=STATIC_BASE+4608;__ATINIT__.push();memoryInitializer="data:application/octet-stream;base64,AQAAAAAAAAAAAAAAiQAAAAAAAACLAACAAAAAAICAAIABAAAAiwAAAAEAAAAAgAAAAQAAAIiAAIABAAAAggAAgAAAAAALAAAAAAAAAAoAAAABAAAAgoAAAAAAAAADgAAAAQAAAIuAAAABAAAACwAAgAEAAACKAACAAQAAAIEAAIAAAAAAgQAAgAAAAAAIAACAAAAAAIMAAAAAAAAAA4AAgAEAAACIgACAAAAAAIgAAIABAAAAAIAAAAAAAACCgACA/wAAANAAAAAFAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAgAAAAAOAAAABAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAK/////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADcDQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFBIQVNFOiVkABEACgAREREAAAAABQAAAAAAAAkAAAAACwAAAAAAAAAAEQAPChEREQMKBwABEwkLCwAACQYLAAALAAYRAAAAERERAAAAAAAAAAAAAAAAAAAAAAsAAAAAAAAAABEACgoREREACgAAAgAJCwAAAAkACwAACwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMAAAAAAAAAAAAAAAMAAAAAAwAAAAACQwAAAAAAAwAAAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADgAAAAAAAAAAAAAADQAAAAQNAAAAAAkOAAAAAAAOAAAOAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAA8AAAAADwAAAAAJEAAAAAAAEAAAEAAAEgAAABISEgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAASAAAAEhISAAAAAAAACQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACwAAAAAAAAAAAAAACgAAAAAKAAAAAAkLAAAAAAALAAALAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAwAAAAAAAAAAAAAAAwAAAAADAAAAAAJDAAAAAAADAAADAAALSsgICAwWDB4AChudWxsKQAtMFgrMFggMFgtMHgrMHggMHgAaW5mAElORgBuYW4ATkFOADAxMjM0NTY3ODlBQkNERUYuAFQhIhkNAQIDEUscDBAECx0SHidobm9wcWIgBQYPExQVGggWBygkFxgJCg4bHyUjg4J9JiorPD0+P0NHSk1YWVpbXF1eX2BhY2RlZmdpamtscnN0eXp7fABJbGxlZ2FsIGJ5dGUgc2VxdWVuY2UARG9tYWluIGVycm9yAFJlc3VsdCBub3QgcmVwcmVzZW50YWJsZQBOb3QgYSB0dHkAUGVybWlzc2lvbiBkZW5pZWQAT3BlcmF0aW9uIG5vdCBwZXJtaXR0ZWQATm8gc3VjaCBmaWxlIG9yIGRpcmVjdG9yeQBObyBzdWNoIHByb2Nlc3MARmlsZSBleGlzdHMAVmFsdWUgdG9vIGxhcmdlIGZvciBkYXRhIHR5cGUATm8gc3BhY2UgbGVmdCBvbiBkZXZpY2UAT3V0IG9mIG1lbW9yeQBSZXNvdXJjZSBidXN5AEludGVycnVwdGVkIHN5c3RlbSBjYWxsAFJlc291cmNlIHRlbXBvcmFyaWx5IHVuYXZhaWxhYmxlAEludmFsaWQgc2VlawBDcm9zcy1kZXZpY2UgbGluawBSZWFkLW9ubHkgZmlsZSBzeXN0ZW0ARGlyZWN0b3J5IG5vdCBlbXB0eQBDb25uZWN0aW9uIHJlc2V0IGJ5IHBlZXIAT3BlcmF0aW9uIHRpbWVkIG91dABDb25uZWN0aW9uIHJlZnVzZWQASG9zdCBpcyBkb3duAEhvc3QgaXMgdW5yZWFjaGFibGUAQWRkcmVzcyBpbiB1c2UAQnJva2VuIHBpcGUASS9PIGVycm9yAE5vIHN1Y2ggZGV2aWNlIG9yIGFkZHJlc3MAQmxvY2sgZGV2aWNlIHJlcXVpcmVkAE5vIHN1Y2ggZGV2aWNlAE5vdCBhIGRpcmVjdG9yeQBJcyBhIGRpcmVjdG9yeQBUZXh0IGZpbGUgYnVzeQBFeGVjIGZvcm1hdCBlcnJvcgBJbnZhbGlkIGFyZ3VtZW50AEFyZ3VtZW50IGxpc3QgdG9vIGxvbmcAU3ltYm9saWMgbGluayBsb29wAEZpbGVuYW1lIHRvbyBsb25nAFRvbyBtYW55IG9wZW4gZmlsZXMgaW4gc3lzdGVtAE5vIGZpbGUgZGVzY3JpcHRvcnMgYXZhaWxhYmxlAEJhZCBmaWxlIGRlc2NyaXB0b3IATm8gY2hpbGQgcHJvY2VzcwBCYWQgYWRkcmVzcwBGaWxlIHRvbyBsYXJnZQBUb28gbWFueSBsaW5rcwBObyBsb2NrcyBhdmFpbGFibGUAUmVzb3VyY2UgZGVhZGxvY2sgd291bGQgb2NjdXIAU3RhdGUgbm90IHJlY292ZXJhYmxlAFByZXZpb3VzIG93bmVyIGRpZWQAT3BlcmF0aW9uIGNhbmNlbGVkAEZ1bmN0aW9uIG5vdCBpbXBsZW1lbnRlZABObyBtZXNzYWdlIG9mIGRlc2lyZWQgdHlwZQBJZGVudGlmaWVyIHJlbW92ZWQARGV2aWNlIG5vdCBhIHN0cmVhbQBObyBkYXRhIGF2YWlsYWJsZQBEZXZpY2UgdGltZW91dABPdXQgb2Ygc3RyZWFtcyByZXNvdXJjZXMATGluayBoYXMgYmVlbiBzZXZlcmVkAFByb3RvY29sIGVycm9yAEJhZCBtZXNzYWdlAEZpbGUgZGVzY3JpcHRvciBpbiBiYWQgc3RhdGUATm90IGEgc29ja2V0AERlc3RpbmF0aW9uIGFkZHJlc3MgcmVxdWlyZWQATWVzc2FnZSB0b28gbGFyZ2UAUHJvdG9jb2wgd3JvbmcgdHlwZSBmb3Igc29ja2V0AFByb3RvY29sIG5vdCBhdmFpbGFibGUAUHJvdG9jb2wgbm90IHN1cHBvcnRlZABTb2NrZXQgdHlwZSBub3Qgc3VwcG9ydGVkAE5vdCBzdXBwb3J0ZWQAUHJvdG9jb2wgZmFtaWx5IG5vdCBzdXBwb3J0ZWQAQWRkcmVzcyBmYW1pbHkgbm90IHN1cHBvcnRlZCBieSBwcm90b2NvbABBZGRyZXNzIG5vdCBhdmFpbGFibGUATmV0d29yayBpcyBkb3duAE5ldHdvcmsgdW5yZWFjaGFibGUAQ29ubmVjdGlvbiByZXNldCBieSBuZXR3b3JrAENvbm5lY3Rpb24gYWJvcnRlZABObyBidWZmZXIgc3BhY2UgYXZhaWxhYmxlAFNvY2tldCBpcyBjb25uZWN0ZWQAU29ja2V0IG5vdCBjb25uZWN0ZWQAQ2Fubm90IHNlbmQgYWZ0ZXIgc29ja2V0IHNodXRkb3duAE9wZXJhdGlvbiBhbHJlYWR5IGluIHByb2dyZXNzAE9wZXJhdGlvbiBpbiBwcm9ncmVzcwBTdGFsZSBmaWxlIGhhbmRsZQBSZW1vdGUgSS9PIGVycm9yAFF1b3RhIGV4Y2VlZGVkAE5vIG1lZGl1bSBmb3VuZABXcm9uZyBtZWRpdW0gdHlwZQBObyBlcnJvciBpbmZvcm1hdGlvbg==";var tempDoublePtr=STATICTOP;STATICTOP+=16;var SYSCALLS={varargs:0,get:(function(varargs){SYSCALLS.varargs+=4;var ret=HEAP32[SYSCALLS.varargs-4>>2];return ret}),getStr:(function(){var ret=Pointer_stringify(SYSCALLS.get());return ret}),get64:(function(){var low=SYSCALLS.get(),high=SYSCALLS.get();if(low>=0)assert(high===0);else assert(high===-1);return low}),getZero:(function(){assert(SYSCALLS.get()===0)})};function ___syscall140(which,varargs){SYSCALLS.varargs=varargs;try{var stream=SYSCALLS.getStreamFromFD(),offset_high=SYSCALLS.get(),offset_low=SYSCALLS.get(),result=SYSCALLS.get(),whence=SYSCALLS.get();var offset=offset_low;FS.llseek(stream,offset,whence);HEAP32[result>>2]=stream.position;if(stream.getdents&&offset===0&&whence===0)stream.getdents=null;return 0}catch(e){if(typeof FS==="undefined"||!(e instanceof FS.ErrnoError))abort(e);return-e.errno}}function flush_NO_FILESYSTEM(){var fflush=Module["_fflush"];if(fflush)fflush(0);var printChar=___syscall146.printChar;if(!printChar)return;var buffers=___syscall146.buffers;if(buffers[1].length)printChar(1,10);if(buffers[2].length)printChar(2,10)}function ___syscall146(which,varargs){SYSCALLS.varargs=varargs;try{var stream=SYSCALLS.get(),iov=SYSCALLS.get(),iovcnt=SYSCALLS.get();var ret=0;if(!___syscall146.buffers){___syscall146.buffers=[null,[],[]];___syscall146.printChar=(function(stream,curr){var buffer=___syscall146.buffers[stream];assert(buffer);if(curr===0||curr===10){(stream===1?Module["print"]:Module["printErr"])(UTF8ArrayToString(buffer,0));buffer.length=0}else{buffer.push(curr)}})}for(var i=0;i<iovcnt;i++){var ptr=HEAP32[iov+i*8>>2];var len=HEAP32[iov+(i*8+4)>>2];for(var j=0;j<len;j++){___syscall146.printChar(stream,HEAPU8[ptr+j])}ret+=len}return ret}catch(e){if(typeof FS==="undefined"||!(e instanceof FS.ErrnoError))abort(e);return-e.errno}}function ___syscall54(which,varargs){SYSCALLS.varargs=varargs;try{return 0}catch(e){if(typeof FS==="undefined"||!(e instanceof FS.ErrnoError))abort(e);return-e.errno}}function ___syscall6(which,varargs){SYSCALLS.varargs=varargs;try{var stream=SYSCALLS.getStreamFromFD();FS.close(stream);return 0}catch(e){if(typeof FS==="undefined"||!(e instanceof FS.ErrnoError))abort(e);return-e.errno}}var cttz_i8=allocate([8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0],"i8",ALLOC_STATIC);function _emscripten_memcpy_big(dest,src,num){HEAPU8.set(HEAPU8.subarray(src,src+num),dest);return dest}function ___setErrNo(value){if(Module["___errno_location"])HEAP32[Module["___errno_location"]()>>2]=value;return value}DYNAMICTOP_PTR=staticAlloc(4);STACK_BASE=STACKTOP=alignMemory(STATICTOP);STACK_MAX=STACK_BASE+TOTAL_STACK;DYNAMIC_BASE=alignMemory(STACK_MAX);HEAP32[DYNAMICTOP_PTR>>2]=DYNAMIC_BASE;staticSealed=true;var ASSERTIONS=false;function intArrayToString(array){var ret=[];for(var i=0;i<array.length;i++){var chr=array[i];if(chr>255){if(ASSERTIONS){assert(false,"Character code "+chr+" ("+String.fromCharCode(chr)+")  at offset "+i+" not in 0x00-0xFF.")}chr&=255}ret.push(String.fromCharCode(chr))}return ret.join("")}var decodeBase64=typeof atob==="function"?atob:(function(input){var keyStr="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";var output="";var chr1,chr2,chr3;var enc1,enc2,enc3,enc4;var i=0;input=input.replace(/[^A-Za-z0-9\+\/\=]/g,"");do{enc1=keyStr.indexOf(input.charAt(i++));enc2=keyStr.indexOf(input.charAt(i++));enc3=keyStr.indexOf(input.charAt(i++));enc4=keyStr.indexOf(input.charAt(i++));chr1=enc1<<2|enc2>>4;chr2=(enc2&15)<<4|enc3>>2;chr3=(enc3&3)<<6|enc4;output=output+String.fromCharCode(chr1);if(enc3!==64){output=output+String.fromCharCode(chr2)}if(enc4!==64){output=output+String.fromCharCode(chr3)}}while(i<input.length);return output});function intArrayFromBase64(s){
		if(typeof ENVIRONMENT_IS_NODE==="boolean"&&ENVIRONMENT_IS_NODE){var buf;try{buf=Buffer.from(s,"base64")}catch(_){buf=new Buffer(s,"base64")}return new Uint8Array(buf.buffer,buf.byteOffset,buf.byteLength)}try{var decoded=decodeBase64(s);var bytes=new Uint8Array(decoded.length);for(var i=0;i<decoded.length;++i){bytes[i]=decoded.charCodeAt(i)}return bytes}catch(_){throw new Error("Converting base64 string to bytes failed.")}}function tryParseAsDataURI(filename){if(!isDataURI(filename)){return}return intArrayFromBase64(filename.slice(dataURIPrefix.length))}function invoke_ii(index,a1){try{return Module["dynCall_ii"](index,a1)}catch(e){if(typeof e!=="number"&&e!=="longjmp")throw e;Module["setThrew"](1,0)}}function invoke_iiii(index,a1,a2,a3){try{return Module["dynCall_iiii"](index,a1,a2,a3)}catch(e){if(typeof e!=="number"&&e!=="longjmp")throw e;Module["setThrew"](1,0)}}Module.asmGlobalArg={"Math":Math,"Int8Array":Int8Array,"Int16Array":Int16Array,"Int32Array":Int32Array,"Uint8Array":Uint8Array,"Uint16Array":Uint16Array,"Uint32Array":Uint32Array,"Float32Array":Float32Array,"Float64Array":Float64Array,"NaN":NaN,"Infinity":Infinity};Module.asmLibraryArg={"abort":abort,"assert":assert,"enlargeMemory":enlargeMemory,"getTotalMemory":getTotalMemory,"abortOnCannotGrowMemory":abortOnCannotGrowMemory,"invoke_ii":invoke_ii,"invoke_iiii":invoke_iiii,"___setErrNo":___setErrNo,"___syscall140":___syscall140,"___syscall146":___syscall146,"___syscall54":___syscall54,"___syscall6":___syscall6,"_emscripten_memcpy_big":_emscripten_memcpy_big,"flush_NO_FILESYSTEM":flush_NO_FILESYSTEM,"DYNAMICTOP_PTR":DYNAMICTOP_PTR,"tempDoublePtr":tempDoublePtr,"ABORT":ABORT,"STACKTOP":STACKTOP,"STACK_MAX":STACK_MAX,"cttz_i8":cttz_i8};// EMSCRIPTEN_START_ASM
var asm=(/** @suppress {uselessCode} */ function(global,env,buffer) {
"use asm";var a=new global.Int8Array(buffer);var b=new global.Int16Array(buffer);var c=new global.Int32Array(buffer);var d=new global.Uint8Array(buffer);var e=new global.Uint16Array(buffer);var f=new global.Uint32Array(buffer);var g=new global.Float32Array(buffer);var h=new global.Float64Array(buffer);var i=env.DYNAMICTOP_PTR|0;var j=env.tempDoublePtr|0;var k=env.ABORT|0;var l=env.STACKTOP|0;var m=env.STACK_MAX|0;var n=env.cttz_i8|0;var o=0;var p=0;var q=0;var r=0;var s=global.NaN,t=global.Infinity;var u=0,v=0,w=0,x=0,y=0.0;var z=0;var A=global.Math.floor;var B=global.Math.abs;var C=global.Math.sqrt;var D=global.Math.pow;var E=global.Math.cos;var F=global.Math.sin;var G=global.Math.tan;var H=global.Math.acos;var I=global.Math.asin;var J=global.Math.atan;var K=global.Math.atan2;var L=global.Math.exp;var M=global.Math.log;var N=global.Math.ceil;var O=global.Math.imul;var P=global.Math.min;var Q=global.Math.max;var R=global.Math.clz32;var S=env.abort;var T=env.assert;var U=env.enlargeMemory;var V=env.getTotalMemory;var W=env.abortOnCannotGrowMemory;var X=env.invoke_ii;var Y=env.invoke_iiii;var Z=env.___setErrNo;var _=env.___syscall140;var $=env.___syscall146;var aa=env.___syscall54;var ba=env.___syscall6;var ca=env._emscripten_memcpy_big;var da=env.flush_NO_FILESYSTEM;var ea=0.0;
// EMSCRIPTEN_START_FUNCS
function ha(a){a=a|0;var b=0;b=l;l=l+a|0;l=l+15&-16;return b|0}function ia(){return l|0}function ja(a){a=a|0;l=a}function ka(a,b){a=a|0;b=b|0;l=a;m=b}function la(a,b){a=a|0;b=b|0;if(!o){o=a;p=b}}function ma(a){a=a|0;z=a}function na(){return z|0}function oa(a,b,d){a=a|0;b=b|0;d=d|0;var e=0,f=0,g=0,h=0,i=0;d=d+-1|0;if((d|0)<=-1)return;while(1){i=c[b>>2]|0;h=(i>>>1^i)&572662306;i=h^i;h=i^h<<1;i=(h^i>>>2)&202116108;h=i^h;i=h^i<<2;h=(i^h>>>4)&15728880;i=h^i;h=i^h<<4;i=(h^i>>>8)&65280;g=c[b+4>>2]|0;f=(g>>>1^g)&572662306;g=f^g;f=g^f<<1;g=(f^g>>>2)&202116108;f=g^f;g=f^g<<2;f=(g^f>>>4)&15728880;g=f^g;f=g^f<<4;g=(f^g>>>8)&65280;e=a+4|0;c[a>>2]=((g^f)<<16|i^h&65535)^c[a>>2];c[e>>2]=((i<<8^h)>>>16|g<<8^f&-65536)^c[e>>2];d=d+-1|0;if((d|0)<=-1)break;else{b=b+8|0;a=a+8|0}}return}function pa(a,b,d,e){a=a|0;b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0,p=0;i=l;l=l+16|0;h=i;if(!d){d=e>>>3;oa(a,b,d);f=h;c[f>>2]=0;c[f+4>>2]=0;Bb(h|0,b+(e&-8)|0,e&7|0)|0;e=c[h>>2]|0;b=c[h+4>>2]|0;f=(e>>>1^e)&572662306;e=f^e;f=e^f<<1;e=(f^e>>>2)&202116108;f=e^f;e=f^e<<2;f=(e^f>>>4)&15728880;e=f^e;f=e^f<<4;e=(f^e>>>8)&65280;g=(b>>>1^b)&572662306;b=g^b;g=b^g<<1;b=(g^b>>>2)&202116108;g=b^g;b=g^b<<2;g=(b^g>>>4)&15728880;b=g^b;g=b^g<<4;b=(g^b>>>8)&65280;h=d<<1;d=a+(h<<2)|0;c[d>>2]=((b^g)<<16|e^f&65535)^c[d>>2];h=a+((h|1)<<2)|0;c[h>>2]=((e<<8^f)>>>16|b<<8^g&-65536)^c[h>>2];l=i;return}if(!e){l=i;return}o=d&7;f=d>>>3;g=h+4|0;d=8-o|0;d=d>>>0>e>>>0?e:d;m=h;c[m>>2]=0;c[m+4>>2]=0;Bb(h+o|0,b|0,d|0)|0;o=c[h>>2]|0;m=c[g>>2]|0;n=(o>>>1^o)&572662306;o=n^o;n=o^n<<1;o=(n^o>>>2)&202116108;n=o^n;o=n^o<<2;n=(o^n>>>4)&15728880;o=n^o;n=o^n<<4;o=(n^o>>>8)&65280;k=(m>>>1^m)&572662306;m=k^m;k=m^k<<1;m=(k^m>>>2)&202116108;k=m^k;m=k^m<<2;k=(m^k>>>4)&15728880;m=k^m;k=m^k<<4;m=(k^m>>>8)&65280;j=f<<1;p=a+(j<<2)|0;c[p>>2]=((m^k)<<16|o^n&65535)^c[p>>2];j=a+((j|1)<<2)|0;c[j>>2]=((o<<8^n)>>>16|m<<8^k&-65536)^c[j>>2];e=e-d|0;if(!e){l=i;return}b=b+d|0;while(1){f=f+1|0;d=e>>>0<8?e:8;k=h;c[k>>2]=0;c[k+4>>2]=0;Bb(h|0,b|0,d|0)|0;k=c[h>>2]|0;n=c[g>>2]|0;m=(k>>>1^k)&572662306;k=m^k;m=k^m<<1;k=(m^k>>>2)&202116108;m=k^m;k=m^k<<2;m=(k^m>>>4)&15728880;k=m^k;m=k^m<<4;k=(m^k>>>8)&65280;o=(n>>>1^n)&572662306;n=o^n;o=n^o<<1;n=(o^n>>>2)&202116108;o=n^o;n=o^n<<2;o=(n^o>>>4)&15728880;n=o^n;o=n^o<<4;n=(o^n>>>8)&65280;p=f<<1;j=a+(p<<2)|0;c[j>>2]=((n^o)<<16|k^m&65535)^c[j>>2];p=a+((p|1)<<2)|0;c[p>>2]=((k<<8^m)>>>16|n<<8^o&-65536)^c[p>>2];e=e-d|0;if(!e)break;else b=b+d|0}l=i;return}function qa(a,b,d){a=a|0;b=b|0;d=d|0;var e=0,f=0,g=0,h=0,i=0;d=d+-1|0;if((d|0)<=-1)return;while(1){g=c[a>>2]|0;i=c[a+4>>2]|0;e=g>>>16;h=(i<<8^g)&65280;g=h^(i<<16|g&65535);h=g^h<<8;g=(h^g>>>4)&15728880;h=g^h;g=h^g<<4;h=(g^h>>>2)&202116108;g=h^g;h=g^h<<2;g=(h^g>>>1)&572662306;f=(i>>>8^e)&65280;e=f^(i&-65536|e);f=e^f<<8;e=(f^e>>>4)&15728880;f=e^f;e=f^e<<4;f=(e^f>>>2)&202116108;e=f^e;f=e^f<<2;e=(f^e>>>1)&572662306;c[b>>2]=g^h^g<<1;c[b+4>>2]=e^f^e<<1;d=d+-1|0;if((d|0)<=-1)break;else{b=b+8|0;a=a+8|0}}return}function ra(a,b,d,e){a=a|0;b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0,p=0;i=l;l=l+16|0;h=i;if(!d){j=e>>>3;qa(a,b,j);j=j<<1;f=c[a+(j<<2)>>2]|0;j=c[a+((j|1)<<2)>>2]|0;a=f>>>16;d=(j<<8^f)&65280;f=d^(j<<16|f&65535);d=f^d<<8;f=(d^f>>>4)&15728880;d=f^d;f=d^f<<4;d=(f^d>>>2)&202116108;f=d^f;d=f^d<<2;f=(d^f>>>1)&572662306;g=(j>>>8^a)&65280;a=g^(j&-65536|a);g=a^g<<8;a=(g^a>>>4)&15728880;g=a^g;a=g^a<<4;g=(a^g>>>2)&202116108;a=g^a;g=a^g<<2;a=(g^a>>>1)&572662306;c[h>>2]=f^d^f<<1;c[h+4>>2]=a^g^a<<1;Bb(b+(e&-8)|0,h|0,e&7|0)|0;l=i;return}if(!e){l=i;return}j=d&7;f=d>>>3;g=h+4|0;d=8-j|0;d=d>>>0>e>>>0?e:d;p=f<<1;n=c[a+(p<<2)>>2]|0;p=c[a+((p|1)<<2)>>2]|0;k=n>>>16;o=(p<<8^n)&65280;n=o^(p<<16|n&65535);o=n^o<<8;n=(o^n>>>4)&15728880;o=n^o;n=o^n<<4;o=(n^o>>>2)&202116108;n=o^n;o=n^o<<2;n=(o^n>>>1)&572662306;m=(p>>>8^k)&65280;k=m^(p&-65536|k);m=k^m<<8;k=(m^k>>>4)&15728880;m=k^m;k=m^k<<4;m=(k^m>>>2)&202116108;k=m^k;m=k^m<<2;k=(m^k>>>1)&572662306;c[h>>2]=n^o^n<<1;c[g>>2]=k^m^k<<1;Bb(b|0,h+j|0,d|0)|0;e=e-d|0;if(!e){l=i;return}b=b+d|0;while(1){f=f+1|0;d=e>>>0<8?e:8;k=f<<1;n=c[a+(k<<2)>>2]|0;k=c[a+((k|1)<<2)>>2]|0;p=n>>>16;m=(k<<8^n)&65280;n=m^(k<<16|n&65535);m=n^m<<8;n=(m^n>>>4)&15728880;m=n^m;n=m^n<<4;m=(n^m>>>2)&202116108;n=m^n;m=n^m<<2;n=(m^n>>>1)&572662306;o=(k>>>8^p)&65280;p=o^(k&-65536|p);o=p^o<<8;p=(o^p>>>4)&15728880;o=p^o;p=o^p<<4;o=(p^o>>>2)&202116108;p=o^p;o=p^o<<2;p=(o^p>>>1)&572662306;c[h>>2]=n^m^n<<1;c[g>>2]=p^o^p<<1;Bb(b|0,h|0,d|0)|0;e=e-d|0;if(!e)break;else b=b+d|0}l=i;return}function sa(a,b){a=a|0;b=b|0;var d=0,e=0,f=0,g=0,h=0,i=0,j=0,k=0,l=0,m=0,n=0,o=0,p=0,q=0,r=0,s=0,t=0,u=0,v=0,w=0,x=0,y=0,z=0,A=0,B=0,C=0,D=0,E=0,F=0,G=0,H=0,I=0,J=0,K=0,L=0,M=0,N=0,O=0,P=0,Q=0,R=0,S=0,T=0,U=0,V=0,W=0,X=0,Y=0,Z=0,_=0,$=0,aa=0,ba=0,ca=0,da=0,ea=0,fa=0,ga=0,ha=0,ia=0,ja=0,ka=0,la=0,ma=0,na=0,oa=0,pa=0,qa=0,ra=0,sa=0;g=8+(24-b<<1<<2)|0;f=b&255;switch(f&3){case 1:{ha=a+40|0;ja=c[ha>>2]|0;ga=a+44|0;b=c[ga>>2]|0;d=a+80|0;ia=a+84|0;e=c[ia>>2]|0;c[ga>>2]=c[d>>2];c[ha>>2]=e;ha=a+160|0;e=a+164|0;ga=c[e>>2]|0;c[d>>2]=c[ha>>2];c[ia>>2]=ga;ia=a+120|0;ga=a+124|0;d=c[ga>>2]|0;c[e>>2]=c[ia>>2];c[ha>>2]=d;c[ia>>2]=ja;c[ga>>2]=b;ga=a+8|0;b=c[ga>>2]|0;ia=a+12|0;ja=c[ia>>2]|0;ha=a+48|0;d=a+52|0;e=c[d>>2]|0;c[ga>>2]=c[ha>>2];c[ia>>2]=e;ia=a+128|0;e=a+132|0;ga=c[e>>2]|0;c[d>>2]=c[ia>>2];c[ha>>2]=ga;ha=a+88|0;ga=a+92|0;d=c[ga>>2]|0;c[ia>>2]=c[ha>>2];c[e>>2]=d;c[ga>>2]=b;c[ha>>2]=ja;ha=a+16|0;ja=c[ha>>2]|0;ga=a+20|0;b=c[ga>>2]|0;e=a+96|0;d=a+100|0;ia=c[d>>2]|0;c[ga>>2]=c[e>>2];c[ha>>2]=ia;ha=a+56|0;ia=a+60|0;ga=c[ia>>2]|0;c[e>>2]=c[ha>>2];c[d>>2]=ga;d=a+176|0;ga=a+180|0;e=c[ga>>2]|0;c[ia>>2]=c[d>>2];c[ha>>2]=e;c[d>>2]=ja;c[ga>>2]=b;ga=a+136|0;b=c[ga>>2]|0;d=a+140|0;c[ga>>2]=c[d>>2];c[d>>2]=b;d=a+24|0;b=c[d>>2]|0;ga=a+28|0;ja=c[ga>>2]|0;ha=a+144|0;e=a+148|0;ia=c[e>>2]|0;c[ga>>2]=c[ha>>2];c[d>>2]=ia;d=a+184|0;ia=a+188|0;ga=c[ia>>2]|0;c[ha>>2]=c[d>>2];c[e>>2]=ga;e=a+64|0;ga=a+68|0;ha=c[ga>>2]|0;c[ia>>2]=c[e>>2];c[d>>2]=ha;c[e>>2]=b;c[ga>>2]=ja;ga=a+104|0;ja=c[ga>>2]|0;e=a+108|0;c[ga>>2]=c[e>>2];c[e>>2]=ja;e=a+32|0;ja=c[e>>2]|0;ga=a+36|0;b=c[ga>>2]|0;d=a+192|0;ha=a+196|0;ia=c[ha>>2]|0;c[e>>2]=c[d>>2];c[ga>>2]=ia;ga=a+112|0;ia=a+116|0;e=c[ia>>2]|0;c[ha>>2]=c[ga>>2];c[d>>2]=e;d=a+152|0;e=a+156|0;ha=c[e>>2]|0;c[ga>>2]=c[d>>2];c[ia>>2]=ha;c[e>>2]=ja;e=0;ja=5;break}case 2:{d=a+40|0;b=c[d>>2]|0;ga=a+44|0;ja=c[ga>>2]|0;ia=a+160|0;e=a+164|0;ha=c[e>>2]|0;c[ga>>2]=c[ia>>2];c[d>>2]=ha;c[e>>2]=b;c[ia>>2]=ja;ia=a+80|0;ja=c[ia>>2]|0;e=a+84|0;b=c[e>>2]|0;d=a+120|0;ha=a+124|0;ga=c[ha>>2]|0;c[e>>2]=c[d>>2];c[ia>>2]=ga;c[ha>>2]=ja;c[d>>2]=b;d=a+8|0;b=c[d>>2]|0;ha=a+12|0;ja=c[ha>>2]|0;ia=a+128|0;ga=a+132|0;e=c[ga>>2]|0;c[ha>>2]=c[ia>>2];c[d>>2]=e;c[ga>>2]=b;c[ia>>2]=ja;ia=a+48|0;ja=c[ia>>2]|0;ga=a+52|0;b=c[ga>>2]|0;d=a+88|0;e=a+92|0;ha=c[e>>2]|0;c[ga>>2]=c[d>>2];c[ia>>2]=ha;c[e>>2]=ja;c[d>>2]=b;d=a+16|0;b=c[d>>2]|0;e=a+20|0;ja=c[e>>2]|0;ia=a+56|0;ha=a+60|0;ga=c[ha>>2]|0;c[e>>2]=c[ia>>2];c[d>>2]=ga;c[ha>>2]=b;c[ia>>2]=ja;ia=a+96|0;ja=c[ia>>2]|0;ha=a+100|0;b=c[ha>>2]|0;d=a+176|0;ga=a+180|0;e=c[ga>>2]|0;c[ha>>2]=c[d>>2];c[ia>>2]=e;c[ga>>2]=ja;c[d>>2]=b;d=a+24|0;b=c[d>>2]|0;ga=a+28|0;ja=c[ga>>2]|0;ia=a+184|0;e=a+188|0;ha=c[e>>2]|0;c[ga>>2]=c[ia>>2];c[d>>2]=ha;c[e>>2]=b;c[ia>>2]=ja;ia=a+64|0;ja=c[ia>>2]|0;e=a+68|0;b=c[e>>2]|0;d=a+144|0;ha=a+148|0;ga=c[ha>>2]|0;c[e>>2]=c[d>>2];c[ia>>2]=ga;c[ha>>2]=ja;c[d>>2]=b;d=a+32|0;b=c[d>>2]|0;ha=a+36|0;ja=c[ha>>2]|0;ia=a+112|0;ga=a+116|0;e=c[ga>>2]|0;c[ha>>2]=c[ia>>2];c[d>>2]=e;c[ga>>2]=b;c[ia>>2]=ja;ia=a+152|0;ja=c[ia>>2]|0;ga=a+156|0;b=c[ga>>2]|0;d=a+192|0;e=a+196|0;ha=c[e>>2]|0;c[ga>>2]=c[d>>2];c[ia>>2]=ha;c[e>>2]=ja;e=0;ja=5;break}case 3:{ja=a+40|0;e=c[ja>>2]|0;ia=a+44|0;b=c[ia>>2]|0;ha=a+120|0;ga=a+124|0;d=c[ga>>2]|0;c[ja>>2]=c[ha>>2];c[ia>>2]=d;ia=a+160|0;d=a+164|0;ja=c[d>>2]|0;c[ga>>2]=c[ia>>2];c[ha>>2]=ja;ha=a+80|0;ja=a+84|0;ga=c[ja>>2]|0;c[ia>>2]=c[ha>>2];c[d>>2]=ga;c[ja>>2]=e;c[ha>>2]=b;ha=a+8|0;b=c[ha>>2]|0;ja=a+12|0;e=c[ja>>2]|0;d=a+88|0;ga=a+92|0;ia=c[ga>>2]|0;c[ja>>2]=c[d>>2];c[ha>>2]=ia;ha=a+128|0;ia=a+132|0;ja=c[ia>>2]|0;c[d>>2]=c[ha>>2];c[ga>>2]=ja;ga=a+48|0;ja=a+52|0;d=c[ja>>2]|0;c[ia>>2]=c[ga>>2];c[ha>>2]=d;c[ga>>2]=b;c[ja>>2]=e;ja=a+16|0;e=c[ja>>2]|0;ga=a+20|0;b=c[ga>>2]|0;ha=a+176|0;d=a+180|0;ia=c[d>>2]|0;c[ja>>2]=c[ha>>2];c[ga>>2]=ia;ga=a+56|0;ia=a+60|0;ja=c[ia>>2]|0;c[d>>2]=c[ga>>2];c[ha>>2]=ja;ha=a+96|0;ja=a+100|0;d=c[ja>>2]|0;c[ga>>2]=c[ha>>2];c[ia>>2]=d;c[ja>>2]=e;c[ha>>2]=b;ha=a+136|0;b=c[ha>>2]|0;ja=a+140|0;c[ha>>2]=c[ja>>2];c[ja>>2]=b;ja=a+24|0;b=c[ja>>2]|0;ha=a+28|0;e=c[ha>>2]|0;ia=a+64|0;d=a+68|0;ga=c[d>>2]|0;c[ja>>2]=c[ia>>2];c[ha>>2]=ga;ha=a+184|0;ga=a+188|0;ja=c[ga>>2]|0;c[d>>2]=c[ha>>2];c[ia>>2]=ja;ia=a+144|0;ja=a+148|0;d=c[ja>>2]|0;c[ha>>2]=c[ia>>2];c[ga>>2]=d;c[ja>>2]=b;c[ia>>2]=e;ia=a+104|0;e=c[ia>>2]|0;ja=a+108|0;c[ia>>2]=c[ja>>2];c[ja>>2]=e;ja=a+32|0;e=c[ja>>2]|0;ia=a+36|0;b=c[ia>>2]|0;ga=a+152|0;d=a+156|0;ha=c[d>>2]|0;c[ia>>2]=c[ga>>2];c[ja>>2]=ha;ja=a+112|0;ha=a+116|0;ia=c[ha>>2]|0;c[ga>>2]=c[ja>>2];c[d>>2]=ia;d=a+192|0;ia=c[a+196>>2]|0;c[ha>>2]=c[d>>2];c[ja>>2]=ia;c[d>>2]=e;e=1;ja=5;break}default:{}}if((ja|0)==5)c[d+(e<<2)>>2]=b;o=a+32|0;p=a+72|0;q=a+112|0;r=a+152|0;s=a+192|0;t=a+12|0;u=a+52|0;v=a+92|0;w=a+132|0;x=a+172|0;y=a+36|0;z=a+76|0;A=a+116|0;B=a+156|0;C=a+196|0;D=a+8|0;E=a+48|0;F=a+88|0;G=a+128|0;H=a+168|0;I=a+16|0;J=a+56|0;K=a+96|0;L=a+136|0;M=a+176|0;N=a+20|0;O=a+60|0;P=a+100|0;Q=a+140|0;R=a+180|0;S=a+40|0;T=a+80|0;U=a+120|0;V=a+160|0;W=a+4|0;X=a+44|0;Y=a+84|0;Z=a+124|0;_=a+164|0;$=a+28|0;aa=a+68|0;ba=a+108|0;ca=a+148|0;da=a+188|0;ea=a+24|0;fa=a+64|0;ga=a+104|0;ha=a+144|0;ia=a+184|0;b=f;d=g;a:while(1){switch(b&3){case 0:{ma=c[s>>2]|0;e=c[p>>2]^c[o>>2]^c[q>>2]^c[r>>2]^ma;ja=c[u>>2]^c[t>>2]^c[v>>2]^c[w>>2]^c[x>>2];n=(ja<<1|ja>>>31)^e;ka=c[z>>2]^c[y>>2]^c[A>>2]^c[B>>2]^c[C>>2];m=c[E>>2]|0;l=m^c[D>>2]^c[F>>2]^c[G>>2]^c[H>>2];la=l^ka;g=c[J>>2]^c[I>>2]^c[K>>2]^c[L>>2]^c[M>>2];ka=g^(ka<<1|ka>>>31);oa=c[P>>2]|0;b=c[O>>2]^c[N>>2]^oa^c[Q>>2]^c[R>>2];e=b^e;k=c[a>>2]|0;i=c[S>>2]^k^c[T>>2]^c[U>>2]^c[V>>2];b=i^(b<<1|b>>>31);f=c[W>>2]|0;j=c[X>>2]^f^c[Y>>2]^c[Z>>2]^c[_>>2];g=j^g;na=c[ca>>2]|0;pa=c[aa>>2]^c[$>>2]^c[ba>>2]^na^c[da>>2];l=(pa<<1|pa>>>31)^l;h=c[ha>>2]|0;qa=c[fa>>2]^c[ea>>2]^c[ga>>2]^h^c[ia>>2];ja=qa^ja;j=qa^(j<<1|j>>>31);i=pa^i;k=k^n;m=b^m;m=m<<22|m>>>10;oa=ja^oa;oa=oa<<22|oa>>>10;na=na^e;na=na<<11|na>>>21;ma=j^ma;ma=ma<<7|ma>>>25;pa=oa&~m^k;c[a>>2]=pa;c[a>>2]=pa^c[d>>2];c[E>>2]=na&~oa^m;c[P>>2]=ma&~na^oa;c[ca>>2]=k&~ma^na;c[s>>2]=ma^m&~k;f=f^la;k=c[u>>2]^g;k=k<<22|k>>>10;m=c[K>>2]^l;m=m<<21|m>>>11;h=h^ka;h=h<<10|h>>>22;ma=c[C>>2]^i;ma=ma<<7|ma>>>25;na=m&~k^f;c[W>>2]=na;c[W>>2]=na^c[d+4>>2];c[u>>2]=h&~m^k;c[K>>2]=ma&~h^m;c[ha>>2]=f&~ma^h;c[C>>2]=ma^k&~f;f=c[Y>>2]^la;f=f<<2|f>>>30;k=c[w>>2]^g;k=k<<23|k>>>9;ma=c[R>>2]^ja;ma=ma<<31|ma>>>1;h=c[ea>>2]^ka;h=h<<14|h>>>18;m=c[p>>2]^j;m=m<<10|m>>>22;c[Y>>2]=f&~m^h;c[w>>2]=m^k&~f;c[R>>2]=ma&~k^f;c[ea>>2]=h&~ma^k;c[p>>2]=m&~h^ma;ma=c[T>>2]^n;ma=ma<<1|ma>>>31;h=c[G>>2]^b;h=h<<22|h>>>10;m=c[M>>2]^l;m=m<<30|m>>>2;k=c[$>>2]^e;k=k<<14|k>>>18;f=c[z>>2]^i;f=f<<10|f>>>22;c[T>>2]=ma&~f^k;c[G>>2]=f^h&~ma;c[M>>2]=m&~h^ma;c[$>>2]=k&~m^h;c[z>>2]=f&~k^m;m=c[V>>2]^n;m=m<<9|m>>>23;k=c[t>>2]^g;k=k<<1|k>>>31;f=c[J>>2]^l;f=f<<3|f>>>29;h=c[ba>>2]^e;h=h<<13|h>>>19;ma=c[r>>2]^j;ma=ma<<4|ma>>>28;c[V>>2]=h&~f^k;c[t>>2]=ma&~h^f;c[J>>2]=m&~ma^h;c[ba>>2]=ma^k&~m;c[r>>2]=f&~k^m;m=c[_>>2]^la;m=m<<9|m>>>23;k=c[D>>2]^b;f=c[O>>2]^ja;f=f<<3|f>>>29;ma=c[ga>>2]^ka;ma=ma<<12|ma>>>20;h=c[B>>2]^i;h=h<<4|h>>>28;c[_>>2]=ma&~f^k;c[D>>2]=h&~ma^f;c[O>>2]=m&~h^ma;c[ga>>2]=h^k&~m;c[B>>2]=f&~k^m;m=c[S>>2]^n;m=m<<18|m>>>14;k=c[F>>2]^b;k=k<<5|k>>>27;f=c[Q>>2]^ja;f=f<<8|f>>>24;h=c[ia>>2]^ka;h=h<<28|h>>>4;ma=c[y>>2]^i;ma=ma<<14|ma>>>18;c[S>>2]=ma^k&~m;c[F>>2]=f&~k^m;c[Q>>2]=h&~f^k;c[ia>>2]=ma&~h^f;c[y>>2]=m&~ma^h;h=c[X>>2]^la;h=h<<18|h>>>14;ma=c[v>>2]^g;ma=ma<<5|ma>>>27;m=c[L>>2]^l;m=m<<7|m>>>25;f=c[da>>2]^e;f=f<<28|f>>>4;k=c[o>>2]^j;k=k<<13|k>>>19;c[X>>2]=k^ma&~h;c[v>>2]=m&~ma^h;c[L>>2]=f&~m^ma;m=k&~f^m;c[da>>2]=m;f=h&~k^f;c[o>>2]=f;la=c[Z>>2]^la;la=la<<21|la>>>11;b=c[H>>2]^b;b=b<<1|b>>>31;l=c[I>>2]^l;l=l<<31|l>>>1;e=c[aa>>2]^e;e=e<<28|e>>>4;i=c[A>>2]^i;i=i<<20|i>>>12;k=i&~e^l;c[Z>>2]=k;h=la&~i^e;c[H>>2]=h;i=i^b&~la;c[I>>2]=i;c[aa>>2]=l&~b^la;b=e&~l^b;c[A>>2]=b;n=c[U>>2]^n;n=n<<20|n>>>12;g=c[x>>2]^g;g=g<<1|g>>>31;ja=c[N>>2]^ja;ja=ja<<31|ja>>>1;ka=c[fa>>2]^ka;ka=ka<<27|ka>>>5;j=c[q>>2]^j;j=j<<19|j>>>13;l=j&~ka^ja;c[U>>2]=l;e=n&~j^ka;c[x>>2]=e;j=j^g&~n;c[N>>2]=j;n=ja&~g^n;c[fa>>2]=n;g=ka&~ja^g;c[q>>2]=g;d=d+8|0;ja=12;break}case 3:{b=c[A>>2]|0;e=c[x>>2]|0;f=c[o>>2]|0;g=c[q>>2]|0;h=c[H>>2]|0;i=c[I>>2]|0;j=c[N>>2]|0;k=c[Z>>2]|0;l=c[U>>2]|0;m=c[da>>2]|0;n=c[fa>>2]|0;ja=12;break}case 2:{b=c[B>>2]|0;e=c[x>>2]|0;f=c[s>>2]|0;g=c[r>>2]|0;h=c[H>>2]|0;i=c[P>>2]|0;j=c[K>>2]|0;k=c[X>>2]|0;l=c[S>>2]|0;m=c[fa>>2]|0;n=c[ea>>2]|0;ja=13;break}case 1:{n=d;b=c[o>>2]|0;d=c[x>>2]|0;e=c[A>>2]|0;f=c[y>>2]|0;g=c[H>>2]|0;h=c[O>>2]|0;i=c[J>>2]|0;j=c[T>>2]|0;k=c[Y>>2]|0;l=c[ea>>2]|0;m=c[ca>>2]|0;break}default:{ja=15;break a}}if((ja|0)==12){na=c[p>>2]^c[s>>2]^c[r>>2]^c[y>>2]^b;qa=c[G>>2]^c[u>>2]^c[D>>2]^c[v>>2]^e;e=(qa<<1|qa>>>31)^na;pa=c[z>>2]^c[C>>2]^c[B>>2]^f^g;ma=c[w>>2]|0;ra=ma^c[E>>2]^c[t>>2]^c[F>>2]^h;oa=ra^pa;g=c[R>>2]^c[P>>2]^c[J>>2]^c[Q>>2]^i;pa=g^(pa<<1|pa>>>31);la=c[O>>2]|0;f=c[M>>2]^c[K>>2]^la^c[L>>2]^j;h=f^na;na=c[a>>2]|0;i=c[Y>>2]^na^c[V>>2]^c[S>>2]^k;k=i^(f<<1|f>>>31);f=c[W>>2]|0;j=c[T>>2]^f^c[_>>2]^c[X>>2]^l;g=j^g;ka=c[$>>2]^c[ha>>2]^c[ga>>2]^m^n;n=(ka<<1|ka>>>31)^ra;l=c[ia>>2]|0;ra=c[ea>>2]^c[ca>>2]^c[ba>>2]^l^c[aa>>2];qa=ra^qa;j=ra^(j<<1|j>>>31);i=ka^i;na=na^e;ma=k^ma;ma=ma<<22|ma>>>10;la=qa^la;la=la<<22|la>>>10;m=m^h;m=m<<11|m>>>21;b=j^b;b=b<<7|b>>>25;ka=la&~ma^na;c[a>>2]=ka;c[a>>2]=ka^c[d>>2];c[w>>2]=m&~la^ma;c[O>>2]=b&~m^la;c[da>>2]=na&~b^m;c[A>>2]=b^ma&~na;f=f^oa;b=c[G>>2]^g;b=b<<22|b>>>10;m=c[J>>2]^n;m=m<<21|m>>>11;l=l^pa;l=l<<10|l>>>22;na=c[q>>2]^i;na=na<<7|na>>>25;ma=m&~b^f;c[W>>2]=ma;c[W>>2]=ma^c[d+4>>2];c[G>>2]=l&~m^b;c[J>>2]=na&~l^m;c[ia>>2]=f&~na^l;c[q>>2]=na^b&~f;f=c[_>>2]^oa;f=f<<2|f>>>30;b=c[v>>2]^g;b=b<<23|b>>>9;na=c[N>>2]^qa;na=na<<31|na>>>1;l=c[ca>>2]^pa;l=l<<14|l>>>18;m=c[p>>2]^j;m=m<<10|m>>>22;c[_>>2]=f&~m^l;c[v>>2]=m^b&~f;c[N>>2]=na&~b^f;c[ca>>2]=l&~na^b;c[p>>2]=m&~l^na;na=c[V>>2]^e;na=na<<1|na>>>31;l=c[F>>2]^k;l=l<<22|l>>>10;m=c[I>>2]^n;m=m<<30|m>>>2;b=c[ha>>2]^h;b=b<<14|b>>>18;f=c[z>>2]^i;f=f<<10|f>>>22;c[V>>2]=na&~f^b;c[F>>2]=f^l&~na;c[I>>2]=m&~l^na;c[ha>>2]=b&~m^l;c[z>>2]=f&~b^m;m=c[Z>>2]^e;m=m<<9|m>>>23;b=c[u>>2]^g;b=b<<1|b>>>31;f=c[R>>2]^n;f=f<<3|f>>>29;l=c[ga>>2]^h;l=l<<13|l>>>19;na=c[y>>2]^j;na=na<<4|na>>>28;c[Z>>2]=l&~f^b;c[u>>2]=na&~l^f;c[R>>2]=m&~na^l;c[ga>>2]=na^b&~m;c[y>>2]=f&~b^m;m=c[U>>2]^oa;m=m<<9|m>>>23;b=c[E>>2]^k;f=c[M>>2]^qa;f=f<<3|f>>>29;na=c[ba>>2]^pa;na=na<<12|na>>>20;l=c[o>>2]^i;l=l<<4|l>>>28;c[U>>2]=na&~f^b;c[E>>2]=l&~na^f;c[M>>2]=m&~l^na;c[ba>>2]=l^b&~m;c[o>>2]=f&~b^m;m=c[Y>>2]^e;m=m<<18|m>>>14;b=c[t>>2]^k;b=b<<5|b>>>27;f=c[L>>2]^qa;f=f<<8|f>>>24;l=c[aa>>2]^pa;l=l<<28|l>>>4;na=c[C>>2]^i;na=na<<14|na>>>18;c[Y>>2]=na^b&~m;c[t>>2]=f&~b^m;c[L>>2]=l&~f^b;c[aa>>2]=na&~l^f;c[C>>2]=m&~na^l;l=c[T>>2]^oa;l=l<<18|l>>>14;na=c[D>>2]^g;na=na<<5|na>>>27;m=c[Q>>2]^n;m=m<<7|m>>>25;f=c[fa>>2]^h;f=f<<28|f>>>4;b=c[s>>2]^j;b=b<<13|b>>>19;c[T>>2]=b^na&~l;c[D>>2]=m&~na^l;c[Q>>2]=f&~m^na;m=b&~f^m;c[fa>>2]=m;f=l&~b^f;c[s>>2]=f;oa=c[X>>2]^oa;oa=oa<<21|oa>>>11;b=c[H>>2]^k;b=b<<1|b>>>31;n=c[P>>2]^n;n=n<<31|n>>>1;l=c[$>>2]^h;l=l<<28|l>>>4;i=c[B>>2]^i;i=i<<20|i>>>12;k=i&~l^n;c[X>>2]=k;h=oa&~i^l;c[H>>2]=h;i=i^b&~oa;c[P>>2]=i;c[$>>2]=n&~b^oa;b=l&~n^b;c[B>>2]=b;n=c[S>>2]^e;n=n<<20|n>>>12;g=c[x>>2]^g;g=g<<1|g>>>31;qa=c[K>>2]^qa;qa=qa<<31|qa>>>1;pa=c[ea>>2]^pa;pa=pa<<27|pa>>>5;j=c[r>>2]^j;j=j<<19|j>>>13;l=j&~pa^qa;c[S>>2]=l;e=n&~j^pa;c[x>>2]=e;j=j^g&~n;c[K>>2]=j;n=qa&~g^n;c[ea>>2]=n;g=pa&~qa^g;c[r>>2]=g;d=d+8|0;ja=13}if((ja|0)==13){ja=0;la=c[p>>2]^c[A>>2]^c[y>>2]^c[C>>2]^b;sa=c[F>>2]^c[G>>2]^c[E>>2]^c[D>>2]^e;ra=(sa<<1|sa>>>31)^la;qa=c[z>>2]^c[q>>2]^c[o>>2]^f^g;na=c[v>>2]|0;oa=na^c[w>>2]^c[u>>2]^c[t>>2]^h;pa=oa^qa;f=c[N>>2]^c[O>>2]^c[R>>2]^c[L>>2]^i;qa=f^(qa<<1|qa>>>31);ma=c[M>>2]|0;g=c[I>>2]^c[J>>2]^ma^c[Q>>2]^j;j=g^la;la=c[a>>2]|0;h=c[_>>2]^la^c[Z>>2]^c[Y>>2]^k;g=h^(g<<1|g>>>31);e=c[W>>2]|0;i=c[V>>2]^e^c[U>>2]^c[T>>2]^l;f=i^f;l=c[ha>>2]^c[ia>>2]^c[ba>>2]^m^n;k=(l<<1|l>>>31)^oa;oa=c[aa>>2]|0;ka=c[ca>>2]^c[da>>2]^c[ga>>2]^oa^c[$>>2];n=ka^sa;i=ka^(i<<1|i>>>31);h=l^h;l=la^ra;na=g^na;na=na<<22|na>>>10;ma=n^ma;ma=ma<<22|ma>>>10;m=m^j;m=m<<11|m>>>21;b=i^b;b=b<<7|b>>>25;la=ma&~na^l;c[a>>2]=la;c[a>>2]=la^c[d>>2];c[v>>2]=m&~ma^na;c[M>>2]=b&~m^ma;c[fa>>2]=l&~b^m;c[B>>2]=b^na&~l;e=e^pa;b=c[F>>2]^f;b=b<<22|b>>>10;l=c[R>>2]^k;l=l<<21|l>>>11;m=oa^qa;m=m<<10|m>>>22;oa=c[r>>2]^h;oa=oa<<7|oa>>>25;na=l&~b^e;c[W>>2]=na;c[W>>2]=na^c[d+4>>2];c[F>>2]=m&~l^b;c[R>>2]=oa&~m^l;c[aa>>2]=e&~oa^m;c[r>>2]=oa^b&~e;e=c[U>>2]^pa;e=e<<2|e>>>30;b=c[D>>2]^f;b=b<<23|b>>>9;oa=c[K>>2]^n;oa=oa<<31|oa>>>1;m=c[da>>2]^qa;m=m<<14|m>>>18;l=c[p>>2]^i;l=l<<10|l>>>22;c[U>>2]=e&~l^m;c[D>>2]=l^b&~e;c[K>>2]=oa&~b^e;c[da>>2]=m&~oa^b;c[p>>2]=l&~m^oa;oa=c[Z>>2]^ra;oa=oa<<1|oa>>>31;m=c[t>>2]^g;m=m<<22|m>>>10;l=c[P>>2]^k;l=l<<30|l>>>2;b=c[ia>>2]^j;b=b<<14|b>>>18;e=c[z>>2]^h;e=e<<10|e>>>22;c[Z>>2]=oa&~e^b;c[t>>2]=e^m&~oa;c[P>>2]=l&~m^oa;c[ia>>2]=b&~l^m;c[z>>2]=e&~b^l;l=c[X>>2]^ra;l=l<<9|l>>>23;b=c[G>>2]^f;b=b<<1|b>>>31;e=c[N>>2]^k;e=e<<3|e>>>29;m=c[ba>>2]^j;m=m<<13|m>>>19;oa=c[C>>2]^i;oa=oa<<4|oa>>>28;c[X>>2]=m&~e^b;c[G>>2]=oa&~m^e;c[N>>2]=l&~oa^m;c[ba>>2]=oa^b&~l;c[C>>2]=e&~b^l;l=c[S>>2]^pa;l=l<<9|l>>>23;b=c[w>>2]^g;e=c[I>>2]^n;e=e<<3|e>>>29;oa=c[ga>>2]^qa;oa=oa<<12|oa>>>20;m=c[s>>2]^h;m=m<<4|m>>>28;c[S>>2]=oa&~e^b;c[w>>2]=m&~oa^e;c[I>>2]=l&~m^oa;c[ga>>2]=m^b&~l;c[s>>2]=e&~b^l;l=c[_>>2]^ra;l=l<<18|l>>>14;b=c[u>>2]^g;b=b<<5|b>>>27;e=c[Q>>2]^n;e=e<<8|e>>>24;m=c[$>>2]^qa;m=m<<28|m>>>4;oa=c[q>>2]^h;oa=oa<<14|oa>>>18;c[_>>2]=oa^b&~l;c[u>>2]=e&~b^l;c[Q>>2]=m&~e^b;c[$>>2]=oa&~m^e;c[q>>2]=l&~oa^m;m=c[V>>2]^pa;m=m<<18|m>>>14;oa=c[E>>2]^f;oa=oa<<5|oa>>>27;l=c[L>>2]^k;l=l<<7|l>>>25;e=c[ea>>2]^j;e=e<<28|e>>>4;b=c[A>>2]^i;b=b<<13|b>>>19;c[V>>2]=b^oa&~m;c[E>>2]=l&~oa^m;c[L>>2]=e&~l^oa;l=b&~e^l;c[ea>>2]=l;e=m&~b^e;c[A>>2]=e;pa=c[T>>2]^pa;pa=pa<<21|pa>>>11;b=c[H>>2]^g;b=b<<1|b>>>31;m=c[O>>2]^k;m=m<<31|m>>>1;k=c[ha>>2]^j;k=k<<28|k>>>4;h=c[o>>2]^h;h=h<<20|h>>>12;j=h&~k^m;c[T>>2]=j;g=pa&~h^k;c[H>>2]=g;h=h^b&~pa;c[O>>2]=h;c[ha>>2]=m&~b^pa;b=k&~m^b;c[o>>2]=b;m=c[Y>>2]^ra;m=m<<20|m>>>12;f=c[x>>2]^f;f=f<<1|f>>>31;n=c[J>>2]^n;n=n<<31|n>>>1;qa=c[ca>>2]^qa;qa=qa<<27|qa>>>5;i=c[y>>2]^i;i=i<<19|i>>>13;k=i&~qa^n;c[Y>>2]=k;ra=m&~i^qa;c[x>>2]=ra;i=i^f&~m;c[J>>2]=i;m=n&~f^m;c[ca>>2]=m;f=qa&~n^f;c[y>>2]=f;n=d+8|0;d=ra}la=c[p>>2]^c[B>>2]^c[C>>2]^c[q>>2]^b;ra=c[t>>2]^c[F>>2]^c[w>>2]^c[E>>2]^d;pa=(ra<<1|ra>>>31)^la;qa=c[z>>2]^c[r>>2]^c[s>>2]^e^f;f=c[D>>2]|0;ma=f^c[v>>2]^c[G>>2]^c[u>>2]^g;ka=ma^qa;sa=c[K>>2]^c[M>>2]^c[N>>2]^c[Q>>2]^h;qa=sa^(qa<<1|qa>>>31);d=c[I>>2]|0;na=c[P>>2]^c[R>>2]^d^c[L>>2]^i;la=na^la;h=c[a>>2]|0;g=c[U>>2]^h^c[X>>2]^c[_>>2]^j;na=g^(na<<1|na>>>31);i=c[W>>2]|0;oa=c[Z>>2]^i^c[S>>2]^c[V>>2]^k;sa=oa^sa;m=c[ia>>2]^c[aa>>2]^c[ga>>2]^l^m;ma=(m<<1|m>>>31)^ma;j=c[$>>2]|0;k=c[da>>2]^c[fa>>2]^c[ba>>2]^j^c[ha>>2];ra=k^ra;oa=k^(oa<<1|oa>>>31);m=m^g;h=h^pa;k=na^f;k=k<<22|k>>>10;d=ra^d;d=d<<22|d>>>10;g=l^la;g=g<<11|g>>>21;l=oa^b;l=l<<7|l>>>25;f=d&~k^h;c[a>>2]=f;c[a>>2]=f^c[n>>2];c[D>>2]=g&~d^k;c[I>>2]=l&~g^d;c[ea>>2]=h&~l^g;c[o>>2]=l^k&~h;l=i^ka;i=c[t>>2]^sa;i=i<<22|i>>>10;h=c[N>>2]^ma;h=h<<21|h>>>11;j=j^qa;j=j<<10|j>>>22;k=c[y>>2]^m;k=k<<7|k>>>25;g=h&~i^l;c[W>>2]=g;d=n+8|0;c[W>>2]=g^c[n+4>>2];c[t>>2]=j&~h^i;c[N>>2]=k&~j^h;c[$>>2]=l&~k^j;c[y>>2]=k^i&~l;n=c[S>>2]^ka;n=n<<2|n>>>30;l=c[E>>2]^sa;l=l<<23|l>>>9;i=c[J>>2]^ra;i=i<<31|i>>>1;k=c[fa>>2]^qa;k=k<<14|k>>>18;j=c[p>>2]^oa;j=j<<10|j>>>22;c[S>>2]=n&~j^k;c[E>>2]=j^l&~n;c[J>>2]=i&~l^n;c[fa>>2]=k&~i^l;c[p>>2]=j&~k^i;i=c[X>>2]^pa;i=i<<1|i>>>31;k=c[u>>2]^na;k=k<<22|k>>>10;j=c[O>>2]^ma;j=j<<30|j>>>2;l=c[aa>>2]^la;l=l<<14|l>>>18;n=c[z>>2]^m;n=n<<10|n>>>22;c[X>>2]=i&~n^l;c[u>>2]=n^k&~i;c[O>>2]=j&~k^i;c[aa>>2]=l&~j^k;c[z>>2]=n&~l^j;j=c[T>>2]^pa;j=j<<9|j>>>23;l=c[F>>2]^sa;l=l<<1|l>>>31;n=c[K>>2]^ma;n=n<<3|n>>>29;k=c[ga>>2]^la;k=k<<13|k>>>19;i=c[q>>2]^oa;i=i<<4|i>>>28;c[T>>2]=k&~n^l;c[F>>2]=i&~k^n;c[K>>2]=j&~i^k;c[ga>>2]=i^l&~j;c[q>>2]=n&~l^j;j=c[Y>>2]^ka;j=j<<9|j>>>23;l=c[v>>2]^na;n=c[P>>2]^ra;n=n<<3|n>>>29;i=c[ba>>2]^qa;i=i<<12|i>>>20;k=c[A>>2]^m;k=k<<4|k>>>28;c[Y>>2]=i&~n^l;c[v>>2]=k&~i^n;c[P>>2]=j&~k^i;c[ba>>2]=k^l&~j;c[A>>2]=n&~l^j;j=c[U>>2]^pa;j=j<<18|j>>>14;l=c[G>>2]^na;l=l<<5|l>>>27;n=c[L>>2]^ra;n=n<<8|n>>>24;k=c[ha>>2]^qa;k=k<<28|k>>>4;i=c[r>>2]^m;i=i<<14|i>>>18;c[U>>2]=i^l&~j;c[G>>2]=n&~l^j;c[L>>2]=k&~n^l;c[ha>>2]=i&~k^n;c[r>>2]=j&~i^k;k=c[Z>>2]^ka;k=k<<18|k>>>14;i=c[w>>2]^sa;i=i<<5|i>>>27;j=c[Q>>2]^ma;j=j<<7|j>>>25;n=c[ca>>2]^la;n=n<<28|n>>>4;l=c[B>>2]^oa;l=l<<13|l>>>19;c[Z>>2]=l^i&~k;c[w>>2]=j&~i^k;c[Q>>2]=n&~j^i;c[ca>>2]=l&~n^j;c[B>>2]=k&~l^n;ka=c[V>>2]^ka;ka=ka<<21|ka>>>11;na=c[H>>2]^na;na=na<<1|na>>>31;ma=c[M>>2]^ma;ma=ma<<31|ma>>>1;la=c[ia>>2]^la;la=la<<28|la>>>4;n=c[s>>2]^m;n=n<<20|n>>>12;c[V>>2]=n&~la^ma;c[H>>2]=ka&~n^la;c[M>>2]=n^na&~ka;c[ia>>2]=ma&~na^ka;c[s>>2]=la&~ma^na;pa=c[_>>2]^pa;pa=pa<<20|pa>>>12;sa=c[x>>2]^sa;sa=sa<<1|sa>>>31;ra=c[R>>2]^ra;ra=ra<<31|ra>>>1;qa=c[da>>2]^qa;qa=qa<<27|qa>>>5;oa=c[C>>2]^oa;oa=oa<<19|oa>>>13;c[_>>2]=oa&~qa^ra;c[x>>2]=pa&~oa^qa;c[R>>2]=oa^sa&~pa;c[da>>2]=ra&~sa^pa;c[C>>2]=qa&~ra^sa;if((c[d>>2]|0)==255){ja=16;break}else b=0}if((ja|0)!=15)if((ja|0)==16)return}function ta(a,b,d){a=a|0;b=b|0;d=d|0;var e=0,f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0,p=0,q=0,r=0,s=0,t=0;r=l;l=l+16|0;p=r;e=c[a+200>>2]|0;q=e>>>3;if(c[a+208>>2]|0){q=1;l=r;return q|0}if(!d){q=0;l=r;return q|0}o=a+204|0;i=e>>>6;j=q&536870904;k=q&7;h=p+4|0;n=i<<1;m=a+(n<<2)|0;n=a+((n|1)<<2)|0;e=b;f=0;while(1){g=c[o>>2]|0;b=d-f|0;if((f+q|0)>>>0>d>>>0|(g|0)!=0){s=(g+b|0)>>>0>q>>>0?q-g|0:b;b=s+f|0;pa(a,e,g,s);e=e+s|0;g=(c[o>>2]|0)+s|0;c[o>>2]=g;if((g|0)==(q|0)){sa(a,12);c[o>>2]=0}}else{if(b>>>0>=q>>>0)do{oa(a,e,i);t=p;c[t>>2]=0;c[t+4>>2]=0;Bb(p|0,e+j|0,k|0)|0;t=c[p>>2]|0;g=c[h>>2]|0;f=(t>>>1^t)&572662306;t=f^t;f=t^f<<1;t=(f^t>>>2)&202116108;f=t^f;t=f^t<<2;f=(t^f>>>4)&15728880;t=f^t;f=t^f<<4;t=(f^t>>>8)&65280;s=(g>>>1^g)&572662306;g=s^g;s=g^s<<1;g=(s^g>>>2)&202116108;s=g^s;g=s^g<<2;s=(g^s>>>4)&15728880;g=s^g;s=g^s<<4;g=(s^g>>>8)&65280;c[m>>2]=((g^s)<<16|t^f&65535)^c[m>>2];c[n>>2]=((t<<8^f)>>>16|g<<8^s&-65536)^c[n>>2];sa(a,12);e=e+q|0;b=b-q|0}while(b>>>0>=q>>>0);b=d-b|0}if(b>>>0<d>>>0)f=b;else{e=0;break}}l=r;return e|0}function ua(a,b){a=a|0;b=b|0;var d=0,e=0,f=0,g=0,h=0,i=0,j=0,k=0,l=0;d=(c[a+200>>2]|0)>>>3;if(!(b<<24>>24)){f=1;return f|0}e=a+208|0;if(c[e>>2]|0){f=1;return f|0}f=a+204|0;g=c[f>>2]|0;k=g&7;h=k>>>0<4;j=b&255;k=k<<3;i=h?0:j<<k+-32;k=h?j<<k:0;j=(k>>>1^k)&572662306;k=j^k;j=k^j<<1;k=(j^k>>>2)&202116108;j=k^j;k=j^k<<2;j=(k^j>>>4)&15728880;k=j^k;j=k^j<<4;k=(j^k>>>8)&65280;h=(i>>>1^i)&572662306;i=h^i;h=i^h<<1;i=(h^i>>>2)&202116108;h=i^h;i=h^i<<2;h=(i^h>>>4)&15728880;i=h^i;h=i^h<<4;i=(h^i>>>8)&65280;g=g>>>3<<1;l=a+(g<<2)|0;c[l>>2]=((i^h)<<16|k^j&65535)^c[l>>2];g=a+((g|1)<<2)|0;c[g>>2]=((k<<8^j)>>>16|i<<8^h&-65536)^c[g>>2];if(b<<24>>24<0){b=d+-1|0;if((c[f>>2]|0)==(b|0))sa(a,12)}else b=d+-1|0;h=b&7;i=h>>>0<4;h=h<<3;j=i?0:128<<h+-32;h=i?128<<h:0;i=(h>>>1^h)&572662306;h=i^h;i=h^i<<1;h=(i^h>>>2)&202116108;i=h^i;h=i^h<<2;i=(h^i>>>4)&15728880;h=i^h;i=h^i<<4;h=(i^h>>>8)&65280;k=(j>>>1^j)&572662306;j=k^j;k=j^k<<1;j=(k^j>>>2)&202116108;k=j^k;j=k^j<<2;k=(j^k>>>4)&15728880;j=k^j;k=j^k<<4;j=(k^j>>>8)&65280;l=b>>>3<<1;g=a+(l<<2)|0;c[g>>2]=c[g>>2]^((j^k)<<16|h^i&65535);l=a+((l|1)<<2)|0;c[l>>2]=c[l>>2]^((h<<8^i)>>>16|j<<8^k&-65536);sa(a,12);c[f>>2]=0;c[e>>2]=1;l=0;return l|0}function va(a,b,d){a=a|0;b=b|0;d=d|0;var e=0,f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0,p=0,q=0,r=0,s=0,t=0,u=0;q=l;l=l+16|0;o=q;e=c[a+200>>2]|0;p=e>>>3;if(!(c[a+208>>2]|0))ua(a,1)|0;if(!d){l=q;return 0}n=a+204|0;h=e>>>6;i=p&536870904;j=p&7;m=h<<1;k=a+(m<<2)|0;m=a+((m|1)<<2)|0;g=o+4|0;e=0;f=b;do{b=c[n>>2]|0;if((e+p|0)>>>0>d>>>0|(b|0)!=(p|0)){if((b|0)==(p|0)){sa(a,12);c[n>>2]=0;b=0}r=d-e|0;r=(b+r|0)>>>0>p>>>0?p-b|0:r;ra(a,f,b,r);c[n>>2]=r+(c[n>>2]|0);f=f+r|0;e=r+e|0}else{e=d-e|0;if(e>>>0<p>>>0){b=e;e=f}else{b=e;e=f;do{sa(a,12);qa(a,e,h);s=c[k>>2]|0;u=c[m>>2]|0;r=s>>>16;t=(u<<8^s)&65280;s=t^(u<<16|s&65535);t=s^t<<8;s=(t^s>>>4)&15728880;t=s^t;s=t^s<<4;t=(s^t>>>2)&202116108;s=t^s;t=s^t<<2;s=(t^s>>>1)&572662306;f=(u>>>8^r)&65280;r=f^(u&-65536|r);f=r^f<<8;r=(f^r>>>4)&15728880;f=r^f;r=f^r<<4;f=(r^f>>>2)&202116108;r=f^r;f=r^f<<2;r=(f^r>>>1)&572662306;c[o>>2]=s^t^s<<1;c[g>>2]=r^f^r<<1;Bb(e+i|0,o|0,j|0)|0;e=e+p|0;b=b-p|0}while(b>>>0>=p>>>0)}f=e;e=d-b|0}}while(e>>>0<d>>>0);l=q;return 0}function wa(a,b){a=a|0;b=b|0;c[a+428>>2]=b;c[a+436>>2]=0;c[a+432>>2]=0;c[a+440>>2]=1;Cb(a+216|0,0,200)|0;c[a+416>>2]=1344;c[a+420>>2]=0;c[a+424>>2]=0;return 0}function xa(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0;o=l;l=l+32|0;m=o;if((c[b+440>>2]|0)!=1){n=1;l=o;return n|0}n=b+432|0;k=b+436|0;f=c[k>>2]|0;do if(!(c[n>>2]|0)){g=8192-f|0;g=g>>>0>e>>>0?e:g;h=b+216|0;if(ta(h,d,g)|0){n=1;l=o;return n|0}f=d+g|0;e=e-g|0;j=(c[k>>2]|0)+g|0;c[k>>2]=j;if((e|0)!=0&(j|0)==8192){a[m>>0]=3;c[k>>2]=0;c[n>>2]=1;if(!(ta(h,m,1)|0)){j=b+420|0;c[j>>2]=(c[j>>2]|0)+7&-8;break}n=1;l=o;return n|0}else i=15}else if(f){g=8192-f|0;g=g>>>0>e>>>0?e:g;if(ta(b,d,g)|0){n=1;l=o;return n|0}f=d+g|0;e=e-g|0;j=(c[k>>2]|0)+g|0;c[k>>2]=j;if((j|0)==8192){c[k>>2]=0;c[n>>2]=(c[n>>2]|0)+1;if((ua(b,11)|0)==0?(va(b,m,32)|0,(ta(b+216|0,m,32)|0)==0):0){i=15;break}n=1;l=o;return n|0}else i=15}else{f=d;i=15}while(0);if((i|0)==15)if(!e){n=0;l=o;return n|0}d=b+200|0;h=b+204|0;i=b+208|0;j=b+216|0;while(1){g=e>>>0<8192?e:8192;Cb(b|0,0,200)|0;c[d>>2]=1344;c[h>>2]=0;c[i>>2]=0;if(ta(b,f,g)|0){e=1;i=25;break}f=f+g|0;if(e>>>0>8191){c[n>>2]=(c[n>>2]|0)+1;if(ua(b,11)|0){i=21;break}va(b,m,32)|0;if(ta(j,m,32)|0){i=21;break}}else c[k>>2]=g;e=e-g|0;if(!e){e=0;i=25;break}}if((i|0)==21){n=1;l=o;return n|0}else if((i|0)==25){l=o;return e|0}return 0}function ya(b,d,e,f){b=b|0;d=d|0;e=e|0;f=f|0;var g=0,h=0,i=0,j=0,k=0;k=l;l=l+48|0;i=k+32|0;h=k;j=b+440|0;if((c[j>>2]|0)!=1){j=1;l=k;return j|0}if(!f)e=0;else{if(!(xa(b,e,f)|0)){e=0;g=f}else{j=1;l=k;return j|0}do{e=e+1|0;g=g>>>8}while((g|0)!=0&e>>>0<4);g=1;do{a[i+(g+-1)>>0]=f>>>(e-g<<3);g=g+1|0}while(e>>>0>=g>>>0)}a[i+e>>0]=e;if(xa(b,i,e+1|0)|0){j=1;l=k;return j|0}g=b+432|0;e=c[g>>2]|0;if(e){do if(c[b+436>>2]|0){c[g>>2]=e+1;if((ua(b,11)|0)==0?(va(b,h,32)|0,(ta(b+216|0,h,32)|0)==0):0){e=c[g>>2]|0;break}j=1;l=k;return j|0}while(0);f=e+-1|0;c[g>>2]=f;if(!f)e=0;else{e=0;g=f;do{e=e+1|0;g=g>>>8}while((g|0)!=0&e>>>0<4);g=1;do{a[i+(g+-1)>>0]=f>>>(e-g<<3);g=g+1|0}while(e>>>0>=g>>>0)}a[i+e>>0]=e;a[i+(e+1)>>0]=-1;a[i+(e+2)>>0]=-1;g=b+216|0;if(!(ta(g,i,e+3|0)|0))e=6;else{j=1;l=k;return j|0}}else{e=7;g=b+216|0}if(ua(g,e)|0){j=1;l=k;return j|0}e=c[b+428>>2]|0;if(!e){c[j>>2]=3;j=0;l=k;return j|0}else{c[j>>2]=2;va(g,d,e)|0;j=0;l=k;return j|0}return 0}function za(a,b,d){a=a|0;b=b|0;d=d|0;if((c[a+440>>2]|0)!=3){d=1;return d|0}va(a+216|0,b,d)|0;d=0;return d|0}function Aa(){return Ea(448)|0}function Ba(a){a=a|0;if(!a){a=0;return a|0}a=(c[a+440>>2]|0)==3&1;return a|0}function Ca(a){a=a|0;if(!a){a=0;return a|0}a=(c[a+440>>2]|0)==1&1;return a|0}function Da(a){a=a|0;var b=0,d=0,e=0;d=l;l=l+16|0;e=d;b=a+440|0;c[e>>2]=c[b>>2];qb(576,e)|0;if(!a){e=0;l=d;return e|0}e=c[b>>2]|0;l=d;return e|0}function Ea(a){a=a|0;var b=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0,p=0,q=0,r=0,s=0,t=0,u=0,v=0,w=0,x=0;x=l;l=l+16|0;p=x;do if(a>>>0<245){m=a>>>0<11?16:a+11&-8;a=m>>>3;o=c[753]|0;d=o>>>a;if(d&3|0){b=(d&1^1)+a|0;a=3052+(b<<1<<2)|0;d=a+8|0;e=c[d>>2]|0;f=e+8|0;g=c[f>>2]|0;if((g|0)==(a|0))c[753]=o&~(1<<b);else{c[g+12>>2]=a;c[d>>2]=g}w=b<<3;c[e+4>>2]=w|3;w=e+w+4|0;c[w>>2]=c[w>>2]|1;w=f;l=x;return w|0}n=c[755]|0;if(m>>>0>n>>>0){if(d|0){b=2<<a;b=d<<a&(b|0-b);b=(b&0-b)+-1|0;i=b>>>12&16;b=b>>>i;d=b>>>5&8;b=b>>>d;g=b>>>2&4;b=b>>>g;a=b>>>1&2;b=b>>>a;e=b>>>1&1;e=(d|i|g|a|e)+(b>>>e)|0;b=3052+(e<<1<<2)|0;a=b+8|0;g=c[a>>2]|0;i=g+8|0;d=c[i>>2]|0;if((d|0)==(b|0)){a=o&~(1<<e);c[753]=a}else{c[d+12>>2]=b;c[a>>2]=d;a=o}w=e<<3;h=w-m|0;c[g+4>>2]=m|3;f=g+m|0;c[f+4>>2]=h|1;c[g+w>>2]=h;if(n|0){e=c[758]|0;b=n>>>3;d=3052+(b<<1<<2)|0;b=1<<b;if(!(a&b)){c[753]=a|b;b=d;a=d+8|0}else{a=d+8|0;b=c[a>>2]|0}c[a>>2]=e;c[b+12>>2]=e;c[e+8>>2]=b;c[e+12>>2]=d}c[755]=h;c[758]=f;w=i;l=x;return w|0}j=c[754]|0;if(j){d=(j&0-j)+-1|0;i=d>>>12&16;d=d>>>i;h=d>>>5&8;d=d>>>h;k=d>>>2&4;d=d>>>k;e=d>>>1&2;d=d>>>e;a=d>>>1&1;a=c[3316+((h|i|k|e|a)+(d>>>a)<<2)>>2]|0;d=(c[a+4>>2]&-8)-m|0;e=c[a+16+(((c[a+16>>2]|0)==0&1)<<2)>>2]|0;if(!e){k=a;h=d}else{do{i=(c[e+4>>2]&-8)-m|0;k=i>>>0<d>>>0;d=k?i:d;a=k?e:a;e=c[e+16+(((c[e+16>>2]|0)==0&1)<<2)>>2]|0}while((e|0)!=0);k=a;h=d}i=k+m|0;if(i>>>0>k>>>0){f=c[k+24>>2]|0;b=c[k+12>>2]|0;do if((b|0)==(k|0)){a=k+20|0;b=c[a>>2]|0;if(!b){a=k+16|0;b=c[a>>2]|0;if(!b){d=0;break}}while(1){d=b+20|0;e=c[d>>2]|0;if(e|0){b=e;a=d;continue}d=b+16|0;e=c[d>>2]|0;if(!e)break;else{b=e;a=d}}c[a>>2]=0;d=b}else{d=c[k+8>>2]|0;c[d+12>>2]=b;c[b+8>>2]=d;d=b}while(0);do if(f|0){b=c[k+28>>2]|0;a=3316+(b<<2)|0;if((k|0)==(c[a>>2]|0)){c[a>>2]=d;if(!d){c[754]=j&~(1<<b);break}}else{c[f+16+(((c[f+16>>2]|0)!=(k|0)&1)<<2)>>2]=d;if(!d)break}c[d+24>>2]=f;b=c[k+16>>2]|0;if(b|0){c[d+16>>2]=b;c[b+24>>2]=d}b=c[k+20>>2]|0;if(b|0){c[d+20>>2]=b;c[b+24>>2]=d}}while(0);if(h>>>0<16){w=h+m|0;c[k+4>>2]=w|3;w=k+w+4|0;c[w>>2]=c[w>>2]|1}else{c[k+4>>2]=m|3;c[i+4>>2]=h|1;c[i+h>>2]=h;if(n|0){e=c[758]|0;b=n>>>3;d=3052+(b<<1<<2)|0;b=1<<b;if(!(o&b)){c[753]=o|b;b=d;a=d+8|0}else{a=d+8|0;b=c[a>>2]|0}c[a>>2]=e;c[b+12>>2]=e;c[e+8>>2]=b;c[e+12>>2]=d}c[755]=h;c[758]=i}w=k+8|0;l=x;return w|0}else n=m}else n=m}else n=m}else if(a>>>0<=4294967231){a=a+11|0;m=a&-8;k=c[754]|0;if(k){e=0-m|0;a=a>>>8;if(a)if(m>>>0>16777215)j=31;else{o=(a+1048320|0)>>>16&8;v=a<<o;n=(v+520192|0)>>>16&4;v=v<<n;j=(v+245760|0)>>>16&2;j=14-(n|o|j)+(v<<j>>>15)|0;j=m>>>(j+7|0)&1|j<<1}else j=0;d=c[3316+(j<<2)>>2]|0;a:do if(!d){d=0;a=0;v=57}else{a=0;i=d;h=m<<((j|0)==31?0:25-(j>>>1)|0);d=0;while(1){f=(c[i+4>>2]&-8)-m|0;if(f>>>0<e>>>0)if(!f){e=0;d=i;a=i;v=61;break a}else{a=i;e=f}f=c[i+20>>2]|0;i=c[i+16+(h>>>31<<2)>>2]|0;d=(f|0)==0|(f|0)==(i|0)?d:f;f=(i|0)==0;if(f){v=57;break}else h=h<<((f^1)&1)}}while(0);if((v|0)==57){if((d|0)==0&(a|0)==0){a=2<<j;a=k&(a|0-a);if(!a){n=m;break}o=(a&0-a)+-1|0;i=o>>>12&16;o=o>>>i;h=o>>>5&8;o=o>>>h;j=o>>>2&4;o=o>>>j;n=o>>>1&2;o=o>>>n;d=o>>>1&1;a=0;d=c[3316+((h|i|j|n|d)+(o>>>d)<<2)>>2]|0}if(!d){i=a;h=e}else v=61}if((v|0)==61)while(1){v=0;n=(c[d+4>>2]&-8)-m|0;o=n>>>0<e>>>0;e=o?n:e;a=o?d:a;d=c[d+16+(((c[d+16>>2]|0)==0&1)<<2)>>2]|0;if(!d){i=a;h=e;break}else v=61}if((i|0)!=0?h>>>0<((c[755]|0)-m|0)>>>0:0){g=i+m|0;if(g>>>0<=i>>>0){w=0;l=x;return w|0}f=c[i+24>>2]|0;b=c[i+12>>2]|0;do if((b|0)==(i|0)){a=i+20|0;b=c[a>>2]|0;if(!b){a=i+16|0;b=c[a>>2]|0;if(!b){b=0;break}}while(1){d=b+20|0;e=c[d>>2]|0;if(e|0){b=e;a=d;continue}d=b+16|0;e=c[d>>2]|0;if(!e)break;else{b=e;a=d}}c[a>>2]=0}else{w=c[i+8>>2]|0;c[w+12>>2]=b;c[b+8>>2]=w}while(0);do if(f){a=c[i+28>>2]|0;d=3316+(a<<2)|0;if((i|0)==(c[d>>2]|0)){c[d>>2]=b;if(!b){e=k&~(1<<a);c[754]=e;break}}else{c[f+16+(((c[f+16>>2]|0)!=(i|0)&1)<<2)>>2]=b;if(!b){e=k;break}}c[b+24>>2]=f;a=c[i+16>>2]|0;if(a|0){c[b+16>>2]=a;c[a+24>>2]=b}a=c[i+20>>2]|0;if(a){c[b+20>>2]=a;c[a+24>>2]=b;e=k}else e=k}else e=k;while(0);do if(h>>>0>=16){c[i+4>>2]=m|3;c[g+4>>2]=h|1;c[g+h>>2]=h;b=h>>>3;if(h>>>0<256){d=3052+(b<<1<<2)|0;a=c[753]|0;b=1<<b;if(!(a&b)){c[753]=a|b;b=d;a=d+8|0}else{a=d+8|0;b=c[a>>2]|0}c[a>>2]=g;c[b+12>>2]=g;c[g+8>>2]=b;c[g+12>>2]=d;break}b=h>>>8;if(b)if(h>>>0>16777215)b=31;else{v=(b+1048320|0)>>>16&8;w=b<<v;u=(w+520192|0)>>>16&4;w=w<<u;b=(w+245760|0)>>>16&2;b=14-(u|v|b)+(w<<b>>>15)|0;b=h>>>(b+7|0)&1|b<<1}else b=0;d=3316+(b<<2)|0;c[g+28>>2]=b;a=g+16|0;c[a+4>>2]=0;c[a>>2]=0;a=1<<b;if(!(e&a)){c[754]=e|a;c[d>>2]=g;c[g+24>>2]=d;c[g+12>>2]=g;c[g+8>>2]=g;break}a=h<<((b|0)==31?0:25-(b>>>1)|0);d=c[d>>2]|0;while(1){if((c[d+4>>2]&-8|0)==(h|0)){v=97;break}e=d+16+(a>>>31<<2)|0;b=c[e>>2]|0;if(!b){v=96;break}else{a=a<<1;d=b}}if((v|0)==96){c[e>>2]=g;c[g+24>>2]=d;c[g+12>>2]=g;c[g+8>>2]=g;break}else if((v|0)==97){v=d+8|0;w=c[v>>2]|0;c[w+12>>2]=g;c[v>>2]=g;c[g+8>>2]=w;c[g+12>>2]=d;c[g+24>>2]=0;break}}else{w=h+m|0;c[i+4>>2]=w|3;w=i+w+4|0;c[w>>2]=c[w>>2]|1}while(0);w=i+8|0;l=x;return w|0}else n=m}else n=m}else n=-1;while(0);d=c[755]|0;if(d>>>0>=n>>>0){b=d-n|0;a=c[758]|0;if(b>>>0>15){w=a+n|0;c[758]=w;c[755]=b;c[w+4>>2]=b|1;c[a+d>>2]=b;c[a+4>>2]=n|3}else{c[755]=0;c[758]=0;c[a+4>>2]=d|3;w=a+d+4|0;c[w>>2]=c[w>>2]|1}w=a+8|0;l=x;return w|0}i=c[756]|0;if(i>>>0>n>>>0){u=i-n|0;c[756]=u;w=c[759]|0;v=w+n|0;c[759]=v;c[v+4>>2]=u|1;c[w+4>>2]=n|3;w=w+8|0;l=x;return w|0}if(!(c[871]|0)){c[873]=4096;c[872]=4096;c[874]=-1;c[875]=-1;c[876]=0;c[864]=0;c[871]=p&-16^1431655768;a=4096}else a=c[873]|0;j=n+48|0;k=n+47|0;h=a+k|0;f=0-a|0;m=h&f;if(m>>>0<=n>>>0){w=0;l=x;return w|0}a=c[863]|0;if(a|0?(o=c[861]|0,p=o+m|0,p>>>0<=o>>>0|p>>>0>a>>>0):0){w=0;l=x;return w|0}b:do if(!(c[864]&4)){d=c[759]|0;c:do if(d){e=3460;while(1){a=c[e>>2]|0;if(a>>>0<=d>>>0?(s=e+4|0,(a+(c[s>>2]|0)|0)>>>0>d>>>0):0)break;a=c[e+8>>2]|0;if(!a){v=118;break c}else e=a}b=h-i&f;if(b>>>0<2147483647){a=Db(b|0)|0;if((a|0)==((c[e>>2]|0)+(c[s>>2]|0)|0)){if((a|0)!=(-1|0)){h=b;g=a;v=135;break b}}else{e=a;v=126}}else b=0}else v=118;while(0);do if((v|0)==118){d=Db(0)|0;if((d|0)!=(-1|0)?(b=d,q=c[872]|0,r=q+-1|0,b=((r&b|0)==0?0:(r+b&0-q)-b|0)+m|0,q=c[861]|0,r=b+q|0,b>>>0>n>>>0&b>>>0<2147483647):0){s=c[863]|0;if(s|0?r>>>0<=q>>>0|r>>>0>s>>>0:0){b=0;break}a=Db(b|0)|0;if((a|0)==(d|0)){h=b;g=d;v=135;break b}else{e=a;v=126}}else b=0}while(0);do if((v|0)==126){d=0-b|0;if(!(j>>>0>b>>>0&(b>>>0<2147483647&(e|0)!=(-1|0))))if((e|0)==(-1|0)){b=0;break}else{h=b;g=e;v=135;break b}a=c[873]|0;a=k-b+a&0-a;if(a>>>0>=2147483647){h=b;g=e;v=135;break b}if((Db(a|0)|0)==(-1|0)){Db(d|0)|0;b=0;break}else{h=a+b|0;g=e;v=135;break b}}while(0);c[864]=c[864]|4;v=133}else{b=0;v=133}while(0);if(((v|0)==133?m>>>0<2147483647:0)?(g=Db(m|0)|0,s=Db(0)|0,t=s-g|0,u=t>>>0>(n+40|0)>>>0,!((g|0)==(-1|0)|u^1|g>>>0<s>>>0&((g|0)!=(-1|0)&(s|0)!=(-1|0))^1)):0){h=u?t:b;v=135}if((v|0)==135){b=(c[861]|0)+h|0;c[861]=b;if(b>>>0>(c[862]|0)>>>0)c[862]=b;j=c[759]|0;do if(j){b=3460;while(1){a=c[b>>2]|0;d=b+4|0;e=c[d>>2]|0;if((g|0)==(a+e|0)){v=143;break}f=c[b+8>>2]|0;if(!f)break;else b=f}if(((v|0)==143?(c[b+12>>2]&8|0)==0:0)?g>>>0>j>>>0&a>>>0<=j>>>0:0){c[d>>2]=e+h;w=(c[756]|0)+h|0;u=j+8|0;u=(u&7|0)==0?0:0-u&7;v=j+u|0;u=w-u|0;c[759]=v;c[756]=u;c[v+4>>2]=u|1;c[j+w+4>>2]=40;c[760]=c[875];break}if(g>>>0<(c[757]|0)>>>0)c[757]=g;a=g+h|0;b=3460;while(1){if((c[b>>2]|0)==(a|0)){v=151;break}b=c[b+8>>2]|0;if(!b){a=3460;break}}if((v|0)==151)if(!(c[b+12>>2]&8)){c[b>>2]=g;m=b+4|0;c[m>>2]=(c[m>>2]|0)+h;m=g+8|0;m=g+((m&7|0)==0?0:0-m&7)|0;b=a+8|0;b=a+((b&7|0)==0?0:0-b&7)|0;k=m+n|0;i=b-m-n|0;c[m+4>>2]=n|3;do if((j|0)!=(b|0)){if((c[758]|0)==(b|0)){w=(c[755]|0)+i|0;c[755]=w;c[758]=k;c[k+4>>2]=w|1;c[k+w>>2]=w;break}a=c[b+4>>2]|0;if((a&3|0)==1){h=a&-8;e=a>>>3;d:do if(a>>>0<256){a=c[b+8>>2]|0;d=c[b+12>>2]|0;if((d|0)==(a|0)){c[753]=c[753]&~(1<<e);break}else{c[a+12>>2]=d;c[d+8>>2]=a;break}}else{g=c[b+24>>2]|0;a=c[b+12>>2]|0;do if((a|0)==(b|0)){e=b+16|0;d=e+4|0;a=c[d>>2]|0;if(!a){a=c[e>>2]|0;if(!a){a=0;break}else d=e}while(1){e=a+20|0;f=c[e>>2]|0;if(f|0){a=f;d=e;continue}e=a+16|0;f=c[e>>2]|0;if(!f)break;else{a=f;d=e}}c[d>>2]=0}else{w=c[b+8>>2]|0;c[w+12>>2]=a;c[a+8>>2]=w}while(0);if(!g)break;d=c[b+28>>2]|0;e=3316+(d<<2)|0;do if((c[e>>2]|0)!=(b|0)){c[g+16+(((c[g+16>>2]|0)!=(b|0)&1)<<2)>>2]=a;if(!a)break d}else{c[e>>2]=a;if(a|0)break;c[754]=c[754]&~(1<<d);break d}while(0);c[a+24>>2]=g;d=b+16|0;e=c[d>>2]|0;if(e|0){c[a+16>>2]=e;c[e+24>>2]=a}d=c[d+4>>2]|0;if(!d)break;c[a+20>>2]=d;c[d+24>>2]=a}while(0);b=b+h|0;f=h+i|0}else f=i;b=b+4|0;c[b>>2]=c[b>>2]&-2;c[k+4>>2]=f|1;c[k+f>>2]=f;b=f>>>3;if(f>>>0<256){d=3052+(b<<1<<2)|0;a=c[753]|0;b=1<<b;if(!(a&b)){c[753]=a|b;b=d;a=d+8|0}else{a=d+8|0;b=c[a>>2]|0}c[a>>2]=k;c[b+12>>2]=k;c[k+8>>2]=b;c[k+12>>2]=d;break}b=f>>>8;do if(!b)b=0;else{if(f>>>0>16777215){b=31;break}v=(b+1048320|0)>>>16&8;w=b<<v;u=(w+520192|0)>>>16&4;w=w<<u;b=(w+245760|0)>>>16&2;b=14-(u|v|b)+(w<<b>>>15)|0;b=f>>>(b+7|0)&1|b<<1}while(0);e=3316+(b<<2)|0;c[k+28>>2]=b;a=k+16|0;c[a+4>>2]=0;c[a>>2]=0;a=c[754]|0;d=1<<b;if(!(a&d)){c[754]=a|d;c[e>>2]=k;c[k+24>>2]=e;c[k+12>>2]=k;c[k+8>>2]=k;break}a=f<<((b|0)==31?0:25-(b>>>1)|0);d=c[e>>2]|0;while(1){if((c[d+4>>2]&-8|0)==(f|0)){v=192;break}e=d+16+(a>>>31<<2)|0;b=c[e>>2]|0;if(!b){v=191;break}else{a=a<<1;d=b}}if((v|0)==191){c[e>>2]=k;c[k+24>>2]=d;c[k+12>>2]=k;c[k+8>>2]=k;break}else if((v|0)==192){v=d+8|0;w=c[v>>2]|0;c[w+12>>2]=k;c[v>>2]=k;c[k+8>>2]=w;c[k+12>>2]=d;c[k+24>>2]=0;break}}else{w=(c[756]|0)+i|0;c[756]=w;c[759]=k;c[k+4>>2]=w|1}while(0);w=m+8|0;l=x;return w|0}else a=3460;while(1){b=c[a>>2]|0;if(b>>>0<=j>>>0?(w=b+(c[a+4>>2]|0)|0,w>>>0>j>>>0):0)break;a=c[a+8>>2]|0}f=w+-47|0;a=f+8|0;a=f+((a&7|0)==0?0:0-a&7)|0;f=j+16|0;a=a>>>0<f>>>0?j:a;b=a+8|0;d=h+-40|0;u=g+8|0;u=(u&7|0)==0?0:0-u&7;v=g+u|0;u=d-u|0;c[759]=v;c[756]=u;c[v+4>>2]=u|1;c[g+d+4>>2]=40;c[760]=c[875];d=a+4|0;c[d>>2]=27;c[b>>2]=c[865];c[b+4>>2]=c[866];c[b+8>>2]=c[867];c[b+12>>2]=c[868];c[865]=g;c[866]=h;c[868]=0;c[867]=b;b=a+24|0;do{v=b;b=b+4|0;c[b>>2]=7}while((v+8|0)>>>0<w>>>0);if((a|0)!=(j|0)){g=a-j|0;c[d>>2]=c[d>>2]&-2;c[j+4>>2]=g|1;c[a>>2]=g;b=g>>>3;if(g>>>0<256){d=3052+(b<<1<<2)|0;a=c[753]|0;b=1<<b;if(!(a&b)){c[753]=a|b;b=d;a=d+8|0}else{a=d+8|0;b=c[a>>2]|0}c[a>>2]=j;c[b+12>>2]=j;c[j+8>>2]=b;c[j+12>>2]=d;break}b=g>>>8;if(b)if(g>>>0>16777215)d=31;else{v=(b+1048320|0)>>>16&8;w=b<<v;u=(w+520192|0)>>>16&4;w=w<<u;d=(w+245760|0)>>>16&2;d=14-(u|v|d)+(w<<d>>>15)|0;d=g>>>(d+7|0)&1|d<<1}else d=0;e=3316+(d<<2)|0;c[j+28>>2]=d;c[j+20>>2]=0;c[f>>2]=0;b=c[754]|0;a=1<<d;if(!(b&a)){c[754]=b|a;c[e>>2]=j;c[j+24>>2]=e;c[j+12>>2]=j;c[j+8>>2]=j;break}a=g<<((d|0)==31?0:25-(d>>>1)|0);d=c[e>>2]|0;while(1){if((c[d+4>>2]&-8|0)==(g|0)){v=213;break}e=d+16+(a>>>31<<2)|0;b=c[e>>2]|0;if(!b){v=212;break}else{a=a<<1;d=b}}if((v|0)==212){c[e>>2]=j;c[j+24>>2]=d;c[j+12>>2]=j;c[j+8>>2]=j;break}else if((v|0)==213){v=d+8|0;w=c[v>>2]|0;c[w+12>>2]=j;c[v>>2]=j;c[j+8>>2]=w;c[j+12>>2]=d;c[j+24>>2]=0;break}}}else{w=c[757]|0;if((w|0)==0|g>>>0<w>>>0)c[757]=g;c[865]=g;c[866]=h;c[868]=0;c[762]=c[871];c[761]=-1;c[766]=3052;c[765]=3052;c[768]=3060;c[767]=3060;c[770]=3068;c[769]=3068;c[772]=3076;c[771]=3076;c[774]=3084;c[773]=3084;c[776]=3092;c[775]=3092;c[778]=3100;c[777]=3100;c[780]=3108;c[779]=3108;c[782]=3116;c[781]=3116;c[784]=3124;c[783]=3124;c[786]=3132;c[785]=3132;c[788]=3140;c[787]=3140;c[790]=3148;c[789]=3148;c[792]=3156;c[791]=3156;c[794]=3164;c[793]=3164;c[796]=3172;c[795]=3172;c[798]=3180;c[797]=3180;c[800]=3188;c[799]=3188;c[802]=3196;c[801]=3196;c[804]=3204;c[803]=3204;c[806]=3212;c[805]=3212;c[808]=3220;c[807]=3220;c[810]=3228;c[809]=3228;c[812]=3236;c[811]=3236;c[814]=3244;c[813]=3244;c[816]=3252;c[815]=3252;c[818]=3260;c[817]=3260;c[820]=3268;c[819]=3268;c[822]=3276;c[821]=3276;c[824]=3284;c[823]=3284;c[826]=3292;c[825]=3292;c[828]=3300;c[827]=3300;w=h+-40|0;u=g+8|0;u=(u&7|0)==0?0:0-u&7;v=g+u|0;u=w-u|0;c[759]=v;c[756]=u;c[v+4>>2]=u|1;c[g+w+4>>2]=40;c[760]=c[875]}while(0);b=c[756]|0;if(b>>>0>n>>>0){u=b-n|0;c[756]=u;w=c[759]|0;v=w+n|0;c[759]=v;c[v+4>>2]=u|1;c[w+4>>2]=n|3;w=w+8|0;l=x;return w|0}}c[(Ka()|0)>>2]=12;w=0;l=x;return w|0}function Fa(a){a=a|0;var b=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;if(!a)return;d=a+-8|0;f=c[757]|0;a=c[a+-4>>2]|0;b=a&-8;j=d+b|0;do if(!(a&1)){e=c[d>>2]|0;if(!(a&3))return;h=d+(0-e)|0;g=e+b|0;if(h>>>0<f>>>0)return;if((c[758]|0)==(h|0)){a=j+4|0;b=c[a>>2]|0;if((b&3|0)!=3){i=h;b=g;break}c[755]=g;c[a>>2]=b&-2;c[h+4>>2]=g|1;c[h+g>>2]=g;return}d=e>>>3;if(e>>>0<256){a=c[h+8>>2]|0;b=c[h+12>>2]|0;if((b|0)==(a|0)){c[753]=c[753]&~(1<<d);i=h;b=g;break}else{c[a+12>>2]=b;c[b+8>>2]=a;i=h;b=g;break}}f=c[h+24>>2]|0;a=c[h+12>>2]|0;do if((a|0)==(h|0)){d=h+16|0;b=d+4|0;a=c[b>>2]|0;if(!a){a=c[d>>2]|0;if(!a){a=0;break}else b=d}while(1){d=a+20|0;e=c[d>>2]|0;if(e|0){a=e;b=d;continue}d=a+16|0;e=c[d>>2]|0;if(!e)break;else{a=e;b=d}}c[b>>2]=0}else{i=c[h+8>>2]|0;c[i+12>>2]=a;c[a+8>>2]=i}while(0);if(f){b=c[h+28>>2]|0;d=3316+(b<<2)|0;if((c[d>>2]|0)==(h|0)){c[d>>2]=a;if(!a){c[754]=c[754]&~(1<<b);i=h;b=g;break}}else{c[f+16+(((c[f+16>>2]|0)!=(h|0)&1)<<2)>>2]=a;if(!a){i=h;b=g;break}}c[a+24>>2]=f;b=h+16|0;d=c[b>>2]|0;if(d|0){c[a+16>>2]=d;c[d+24>>2]=a}b=c[b+4>>2]|0;if(b){c[a+20>>2]=b;c[b+24>>2]=a;i=h;b=g}else{i=h;b=g}}else{i=h;b=g}}else{i=d;h=d}while(0);if(h>>>0>=j>>>0)return;a=j+4|0;e=c[a>>2]|0;if(!(e&1))return;if(!(e&2)){if((c[759]|0)==(j|0)){j=(c[756]|0)+b|0;c[756]=j;c[759]=i;c[i+4>>2]=j|1;if((i|0)!=(c[758]|0))return;c[758]=0;c[755]=0;return}if((c[758]|0)==(j|0)){j=(c[755]|0)+b|0;c[755]=j;c[758]=h;c[i+4>>2]=j|1;c[h+j>>2]=j;return}f=(e&-8)+b|0;d=e>>>3;do if(e>>>0<256){b=c[j+8>>2]|0;a=c[j+12>>2]|0;if((a|0)==(b|0)){c[753]=c[753]&~(1<<d);break}else{c[b+12>>2]=a;c[a+8>>2]=b;break}}else{g=c[j+24>>2]|0;a=c[j+12>>2]|0;do if((a|0)==(j|0)){d=j+16|0;b=d+4|0;a=c[b>>2]|0;if(!a){a=c[d>>2]|0;if(!a){d=0;break}else b=d}while(1){d=a+20|0;e=c[d>>2]|0;if(e|0){a=e;b=d;continue}d=a+16|0;e=c[d>>2]|0;if(!e)break;else{a=e;b=d}}c[b>>2]=0;d=a}else{d=c[j+8>>2]|0;c[d+12>>2]=a;c[a+8>>2]=d;d=a}while(0);if(g|0){a=c[j+28>>2]|0;b=3316+(a<<2)|0;if((c[b>>2]|0)==(j|0)){c[b>>2]=d;if(!d){c[754]=c[754]&~(1<<a);break}}else{c[g+16+(((c[g+16>>2]|0)!=(j|0)&1)<<2)>>2]=d;if(!d)break}c[d+24>>2]=g;a=j+16|0;b=c[a>>2]|0;if(b|0){c[d+16>>2]=b;c[b+24>>2]=d}a=c[a+4>>2]|0;if(a|0){c[d+20>>2]=a;c[a+24>>2]=d}}}while(0);c[i+4>>2]=f|1;c[h+f>>2]=f;if((i|0)==(c[758]|0)){c[755]=f;return}}else{c[a>>2]=e&-2;c[i+4>>2]=b|1;c[h+b>>2]=b;f=b}a=f>>>3;if(f>>>0<256){d=3052+(a<<1<<2)|0;b=c[753]|0;a=1<<a;if(!(b&a)){c[753]=b|a;a=d;b=d+8|0}else{b=d+8|0;a=c[b>>2]|0}c[b>>2]=i;c[a+12>>2]=i;c[i+8>>2]=a;c[i+12>>2]=d;return}a=f>>>8;if(a)if(f>>>0>16777215)a=31;else{h=(a+1048320|0)>>>16&8;j=a<<h;g=(j+520192|0)>>>16&4;j=j<<g;a=(j+245760|0)>>>16&2;a=14-(g|h|a)+(j<<a>>>15)|0;a=f>>>(a+7|0)&1|a<<1}else a=0;e=3316+(a<<2)|0;c[i+28>>2]=a;c[i+20>>2]=0;c[i+16>>2]=0;b=c[754]|0;d=1<<a;do if(b&d){b=f<<((a|0)==31?0:25-(a>>>1)|0);d=c[e>>2]|0;while(1){if((c[d+4>>2]&-8|0)==(f|0)){a=73;break}e=d+16+(b>>>31<<2)|0;a=c[e>>2]|0;if(!a){a=72;break}else{b=b<<1;d=a}}if((a|0)==72){c[e>>2]=i;c[i+24>>2]=d;c[i+12>>2]=i;c[i+8>>2]=i;break}else if((a|0)==73){h=d+8|0;j=c[h>>2]|0;c[j+12>>2]=i;c[h>>2]=i;c[i+8>>2]=j;c[i+12>>2]=d;c[i+24>>2]=0;break}}else{c[754]=b|d;c[e>>2]=i;c[i+24>>2]=e;c[i+12>>2]=i;c[i+8>>2]=i}while(0);j=(c[761]|0)+-1|0;c[761]=j;if(!j)a=3468;else return;while(1){a=c[a>>2]|0;if(!a)break;else a=a+8|0}c[761]=-1;return}function Ga(a){a=a|0;var b=0,d=0;b=l;l=l+16|0;d=b;c[d>>2]=La(c[a+60>>2]|0)|0;a=Ja(ba(6,d|0)|0)|0;l=b;return a|0}function Ha(a,b,d){a=a|0;b=b|0;d=d|0;var e=0,f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0,p=0;n=l;l=l+48|0;k=n+16|0;g=n;f=n+32|0;i=a+28|0;e=c[i>>2]|0;c[f>>2]=e;j=a+20|0;e=(c[j>>2]|0)-e|0;c[f+4>>2]=e;c[f+8>>2]=b;c[f+12>>2]=d;e=e+d|0;h=a+60|0;c[g>>2]=c[h>>2];c[g+4>>2]=f;c[g+8>>2]=2;g=Ja($(146,g|0)|0)|0;a:do if((e|0)!=(g|0)){b=2;while(1){if((g|0)<0)break;e=e-g|0;p=c[f+4>>2]|0;o=g>>>0>p>>>0;f=o?f+8|0:f;b=b+(o<<31>>31)|0;p=g-(o?p:0)|0;c[f>>2]=(c[f>>2]|0)+p;o=f+4|0;c[o>>2]=(c[o>>2]|0)-p;c[k>>2]=c[h>>2];c[k+4>>2]=f;c[k+8>>2]=b;g=Ja($(146,k|0)|0)|0;if((e|0)==(g|0)){m=3;break a}}c[a+16>>2]=0;c[i>>2]=0;c[j>>2]=0;c[a>>2]=c[a>>2]|32;if((b|0)==2)d=0;else d=d-(c[f+4>>2]|0)|0}else m=3;while(0);if((m|0)==3){p=c[a+44>>2]|0;c[a+16>>2]=p+(c[a+48>>2]|0);c[i>>2]=p;c[j>>2]=p}l=n;return d|0}function Ia(a,b,d){a=a|0;b=b|0;d=d|0;var e=0,f=0,g=0;f=l;l=l+32|0;g=f;e=f+20|0;c[g>>2]=c[a+60>>2];c[g+4>>2]=0;c[g+8>>2]=b;c[g+12>>2]=e;c[g+16>>2]=d;if((Ja(_(140,g|0)|0)|0)<0){c[e>>2]=-1;a=-1}else a=c[e>>2]|0;l=f;return a|0}function Ja(a){a=a|0;if(a>>>0>4294963200){c[(Ka()|0)>>2]=0-a;a=-1}return a|0}function Ka(){return 3572}function La(a){a=a|0;return a|0}function Ma(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0;g=l;l=l+32|0;f=g;c[b+36>>2]=3;if((c[b>>2]&64|0)==0?(c[f>>2]=c[b+60>>2],c[f+4>>2]=21523,c[f+8>>2]=g+16,aa(54,f|0)|0):0)a[b+75>>0]=-1;f=Ha(b,d,e)|0;l=g;return f|0}function Na(b,c){b=b|0;c=c|0;var d=0,e=0;d=a[b>>0]|0;e=a[c>>0]|0;if(d<<24>>24==0?1:d<<24>>24!=e<<24>>24)b=e;else{do{b=b+1|0;c=c+1|0;d=a[b>>0]|0;e=a[c>>0]|0}while(!(d<<24>>24==0?1:d<<24>>24!=e<<24>>24));b=e}return (d&255)-(b&255)|0}function Oa(a){a=a|0;return (a+-48|0)>>>0<10|0}function Pa(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0,j=0,k=0,m=0,n=0,o=0,p=0,q=0,r=0,s=0;s=l;l=l+224|0;n=s+120|0;o=s+80|0;q=s;r=s+136|0;f=o;g=f+40|0;do{c[f>>2]=0;f=f+4|0}while((f|0)<(g|0));c[n>>2]=c[e>>2];if((Qa(0,d,n,q,o)|0)<0)e=-1;else{if((c[b+76>>2]|0)>-1)p=Ra(b)|0;else p=0;e=c[b>>2]|0;m=e&32;if((a[b+74>>0]|0)<1)c[b>>2]=e&-33;f=b+48|0;if(!(c[f>>2]|0)){g=b+44|0;h=c[g>>2]|0;c[g>>2]=r;i=b+28|0;c[i>>2]=r;j=b+20|0;c[j>>2]=r;c[f>>2]=80;k=b+16|0;c[k>>2]=r+80;e=Qa(b,d,n,q,o)|0;if(h){ga[c[b+36>>2]&3](b,0,0)|0;e=(c[j>>2]|0)==0?-1:e;c[g>>2]=h;c[f>>2]=0;c[k>>2]=0;c[i>>2]=0;c[j>>2]=0}}else e=Qa(b,d,n,q,o)|0;f=c[b>>2]|0;c[b>>2]=f|m;if(p|0)Sa(b);e=(f&32|0)==0?e:-1}l=s;return e|0}function Qa(d,e,f,g,i){d=d|0;e=e|0;f=f|0;g=g|0;i=i|0;var j=0,k=0,m=0,n=0,o=0,p=0,q=0,r=0,s=0,t=0,u=0,v=0,w=0,x=0,y=0,A=0,B=0,C=0,D=0,E=0,F=0,G=0,H=0,I=0;I=l;l=l+64|0;D=I+16|0;E=I;A=I+24|0;G=I+8|0;H=I+20|0;c[D>>2]=e;w=(d|0)!=0;x=A+40|0;y=x;A=A+39|0;B=G+4|0;j=0;e=0;m=0;a:while(1){do if((e|0)>-1)if((j|0)>(2147483647-e|0)){c[(Ka()|0)>>2]=75;e=-1;break}else{e=j+e|0;break}while(0);r=c[D>>2]|0;j=a[r>>0]|0;if(!(j<<24>>24)){v=88;break}else k=r;b:while(1){switch(j<<24>>24){case 37:{j=k;v=9;break b}case 0:{j=k;break b}default:{}}u=k+1|0;c[D>>2]=u;j=a[u>>0]|0;k=u}c:do if((v|0)==9)while(1){v=0;if((a[k+1>>0]|0)!=37)break c;j=j+1|0;k=k+2|0;c[D>>2]=k;if((a[k>>0]|0)!=37)break;else v=9}while(0);j=j-r|0;if(w)Ta(d,r,j);if(j|0)continue;u=(Oa(a[(c[D>>2]|0)+1>>0]|0)|0)==0;k=c[D>>2]|0;if(!u?(a[k+2>>0]|0)==36:0){s=(a[k+1>>0]|0)+-48|0;n=1;j=3}else{s=-1;n=m;j=1}j=k+j|0;c[D>>2]=j;k=a[j>>0]|0;u=(k<<24>>24)+-32|0;if(u>>>0>31|(1<<u&75913|0)==0)m=0;else{m=0;do{m=1<<(k<<24>>24)+-32|m;j=j+1|0;c[D>>2]=j;k=a[j>>0]|0;u=(k<<24>>24)+-32|0}while(!(u>>>0>31|(1<<u&75913|0)==0))}if(k<<24>>24==42){if((Oa(a[j+1>>0]|0)|0)!=0?(F=c[D>>2]|0,(a[F+2>>0]|0)==36):0){j=F+1|0;c[i+((a[j>>0]|0)+-48<<2)>>2]=10;j=c[g+((a[j>>0]|0)+-48<<3)>>2]|0;k=1;n=F+3|0}else{if(n|0){e=-1;break}if(w){u=(c[f>>2]|0)+(4-1)&~(4-1);j=c[u>>2]|0;c[f>>2]=u+4}else j=0;k=0;n=(c[D>>2]|0)+1|0}c[D>>2]=n;t=(j|0)<0;u=t?0-j|0:j;m=t?m|8192:m;t=k;j=n}else{j=Ua(D)|0;if((j|0)<0){e=-1;break}u=j;t=n;j=c[D>>2]|0}do if((a[j>>0]|0)==46){if((a[j+1>>0]|0)!=42){c[D>>2]=j+1;p=Ua(D)|0;j=c[D>>2]|0;break}if(Oa(a[j+2>>0]|0)|0?(C=c[D>>2]|0,(a[C+3>>0]|0)==36):0){p=C+2|0;c[i+((a[p>>0]|0)+-48<<2)>>2]=10;p=c[g+((a[p>>0]|0)+-48<<3)>>2]|0;j=C+4|0;c[D>>2]=j;break}if(t|0){e=-1;break a}if(w){q=(c[f>>2]|0)+(4-1)&~(4-1);j=c[q>>2]|0;c[f>>2]=q+4}else j=0;q=(c[D>>2]|0)+2|0;c[D>>2]=q;p=j;j=q}else p=-1;while(0);q=0;while(1){if(((a[j>>0]|0)+-65|0)>>>0>57){e=-1;break a}k=j;j=j+1|0;c[D>>2]=j;k=a[(a[k>>0]|0)+-65+(585+(q*58|0))>>0]|0;n=k&255;if((n+-1|0)>>>0>=8)break;else q=n}if(!(k<<24>>24)){e=-1;break}o=(s|0)>-1;do if(k<<24>>24==19)if(o){e=-1;break a}else v=50;else{if(o){c[i+(s<<2)>>2]=n;o=g+(s<<3)|0;s=c[o+4>>2]|0;v=E;c[v>>2]=c[o>>2];c[v+4>>2]=s;v=50;break}if(!w){e=0;break a}Va(E,n,f);j=c[D>>2]|0}while(0);if((v|0)==50){v=0;if(!w){j=0;m=t;continue}}k=a[j+-1>>0]|0;k=(q|0)!=0&(k&15|0)==3?k&-33:k;j=m&-65537;s=(m&8192|0)==0?m:j;d:do switch(k|0){case 110:switch((q&255)<<24>>24){case 0:{c[c[E>>2]>>2]=e;j=0;m=t;continue a}case 1:{c[c[E>>2]>>2]=e;j=0;m=t;continue a}case 2:{j=c[E>>2]|0;c[j>>2]=e;c[j+4>>2]=((e|0)<0)<<31>>31;j=0;m=t;continue a}case 3:{b[c[E>>2]>>1]=e;j=0;m=t;continue a}case 4:{a[c[E>>2]>>0]=e;j=0;m=t;continue a}case 6:{c[c[E>>2]>>2]=e;j=0;m=t;continue a}case 7:{j=c[E>>2]|0;c[j>>2]=e;c[j+4>>2]=((e|0)<0)<<31>>31;j=0;m=t;continue a}default:{j=0;m=t;continue a}}case 112:{k=120;j=p>>>0>8?p:8;m=s|8;v=62;break}case 88:case 120:{j=p;m=s;v=62;break}case 111:{k=E;j=c[k>>2]|0;k=c[k+4>>2]|0;o=Xa(j,k,x)|0;m=y-o|0;q=0;n=1049;p=(s&8|0)==0|(p|0)>(m|0)?p:m+1|0;m=s;v=68;break}case 105:case 100:{k=E;j=c[k>>2]|0;k=c[k+4>>2]|0;if((k|0)<0){j=tb(0,0,j|0,k|0)|0;k=z;m=E;c[m>>2]=j;c[m+4>>2]=k;m=1;n=1049;v=67;break d}else{m=(s&2049|0)!=0&1;n=(s&2048|0)==0?((s&1|0)==0?1049:1051):1050;v=67;break d}}case 117:{k=E;m=0;n=1049;j=c[k>>2]|0;k=c[k+4>>2]|0;v=67;break}case 99:{a[A>>0]=c[E>>2];r=A;q=0;n=1049;o=x;k=1;break}case 109:{k=Za(c[(Ka()|0)>>2]|0)|0;v=72;break}case 115:{k=c[E>>2]|0;k=k|0?k:1059;v=72;break}case 67:{c[G>>2]=c[E>>2];c[B>>2]=0;c[E>>2]=G;p=-1;m=G;v=76;break}case 83:{j=c[E>>2]|0;if(!p){$a(d,32,u,0,s);j=0;v=85}else{m=j;v=76}break}case 65:case 71:case 70:case 69:case 97:case 103:case 102:case 101:{j=bb(d,+h[E>>3],u,p,s,k)|0;m=t;continue a}default:{q=0;n=1049;o=x;k=p;j=s}}while(0);e:do if((v|0)==62){s=E;r=c[s>>2]|0;s=c[s+4>>2]|0;o=Wa(r,s,x,k&32)|0;n=(m&8|0)==0|(r|0)==0&(s|0)==0;q=n?0:2;n=n?1049:1049+(k>>4)|0;p=j;j=r;k=s;v=68}else if((v|0)==67){o=Ya(j,k,x)|0;q=m;m=s;v=68}else if((v|0)==72){v=0;s=_a(k,0,p)|0;m=(s|0)==0;r=k;q=0;n=1049;o=m?k+p|0:s;k=m?p:s-k|0}else if((v|0)==76){v=0;o=m;j=0;k=0;while(1){n=c[o>>2]|0;if(!n)break;k=ab(H,n)|0;if((k|0)<0|k>>>0>(p-j|0)>>>0)break;j=k+j|0;if(p>>>0>j>>>0)o=o+4|0;else break}if((k|0)<0){e=-1;break a}$a(d,32,u,j,s);if(!j){j=0;v=85}else{n=0;while(1){k=c[m>>2]|0;if(!k){v=85;break e}k=ab(H,k)|0;n=k+n|0;if((n|0)>(j|0)){v=85;break e}Ta(d,H,k);if(n>>>0>=j>>>0){v=85;break}else m=m+4|0}}}while(0);if((v|0)==68){v=0;k=(j|0)!=0|(k|0)!=0;j=(p|0)!=0|k;k=y-o+((k^1)&1)|0;r=j?o:x;o=x;k=j?((p|0)>(k|0)?p:k):p;j=(p|0)>-1?m&-65537:m}else if((v|0)==85){v=0;$a(d,32,u,j,s^8192);j=(u|0)>(j|0)?u:j;m=t;continue}p=o-r|0;o=(k|0)<(p|0)?p:k;s=o+q|0;m=(u|0)<(s|0)?s:u;$a(d,32,m,s,j);Ta(d,n,q);$a(d,48,m,s,j^65536);$a(d,48,o,p,0);Ta(d,r,p);$a(d,32,m,s,j^8192);j=m;m=t}f:do if((v|0)==88)if(!d)if(m){e=1;while(1){j=c[i+(e<<2)>>2]|0;if(!j)break;Va(g+(e<<3)|0,j,f);j=e+1|0;if((e|0)<9)e=j;else{e=j;break}}if((e|0)<10)while(1){if(c[i+(e<<2)>>2]|0){e=-1;break f}if((e|0)<9)e=e+1|0;else{e=1;break}}else e=1}else e=0;while(0);l=I;return e|0}function Ra(a){a=a|0;return 0}function Sa(a){a=a|0;return}function Ta(a,b,d){a=a|0;b=b|0;d=d|0;if(!(c[a>>2]&32))ob(b,d,a)|0;return}function Ua(b){b=b|0;var d=0,e=0;if(!(Oa(a[c[b>>2]>>0]|0)|0))d=0;else{d=0;do{e=c[b>>2]|0;d=(d*10|0)+-48+(a[e>>0]|0)|0;e=e+1|0;c[b>>2]=e}while((Oa(a[e>>0]|0)|0)!=0)}return d|0}function Va(a,b,d){a=a|0;b=b|0;d=d|0;var e=0,f=0,g=0.0;a:do if(b>>>0<=20)do switch(b|0){case 9:{e=(c[d>>2]|0)+(4-1)&~(4-1);b=c[e>>2]|0;c[d>>2]=e+4;c[a>>2]=b;break a}case 10:{e=(c[d>>2]|0)+(4-1)&~(4-1);b=c[e>>2]|0;c[d>>2]=e+4;e=a;c[e>>2]=b;c[e+4>>2]=((b|0)<0)<<31>>31;break a}case 11:{e=(c[d>>2]|0)+(4-1)&~(4-1);b=c[e>>2]|0;c[d>>2]=e+4;e=a;c[e>>2]=b;c[e+4>>2]=0;break a}case 12:{e=(c[d>>2]|0)+(8-1)&~(8-1);b=e;f=c[b>>2]|0;b=c[b+4>>2]|0;c[d>>2]=e+8;e=a;c[e>>2]=f;c[e+4>>2]=b;break a}case 13:{f=(c[d>>2]|0)+(4-1)&~(4-1);e=c[f>>2]|0;c[d>>2]=f+4;e=(e&65535)<<16>>16;f=a;c[f>>2]=e;c[f+4>>2]=((e|0)<0)<<31>>31;break a}case 14:{f=(c[d>>2]|0)+(4-1)&~(4-1);e=c[f>>2]|0;c[d>>2]=f+4;f=a;c[f>>2]=e&65535;c[f+4>>2]=0;break a}case 15:{f=(c[d>>2]|0)+(4-1)&~(4-1);e=c[f>>2]|0;c[d>>2]=f+4;e=(e&255)<<24>>24;f=a;c[f>>2]=e;c[f+4>>2]=((e|0)<0)<<31>>31;break a}case 16:{f=(c[d>>2]|0)+(4-1)&~(4-1);e=c[f>>2]|0;c[d>>2]=f+4;f=a;c[f>>2]=e&255;c[f+4>>2]=0;break a}case 17:{f=(c[d>>2]|0)+(8-1)&~(8-1);g=+h[f>>3];c[d>>2]=f+8;h[a>>3]=g;break a}case 18:{f=(c[d>>2]|0)+(8-1)&~(8-1);g=+h[f>>3];c[d>>2]=f+8;h[a>>3]=g;break a}default:break a}while(0);while(0);return}function Wa(b,c,e,f){b=b|0;c=c|0;e=e|0;f=f|0;if(!((b|0)==0&(c|0)==0))do{e=e+-1|0;a[e>>0]=d[1101+(b&15)>>0]|0|f;b=yb(b|0,c|0,4)|0;c=z}while(!((b|0)==0&(c|0)==0));return e|0}function Xa(b,c,d){b=b|0;c=c|0;d=d|0;if(!((b|0)==0&(c|0)==0))do{d=d+-1|0;a[d>>0]=b&7|48;b=yb(b|0,c|0,3)|0;c=z}while(!((b|0)==0&(c|0)==0));return d|0}function Ya(b,c,d){b=b|0;c=c|0;d=d|0;var e=0;if(c>>>0>0|(c|0)==0&b>>>0>4294967295){while(1){e=xb(b|0,c|0,10,0)|0;d=d+-1|0;a[d>>0]=e&255|48;e=b;b=wb(b|0,c|0,10,0)|0;if(!(c>>>0>9|(c|0)==9&e>>>0>4294967295))break;else c=z}c=b}else c=b;if(c)while(1){d=d+-1|0;a[d>>0]=(c>>>0)%10|0|48;if(c>>>0<10)break;else c=(c>>>0)/10|0}return d|0}function Za(a){a=a|0;return jb(a,c[(ib()|0)+188>>2]|0)|0}function _a(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0;h=d&255;f=(e|0)!=0;a:do if(f&(b&3|0)!=0){g=d&255;while(1){if((a[b>>0]|0)==g<<24>>24){i=6;break a}b=b+1|0;e=e+-1|0;f=(e|0)!=0;if(!(f&(b&3|0)!=0)){i=5;break}}}else i=5;while(0);if((i|0)==5)if(f)i=6;else e=0;b:do if((i|0)==6){g=d&255;if((a[b>>0]|0)!=g<<24>>24){f=O(h,16843009)|0;c:do if(e>>>0>3)while(1){h=c[b>>2]^f;if((h&-2139062144^-2139062144)&h+-16843009|0)break;b=b+4|0;e=e+-4|0;if(e>>>0<=3){i=11;break c}}else i=11;while(0);if((i|0)==11)if(!e){e=0;break}while(1){if((a[b>>0]|0)==g<<24>>24)break b;b=b+1|0;e=e+-1|0;if(!e){e=0;break}}}}while(0);return (e|0?b:0)|0}function $a(a,b,c,d,e){a=a|0;b=b|0;c=c|0;d=d|0;e=e|0;var f=0,g=0;g=l;l=l+256|0;f=g;if((c|0)>(d|0)&(e&73728|0)==0){e=c-d|0;Cb(f|0,b<<24>>24|0,(e>>>0<256?e:256)|0)|0;if(e>>>0>255){b=c-d|0;do{Ta(a,f,256);e=e+-256|0}while(e>>>0>255);e=b&255}Ta(a,f,e)}l=g;return}function ab(a,b){a=a|0;b=b|0;if(!a)a=0;else a=fb(a,b,0)|0;return a|0}function bb(b,e,f,g,h,i){b=b|0;e=+e;f=f|0;g=g|0;h=h|0;i=i|0;var j=0,k=0,m=0,n=0,o=0,p=0,q=0,r=0.0,s=0,t=0,u=0,v=0,w=0,x=0,y=0,A=0,B=0,C=0,D=0,E=0,F=0,G=0,H=0;H=l;l=l+560|0;m=H+8|0;u=H;G=H+524|0;F=G;n=H+512|0;c[u>>2]=0;E=n+12|0;cb(e)|0;if((z|0)<0){e=-e;C=1;B=1066}else{C=(h&2049|0)!=0&1;B=(h&2048|0)==0?((h&1|0)==0?1067:1072):1069}cb(e)|0;do if(0==0&(z&2146435072|0)==2146435072){G=(i&32|0)!=0;j=C+3|0;$a(b,32,f,j,h&-65537);Ta(b,B,C);Ta(b,e!=e|0.0!=0.0?(G?1093:1097):G?1085:1089,3);$a(b,32,f,j,h^8192)}else{r=+db(e,u)*2.0;j=r!=0.0;if(j)c[u>>2]=(c[u>>2]|0)+-1;w=i|32;if((w|0)==97){p=i&32;s=(p|0)==0?B:B+9|0;q=C|2;j=12-g|0;do if(!(g>>>0>11|(j|0)==0)){e=8.0;do{j=j+-1|0;e=e*16.0}while((j|0)!=0);if((a[s>>0]|0)==45){e=-(e+(-r-e));break}else{e=r+e-e;break}}else e=r;while(0);k=c[u>>2]|0;j=(k|0)<0?0-k|0:k;j=Ya(j,((j|0)<0)<<31>>31,E)|0;if((j|0)==(E|0)){j=n+11|0;a[j>>0]=48}a[j+-1>>0]=(k>>31&2)+43;o=j+-2|0;a[o>>0]=i+15;m=(g|0)<1;n=(h&8|0)==0;j=G;do{D=~~e;k=j+1|0;a[j>>0]=p|d[1101+D>>0];e=(e-+(D|0))*16.0;if((k-F|0)==1?!(n&(m&e==0.0)):0){a[k>>0]=46;j=j+2|0}else j=k}while(e!=0.0);if((g|0)!=0?(-2-F+j|0)<(g|0):0){k=j-F|0;j=g+2|0}else{j=j-F|0;k=j}E=E-o|0;F=E+q+j|0;$a(b,32,f,F,h);Ta(b,s,q);$a(b,48,f,F,h^65536);Ta(b,G,k);$a(b,48,j-k|0,0,0);Ta(b,o,E);$a(b,32,f,F,h^8192);j=F;break}k=(g|0)<0?6:g;if(j){j=(c[u>>2]|0)+-28|0;c[u>>2]=j;e=r*268435456.0}else{e=r;j=c[u>>2]|0}D=(j|0)<0?m:m+288|0;m=D;do{y=~~e>>>0;c[m>>2]=y;m=m+4|0;e=(e-+(y>>>0))*1.0e9}while(e!=0.0);if((j|0)>0){n=D;p=m;while(1){o=(j|0)<29?j:29;j=p+-4|0;if(j>>>0>=n>>>0){m=0;do{x=zb(c[j>>2]|0,0,o|0)|0;x=sb(x|0,z|0,m|0,0)|0;y=z;v=xb(x|0,y|0,1e9,0)|0;c[j>>2]=v;m=wb(x|0,y|0,1e9,0)|0;j=j+-4|0}while(j>>>0>=n>>>0);if(m){n=n+-4|0;c[n>>2]=m}}m=p;while(1){if(m>>>0<=n>>>0)break;j=m+-4|0;if(!(c[j>>2]|0))m=j;else break}j=(c[u>>2]|0)-o|0;c[u>>2]=j;if((j|0)>0)p=m;else break}}else n=D;if((j|0)<0){g=((k+25|0)/9|0)+1|0;t=(w|0)==102;do{s=0-j|0;s=(s|0)<9?s:9;if(n>>>0<m>>>0){o=(1<<s)+-1|0;p=1e9>>>s;q=0;j=n;do{y=c[j>>2]|0;c[j>>2]=(y>>>s)+q;q=O(y&o,p)|0;j=j+4|0}while(j>>>0<m>>>0);j=(c[n>>2]|0)==0?n+4|0:n;if(!q){n=j;j=m}else{c[m>>2]=q;n=j;j=m+4|0}}else{n=(c[n>>2]|0)==0?n+4|0:n;j=m}m=t?D:n;m=(j-m>>2|0)>(g|0)?m+(g<<2)|0:j;j=(c[u>>2]|0)+s|0;c[u>>2]=j}while((j|0)<0);j=n;g=m}else{j=n;g=m}y=D;if(j>>>0<g>>>0){m=(y-j>>2)*9|0;o=c[j>>2]|0;if(o>>>0>=10){n=10;do{n=n*10|0;m=m+1|0}while(o>>>0>=n>>>0)}}else m=0;t=(w|0)==103;v=(k|0)!=0;n=k-((w|0)!=102?m:0)+((v&t)<<31>>31)|0;if((n|0)<(((g-y>>2)*9|0)+-9|0)){n=n+9216|0;s=D+4+(((n|0)/9|0)+-1024<<2)|0;n=(n|0)%9|0;if((n|0)<8){o=10;while(1){o=o*10|0;if((n|0)<7)n=n+1|0;else break}}else o=10;p=c[s>>2]|0;q=(p>>>0)%(o>>>0)|0;n=(s+4|0)==(g|0);if(!(n&(q|0)==0)){r=(((p>>>0)/(o>>>0)|0)&1|0)==0?9007199254740992.0:9007199254740994.0;x=(o|0)/2|0;e=q>>>0<x>>>0?.5:n&(q|0)==(x|0)?1.0:1.5;if(C){x=(a[B>>0]|0)==45;e=x?-e:e;r=x?-r:r}n=p-q|0;c[s>>2]=n;if(r+e!=r){x=n+o|0;c[s>>2]=x;if(x>>>0>999999999){m=s;while(1){n=m+-4|0;c[m>>2]=0;if(n>>>0<j>>>0){j=j+-4|0;c[j>>2]=0}x=(c[n>>2]|0)+1|0;c[n>>2]=x;if(x>>>0>999999999)m=n;else break}}else n=s;m=(y-j>>2)*9|0;p=c[j>>2]|0;if(p>>>0>=10){o=10;do{o=o*10|0;m=m+1|0}while(p>>>0>=o>>>0)}}else n=s}else n=s;n=n+4|0;n=g>>>0>n>>>0?n:g;x=j}else{n=g;x=j}w=n;while(1){if(w>>>0<=x>>>0){u=0;break}j=w+-4|0;if(!(c[j>>2]|0))w=j;else{u=1;break}}g=0-m|0;do if(t){j=k+((v^1)&1)|0;if((j|0)>(m|0)&(m|0)>-5){o=i+-1|0;k=j+-1-m|0}else{o=i+-2|0;k=j+-1|0}j=h&8;if(!j){if(u?(A=c[w+-4>>2]|0,(A|0)!=0):0)if(!((A>>>0)%10|0)){n=0;j=10;do{j=j*10|0;n=n+1|0}while(!((A>>>0)%(j>>>0)|0|0))}else n=0;else n=9;j=((w-y>>2)*9|0)+-9|0;if((o|32|0)==102){s=j-n|0;s=(s|0)>0?s:0;k=(k|0)<(s|0)?k:s;s=0;break}else{s=j+m-n|0;s=(s|0)>0?s:0;k=(k|0)<(s|0)?k:s;s=0;break}}else s=j}else{o=i;s=h&8}while(0);t=k|s;p=(t|0)!=0&1;q=(o|32|0)==102;if(q){v=0;j=(m|0)>0?m:0}else{j=(m|0)<0?g:m;j=Ya(j,((j|0)<0)<<31>>31,E)|0;n=E;if((n-j|0)<2)do{j=j+-1|0;a[j>>0]=48}while((n-j|0)<2);a[j+-1>>0]=(m>>31&2)+43;j=j+-2|0;a[j>>0]=o;v=j;j=n-j|0}j=C+1+k+p+j|0;$a(b,32,f,j,h);Ta(b,B,C);$a(b,48,f,j,h^65536);if(q){o=x>>>0>D>>>0?D:x;s=G+9|0;p=s;q=G+8|0;n=o;do{m=Ya(c[n>>2]|0,0,s)|0;if((n|0)==(o|0)){if((m|0)==(s|0)){a[q>>0]=48;m=q}}else if(m>>>0>G>>>0){Cb(G|0,48,m-F|0)|0;do m=m+-1|0;while(m>>>0>G>>>0)}Ta(b,m,p-m|0);n=n+4|0}while(n>>>0<=D>>>0);if(t|0)Ta(b,1117,1);if(n>>>0<w>>>0&(k|0)>0)while(1){m=Ya(c[n>>2]|0,0,s)|0;if(m>>>0>G>>>0){Cb(G|0,48,m-F|0)|0;do m=m+-1|0;while(m>>>0>G>>>0)}Ta(b,m,(k|0)<9?k:9);n=n+4|0;m=k+-9|0;if(!(n>>>0<w>>>0&(k|0)>9)){k=m;break}else k=m}$a(b,48,k+9|0,9,0)}else{t=u?w:x+4|0;if((k|0)>-1){u=G+9|0;s=(s|0)==0;g=u;p=0-F|0;q=G+8|0;o=x;do{m=Ya(c[o>>2]|0,0,u)|0;if((m|0)==(u|0)){a[q>>0]=48;m=q}do if((o|0)==(x|0)){n=m+1|0;Ta(b,m,1);if(s&(k|0)<1){m=n;break}Ta(b,1117,1);m=n}else{if(m>>>0<=G>>>0)break;Cb(G|0,48,m+p|0)|0;do m=m+-1|0;while(m>>>0>G>>>0)}while(0);F=g-m|0;Ta(b,m,(k|0)>(F|0)?F:k);k=k-F|0;o=o+4|0}while(o>>>0<t>>>0&(k|0)>-1)}$a(b,48,k+18|0,18,0);Ta(b,v,E-v|0)}$a(b,32,f,j,h^8192)}while(0);l=H;return ((j|0)<(f|0)?f:j)|0}function cb(a){a=+a;var b=0;h[j>>3]=a;b=c[j>>2]|0;z=c[j+4>>2]|0;return b|0}function db(a,b){a=+a;b=b|0;return +(+eb(a,b))}function eb(a,b){a=+a;b=b|0;var d=0,e=0,f=0;h[j>>3]=a;d=c[j>>2]|0;e=c[j+4>>2]|0;f=yb(d|0,e|0,52)|0;switch(f&2047){case 0:{if(a!=0.0){a=+eb(a*18446744073709551616.0,b);d=(c[b>>2]|0)+-64|0}else d=0;c[b>>2]=d;break}case 2047:break;default:{c[b>>2]=(f&2047)+-1022;c[j>>2]=d;c[j+4>>2]=e&-2146435073|1071644672;a=+h[j>>3]}}return +a}function fb(b,d,e){b=b|0;d=d|0;e=e|0;do if(b){if(d>>>0<128){a[b>>0]=d;b=1;break}if(!(c[c[(gb()|0)+188>>2]>>2]|0))if((d&-128|0)==57216){a[b>>0]=d;b=1;break}else{c[(Ka()|0)>>2]=84;b=-1;break}if(d>>>0<2048){a[b>>0]=d>>>6|192;a[b+1>>0]=d&63|128;b=2;break}if(d>>>0<55296|(d&-8192|0)==57344){a[b>>0]=d>>>12|224;a[b+1>>0]=d>>>6&63|128;a[b+2>>0]=d&63|128;b=3;break}if((d+-65536|0)>>>0<1048576){a[b>>0]=d>>>18|240;a[b+1>>0]=d>>>12&63|128;a[b+2>>0]=d>>>6&63|128;a[b+3>>0]=d&63|128;b=4;break}else{c[(Ka()|0)>>2]=84;b=-1;break}}else b=1;while(0);return b|0}function gb(){return hb()|0}function hb(){return 332}function ib(){return hb()|0}function jb(b,e){b=b|0;e=e|0;var f=0,g=0;g=0;while(1){if((d[1119+g>>0]|0)==(b|0)){b=2;break}f=g+1|0;if((f|0)==87){f=1207;g=87;b=5;break}else g=f}if((b|0)==2)if(!g)f=1207;else{f=1207;b=5}if((b|0)==5)while(1){do{b=f;f=f+1|0}while((a[b>>0]|0)!=0);g=g+-1|0;if(!g)break;else b=5}return kb(f,c[e+20>>2]|0)|0}function kb(a,b){a=a|0;b=b|0;return lb(a,b)|0}function lb(a,b){a=a|0;b=b|0;if(!b)b=0;else b=mb(c[b>>2]|0,c[b+4>>2]|0,a)|0;return (b|0?b:a)|0}function mb(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0,j=0,k=0,l=0,m=0,n=0,o=0;o=(c[b>>2]|0)+1794895138|0;h=nb(c[b+8>>2]|0,o)|0;f=nb(c[b+12>>2]|0,o)|0;g=nb(c[b+16>>2]|0,o)|0;a:do if((h>>>0<d>>>2>>>0?(n=d-(h<<2)|0,f>>>0<n>>>0&g>>>0<n>>>0):0)?((g|f)&3|0)==0:0){n=f>>>2;m=g>>>2;l=0;while(1){j=h>>>1;k=l+j|0;i=k<<1;g=i+n|0;f=nb(c[b+(g<<2)>>2]|0,o)|0;g=nb(c[b+(g+1<<2)>>2]|0,o)|0;if(!(g>>>0<d>>>0&f>>>0<(d-g|0)>>>0)){f=0;break a}if(a[b+(g+f)>>0]|0){f=0;break a}f=Na(e,b+g|0)|0;if(!f)break;f=(f|0)<0;if((h|0)==1){f=0;break a}else{l=f?l:k;h=f?j:h-j|0}}f=i+m|0;g=nb(c[b+(f<<2)>>2]|0,o)|0;f=nb(c[b+(f+1<<2)>>2]|0,o)|0;if(f>>>0<d>>>0&g>>>0<(d-f|0)>>>0)f=(a[b+(f+g)>>0]|0)==0?b+f|0:0;else f=0}else f=0;while(0);return f|0}function nb(a,b){a=a|0;b=b|0;var c=0;c=Ab(a|0)|0;return ((b|0)==0?a:c)|0}function ob(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0,j=0;f=e+16|0;g=c[f>>2]|0;if(!g)if(!(pb(e)|0)){g=c[f>>2]|0;h=5}else f=0;else h=5;a:do if((h|0)==5){j=e+20|0;i=c[j>>2]|0;f=i;if((g-i|0)>>>0<d>>>0){f=ga[c[e+36>>2]&3](e,b,d)|0;break}b:do if((a[e+75>>0]|0)>-1){i=d;while(1){if(!i){h=0;g=b;break b}g=i+-1|0;if((a[b+g>>0]|0)==10)break;else i=g}f=ga[c[e+36>>2]&3](e,b,i)|0;if(f>>>0<i>>>0)break a;h=i;g=b+i|0;d=d-i|0;f=c[j>>2]|0}else{h=0;g=b}while(0);Bb(f|0,g|0,d|0)|0;c[j>>2]=(c[j>>2]|0)+d;f=h+d|0}while(0);return f|0}function pb(b){b=b|0;var d=0,e=0;d=b+74|0;e=a[d>>0]|0;a[d>>0]=e+255|e;d=c[b>>2]|0;if(!(d&8)){c[b+8>>2]=0;c[b+4>>2]=0;e=c[b+44>>2]|0;c[b+28>>2]=e;c[b+20>>2]=e;c[b+16>>2]=e+(c[b+48>>2]|0);b=0}else{c[b>>2]=d|32;b=-1}return b|0}function qb(a,b){a=a|0;b=b|0;var d=0,e=0;d=l;l=l+16|0;e=d;c[e>>2]=b;b=Pa(c[51]|0,a,e)|0;l=d;return b|0}function rb(){}function sb(a,b,c,d){a=a|0;b=b|0;c=c|0;d=d|0;c=a+c>>>0;return (z=b+d+(c>>>0<a>>>0|0)>>>0,c|0)|0}function tb(a,b,c,d){a=a|0;b=b|0;c=c|0;d=d|0;d=b-d-(c>>>0>a>>>0|0)>>>0;return (z=d,a-c>>>0|0)|0}function ub(b){b=b|0;var c=0;c=a[n+(b&255)>>0]|0;if((c|0)<8)return c|0;c=a[n+(b>>8&255)>>0]|0;if((c|0)<8)return c+8|0;c=a[n+(b>>16&255)>>0]|0;if((c|0)<8)return c+16|0;return (a[n+(b>>>24)>>0]|0)+24|0}function vb(a,b,d,e,f){a=a|0;b=b|0;d=d|0;e=e|0;f=f|0;var g=0,h=0,i=0,j=0,k=0,l=0,m=0,n=0,o=0,p=0;l=a;j=b;k=j;h=d;n=e;i=n;if(!k){g=(f|0)!=0;if(!i){if(g){c[f>>2]=(l>>>0)%(h>>>0);c[f+4>>2]=0}n=0;f=(l>>>0)/(h>>>0)>>>0;return (z=n,f)|0}else{if(!g){n=0;f=0;return (z=n,f)|0}c[f>>2]=a|0;c[f+4>>2]=b&0;n=0;f=0;return (z=n,f)|0}}g=(i|0)==0;do if(h){if(!g){g=(R(i|0)|0)-(R(k|0)|0)|0;if(g>>>0<=31){m=g+1|0;i=31-g|0;b=g-31>>31;h=m;a=l>>>(m>>>0)&b|k<<i;b=k>>>(m>>>0)&b;g=0;i=l<<i;break}if(!f){n=0;f=0;return (z=n,f)|0}c[f>>2]=a|0;c[f+4>>2]=j|b&0;n=0;f=0;return (z=n,f)|0}g=h-1|0;if(g&h|0){i=(R(h|0)|0)+33-(R(k|0)|0)|0;p=64-i|0;m=32-i|0;j=m>>31;o=i-32|0;b=o>>31;h=i;a=m-1>>31&k>>>(o>>>0)|(k<<m|l>>>(i>>>0))&b;b=b&k>>>(i>>>0);g=l<<p&j;i=(k<<p|l>>>(o>>>0))&j|l<<m&i-33>>31;break}if(f|0){c[f>>2]=g&l;c[f+4>>2]=0}if((h|0)==1){o=j|b&0;p=a|0|0;return (z=o,p)|0}else{p=ub(h|0)|0;o=k>>>(p>>>0)|0;p=k<<32-p|l>>>(p>>>0)|0;return (z=o,p)|0}}else{if(g){if(f|0){c[f>>2]=(k>>>0)%(h>>>0);c[f+4>>2]=0}o=0;p=(k>>>0)/(h>>>0)>>>0;return (z=o,p)|0}if(!l){if(f|0){c[f>>2]=0;c[f+4>>2]=(k>>>0)%(i>>>0)}o=0;p=(k>>>0)/(i>>>0)>>>0;return (z=o,p)|0}g=i-1|0;if(!(g&i)){if(f|0){c[f>>2]=a|0;c[f+4>>2]=g&k|b&0}o=0;p=k>>>((ub(i|0)|0)>>>0);return (z=o,p)|0}g=(R(i|0)|0)-(R(k|0)|0)|0;if(g>>>0<=30){b=g+1|0;i=31-g|0;h=b;a=k<<i|l>>>(b>>>0);b=k>>>(b>>>0);g=0;i=l<<i;break}if(!f){o=0;p=0;return (z=o,p)|0}c[f>>2]=a|0;c[f+4>>2]=j|b&0;o=0;p=0;return (z=o,p)|0}while(0);if(!h){k=i;j=0;i=0}else{m=d|0|0;l=n|e&0;k=sb(m|0,l|0,-1,-1)|0;d=z;j=i;i=0;do{e=j;j=g>>>31|j<<1;g=i|g<<1;e=a<<1|e>>>31|0;n=a>>>31|b<<1|0;tb(k|0,d|0,e|0,n|0)|0;p=z;o=p>>31|((p|0)<0?-1:0)<<1;i=o&1;a=tb(e|0,n|0,o&m|0,(((p|0)<0?-1:0)>>31|((p|0)<0?-1:0)<<1)&l|0)|0;b=z;h=h-1|0}while((h|0)!=0);k=j;j=0}h=0;if(f|0){c[f>>2]=a;c[f+4>>2]=b}o=(g|0)>>>31|(k|h)<<1|(h<<1|g>>>31)&0|j;p=(g<<1|0>>>31)&-2|i;return (z=o,p)|0}function wb(a,b,c,d){a=a|0;b=b|0;c=c|0;d=d|0;return vb(a,b,c,d,0)|0}function xb(a,b,d,e){a=a|0;b=b|0;d=d|0;e=e|0;var f=0,g=0;g=l;l=l+16|0;f=g|0;vb(a,b,d,e,f)|0;l=g;return (z=c[f+4>>2]|0,c[f>>2]|0)|0}function yb(a,b,c){a=a|0;b=b|0;c=c|0;if((c|0)<32){z=b>>>c;return a>>>c|(b&(1<<c)-1)<<32-c}z=0;return b>>>c-32|0}function zb(a,b,c){a=a|0;b=b|0;c=c|0;if((c|0)<32){z=b<<c|(a&(1<<c)-1<<32-c)>>>32-c;return a<<c}z=a<<c-32;return 0}function Ab(a){a=a|0;return (a&255)<<24|(a>>8&255)<<16|(a>>16&255)<<8|a>>>24|0}function Bb(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0;if((e|0)>=8192)return ca(b|0,d|0,e|0)|0;h=b|0;g=b+e|0;if((b&3)==(d&3)){while(b&3){if(!e)return h|0;a[b>>0]=a[d>>0]|0;b=b+1|0;d=d+1|0;e=e-1|0}e=g&-4|0;f=e-64|0;while((b|0)<=(f|0)){c[b>>2]=c[d>>2];c[b+4>>2]=c[d+4>>2];c[b+8>>2]=c[d+8>>2];c[b+12>>2]=c[d+12>>2];c[b+16>>2]=c[d+16>>2];c[b+20>>2]=c[d+20>>2];c[b+24>>2]=c[d+24>>2];c[b+28>>2]=c[d+28>>2];c[b+32>>2]=c[d+32>>2];c[b+36>>2]=c[d+36>>2];c[b+40>>2]=c[d+40>>2];c[b+44>>2]=c[d+44>>2];c[b+48>>2]=c[d+48>>2];c[b+52>>2]=c[d+52>>2];c[b+56>>2]=c[d+56>>2];c[b+60>>2]=c[d+60>>2];b=b+64|0;d=d+64|0}while((b|0)<(e|0)){c[b>>2]=c[d>>2];b=b+4|0;d=d+4|0}}else{e=g-4|0;while((b|0)<(e|0)){a[b>>0]=a[d>>0]|0;a[b+1>>0]=a[d+1>>0]|0;a[b+2>>0]=a[d+2>>0]|0;a[b+3>>0]=a[d+3>>0]|0;b=b+4|0;d=d+4|0}}while((b|0)<(g|0)){a[b>>0]=a[d>>0]|0;b=b+1|0;d=d+1|0}return h|0}function Cb(b,d,e){b=b|0;d=d|0;e=e|0;var f=0,g=0,h=0,i=0;h=b+e|0;d=d&255;if((e|0)>=67){while(b&3){a[b>>0]=d;b=b+1|0}f=h&-4|0;g=f-64|0;i=d|d<<8|d<<16|d<<24;while((b|0)<=(g|0)){c[b>>2]=i;c[b+4>>2]=i;c[b+8>>2]=i;c[b+12>>2]=i;c[b+16>>2]=i;c[b+20>>2]=i;c[b+24>>2]=i;c[b+28>>2]=i;c[b+32>>2]=i;c[b+36>>2]=i;c[b+40>>2]=i;c[b+44>>2]=i;c[b+48>>2]=i;c[b+52>>2]=i;c[b+56>>2]=i;c[b+60>>2]=i;b=b+64|0}while((b|0)<(f|0)){c[b>>2]=i;b=b+4|0}}while((b|0)<(h|0)){a[b>>0]=d;b=b+1|0}return h-e|0}function Db(a){a=a|0;var b=0,d=0;d=c[i>>2]|0;b=d+a|0;if((a|0)>0&(b|0)<(d|0)|(b|0)<0){W()|0;Z(12);return -1}c[i>>2]=b;if((b|0)>(V()|0)?(U()|0)==0:0){c[i>>2]=d;Z(12);return -1}return d|0}function Eb(a,b){a=a|0;b=b|0;return fa[a&1](b|0)|0}function Fb(a,b,c,d){a=a|0;b=b|0;c=c|0;d=d|0;return ga[a&3](b|0,c|0,d|0)|0}function Gb(a){a=a|0;S(0);return 0}function Hb(a,b,c){a=a|0;b=b|0;c=c|0;S(1);return 0}

// EMSCRIPTEN_END_FUNCS
var fa=[Gb,Ga];var ga=[Hb,Ma,Ia,Ha];return{_KangarooTwelve_Final:ya,_KangarooTwelve_Initialize:wa,_KangarooTwelve_IsAbsorbing:Ca,_KangarooTwelve_IsSqueezing:Ba,_KangarooTwelve_Squeeze:za,_KangarooTwelve_Update:xa,_KangarooTwelve_phase:Da,_NewKangarooTwelve:Aa,___errno_location:Ka,___udivdi3:wb,___uremdi3:xb,_bitshift64Lshr:yb,_bitshift64Shl:zb,_free:Fa,_i64Add:sb,_i64Subtract:tb,_llvm_bswap_i32:Ab,_malloc:Ea,_memcpy:Bb,_memset:Cb,_sbrk:Db,dynCall_ii:Eb,dynCall_iiii:Fb,establishStackSpace:ka,getTempRet0:na,runPostSets:rb,setTempRet0:ma,setThrew:la,stackAlloc:ha,stackRestore:ja,stackSave:ia}})


// EMSCRIPTEN_END_ASM
(Module.asmGlobalArg,Module.asmLibraryArg,buffer);var _KangarooTwelve_Final=Module["_KangarooTwelve_Final"]=asm["_KangarooTwelve_Final"];var _KangarooTwelve_Initialize=Module["_KangarooTwelve_Initialize"]=asm["_KangarooTwelve_Initialize"];var _KangarooTwelve_IsAbsorbing=Module["_KangarooTwelve_IsAbsorbing"]=asm["_KangarooTwelve_IsAbsorbing"];var _KangarooTwelve_IsSqueezing=Module["_KangarooTwelve_IsSqueezing"]=asm["_KangarooTwelve_IsSqueezing"];var _KangarooTwelve_Squeeze=Module["_KangarooTwelve_Squeeze"]=asm["_KangarooTwelve_Squeeze"];var _KangarooTwelve_Update=Module["_KangarooTwelve_Update"]=asm["_KangarooTwelve_Update"];var _KangarooTwelve_phase=Module["_KangarooTwelve_phase"]=asm["_KangarooTwelve_phase"];var _NewKangarooTwelve=Module["_NewKangarooTwelve"]=asm["_NewKangarooTwelve"];var ___errno_location=Module["___errno_location"]=asm["___errno_location"];var ___udivdi3=Module["___udivdi3"]=asm["___udivdi3"];var ___uremdi3=Module["___uremdi3"]=asm["___uremdi3"];var _bitshift64Lshr=Module["_bitshift64Lshr"]=asm["_bitshift64Lshr"];var _bitshift64Shl=Module["_bitshift64Shl"]=asm["_bitshift64Shl"];var _free=Module["_free"]=asm["_free"];var _i64Add=Module["_i64Add"]=asm["_i64Add"];var _i64Subtract=Module["_i64Subtract"]=asm["_i64Subtract"];var _llvm_bswap_i32=Module["_llvm_bswap_i32"]=asm["_llvm_bswap_i32"];var _malloc=Module["_malloc"]=asm["_malloc"];var _memcpy=Module["_memcpy"]=asm["_memcpy"];var _memset=Module["_memset"]=asm["_memset"];var _sbrk=Module["_sbrk"]=asm["_sbrk"];var establishStackSpace=Module["establishStackSpace"]=asm["establishStackSpace"];var getTempRet0=Module["getTempRet0"]=asm["getTempRet0"];var runPostSets=Module["runPostSets"]=asm["runPostSets"];var setTempRet0=Module["setTempRet0"]=asm["setTempRet0"];var setThrew=Module["setThrew"]=asm["setThrew"];var stackAlloc=Module["stackAlloc"]=asm["stackAlloc"];var stackRestore=Module["stackRestore"]=asm["stackRestore"];var stackSave=Module["stackSave"]=asm["stackSave"];var dynCall_ii=Module["dynCall_ii"]=asm["dynCall_ii"];var dynCall_iiii=Module["dynCall_iiii"]=asm["dynCall_iiii"];Module["asm"]=asm;Module["stringToUTF8"]=stringToUTF8;if(memoryInitializer){if(!isDataURI(memoryInitializer)){if(typeof Module["locateFile"]==="function"){memoryInitializer=Module["locateFile"](memoryInitializer)}else if(Module["memoryInitializerPrefixURL"]){memoryInitializer=Module["memoryInitializerPrefixURL"]+memoryInitializer}}
if(ENVIRONMENT_IS_NODE||ENVIRONMENT_IS_SHELL){var data=Module["readBinary"](memoryInitializer);HEAPU8.set(data,GLOBAL_BASE)}else{addRunDependency("memory initializer");var applyMemoryInitializer=(function(data){if(data.byteLength)data=new Uint8Array(data);HEAPU8.set(data,GLOBAL_BASE);if(Module["memoryInitializerRequest"])delete Module["memoryInitializerRequest"].response;removeRunDependency("memory initializer")});function doBrowserLoad(){Module["readAsync"](memoryInitializer,applyMemoryInitializer,(function(){throw"could not load memory initializer "+memoryInitializer}))}var memoryInitializerBytes=tryParseAsDataURI(memoryInitializer);if(memoryInitializerBytes){applyMemoryInitializer(memoryInitializerBytes.buffer)}else if(Module["memoryInitializerRequest"]){function useRequest(){var request=Module["memoryInitializerRequest"];var response=request.response;if(request.status!==200&&request.status!==0){var data=tryParseAsDataURI(Module["memoryInitializerRequestURL"]);if(data){response=data.buffer}else{console.warn("a problem seems to have happened with Module.memoryInitializerRequest, status: "+request.status+", retrying "+memoryInitializer);doBrowserLoad();return}}applyMemoryInitializer(response)}if(Module["memoryInitializerRequest"].response){setTimeout(useRequest,0)}else{Module["memoryInitializerRequest"].addEventListener("load",useRequest)}}else{doBrowserLoad()}}}function ExitStatus(status){this.name="ExitStatus";this.message="Program terminated with exit("+status+")";this.status=status}ExitStatus.prototype=new Error;ExitStatus.prototype.constructor=ExitStatus;var initialStackTop;dependenciesFulfilled=function runCaller(){if(!Module["calledRun"])run();if(!Module["calledRun"])dependenciesFulfilled=runCaller};function run(args){args=args||Module["arguments"];if(runDependencies>0){return}preRun();if(runDependencies>0)return;if(Module["calledRun"])return;function doRun(){if(Module["calledRun"])return;Module["calledRun"]=true;if(ABORT)return;ensureInitRuntime();preMain();if(Module["onRuntimeInitialized"])Module["onRuntimeInitialized"]();postRun()}if(Module["setStatus"]){Module["setStatus"]("Running...");setTimeout((function(){setTimeout((function(){Module["setStatus"]("")}),1);doRun()}),1)}else{doRun()}}Module["run"]=run;function exit(status,implicit){if(implicit&&Module["noExitRuntime"]&&status===0){return}if(Module["noExitRuntime"]){}else{ABORT=true;EXITSTATUS=status;STACKTOP=initialStackTop;exitRuntime();if(Module["onExit"])Module["onExit"](status)}if(ENVIRONMENT_IS_NODE){process["exit"](status)}Module["quit"](status,new ExitStatus(status))}Module["exit"]=exit;function abort(what){if(Module["onAbort"]){Module["onAbort"](what)}if(what!==undefined){Module.print(what);Module.printErr(what);what=JSON.stringify(what)}else{what=""}ABORT=true;EXITSTATUS=1;throw"abort("+what+"). Build with -s ASSERTIONS=1 for more info."}Module["abort"]=abort;if(Module["preInit"]){if(typeof Module["preInit"]=="function")Module["preInit"]=[Module["preInit"]];while(Module["preInit"].length>0){Module["preInit"].pop()()}}Module["noExitRuntime"]=true;run()

return k12Module.exports;
}  )(k12Module);


if( 0 ) {
	var k = KangarooTwelve();

	k.update( "" );
	k.final();
	var realBuf = k.squeeze( 64 );

	var outstr = [];
	realBuf.forEach( v=>outstr.push( v.toString(16)) );
	console.log( "BUF:", outstr.join( " " ) );

	k.release( realBuf );


	console.log( "init..." );

	k.init();
	k.update( "asdf" );
	k.final();
	realBuf = k.squeeze( 64 );

	outstr = [];
	console.log( "format..." );
	realBuf.forEach( v=>outstr.push( v.toString(16)) );
	console.log( "otuput..." );
	console.log( "BUF:", outstr.join( " " ) );


	realBuf = k.squeeze( 64 );

	outstr = [];
	realBuf.forEach( v=>outstr.push( v.toString(16)) );
	console.log( "BUF:", outstr.join( " " ) );
	realBuf = k.squeeze( 64 );

	outstr = [];
	realBuf.forEach( v=>outstr.push( v.toString(16)) );
	console.log( "BUF:", outstr.join( " " ) );
	realBuf = k.squeeze( 64 );

	outstr = [];
	realBuf.forEach( v=>outstr.push( v.toString(16)) );
	console.log( "BUF:", outstr.join( " " ) );


	k.release( realBuf );


	k.init();
	k.update( "asdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdf" );
	k.final();
	realBuf = k.squeeze( 64 );

	outstr = [];
	realBuf.forEach( v=>outstr.push( v.toString(16)) );
	console.log( "BUF:", outstr.join( " " ) );

	k.release( realBuf );
}


if( 0 ) {

	var start = Date.now();
	var n;
	for( n = 0; n < 10000000; n++ ) {
		var s = k.squeeze( 64 );
		k.release(s);
	}
	var end = Date.now();
	console.log( "N in M...", n, end-start, n/(end-start), n*32/(end-start) );

}


function KangarooTwelve() {
	const data = {
		k : 0,
		keybuf : 0,
		keybuflen : 0,
		buf : 0,
		bufMaps : new WeakMap(),
		outbuf : 0,
		realBuf : null,
	};
	var tn = 0;
	var s;
	var K12 = {
		init() {
			s = k12._KangarooTwelve_Initialize( data.k, 0 );	
			//console.log( "Initialize S?", s );
		},
		drop() {
			k12._free( data.keybuf );
			k12._free( data.buf );
			k12._free( data.k );
			//console.log( "S?", s );
		},
		update(buf) {
			//console.log( "feed update:", buf );
			
		        if( buf.length > data.keybuflen ) {
				if( data.keybuf )
					k12._free( data.keybuf );
				data.keybuflen = buf.length+1;
				data.keybuf = k12._malloc( buf.length+1 );
		
			}
			var byteLength;
			if( "string" === typeof buf )
				k12.stringToUTF8( buf, data.keybuf, buf.length*3+1 );
			else if( buf instanceof Uint32Array ) {
				var keydata = new Uint32Array( k12.HEAPU32.buffer, data.keybuf, buf.length );
				byteLength = buf.length * 4;
				//console.log( "copy keydata from binay", keydata );
				for( var b = 0; b < buf.length; b++ )
					keydata[b] = buf[b];
			}
			else if( buf instanceof Uint8Array ) {
				var keydata = new Uint8Array( k12.HEAPU8.buffer, data.keybuf, buf.length );
				byteLength = buf.length;
				//console.log( "copy keydata from binay", keydata );
				for( var b = 0; b < buf.length; b++ )
					keydata[b] = buf[b];
			}

			var keybuf = new Uint8Array( k12.HEAPU8.buffer, data.keybuf, byteLength );

			s = k12._KangarooTwelve_Update( data.k, data.keybuf, byteLength );
			//console.log( "Update S?", s );
		},
		final() {
			s = k12._KangarooTwelve_Final( data.k, 0, 0, 0 );
			//console.log( "Final S?", s );
		},
		squeeze(n) {
			s = k12._KangarooTwelve_Squeeze( data.k, data.outbuf, n );
			//data.realBuf = new Uint8Array( k12.HEAPU8.buffer, data.outbuf, 64 );
			//console.log( "Squeeze?", s, n );
			return data.realBuf;
		},
		release(buf) {
			if(0){
				var bufID = data.bufMaps.get( buf );
				if( bufID ) {
					k12._free( bufID );
					data.bufMaps.delete( buf );
				}
			}
		},
		absorbing: null,
		squeezing: null,
		clone() {
		},
		copy(from) {
		},
		phase() {
			return k12._KangarooTwelve_phase( data.k );
		},
	};
	
	data.k = k12._NewKangarooTwelve();
	data.outbuf = k12._malloc( 64 );
	//console.log( "malloc:", data.outbuf );
	//data.realBuf = k12.HEAPU8.slice( data.outbuf, data.outbuf+64 );
	data.realBuf = new Uint8Array( k12.HEAPU8.buffer, data.outbuf, 64 );
	K12.absorbing = k12._KangarooTwelve_IsAbsorbing.bind(k12,data.k),
	K12.squeezing = k12._KangarooTwelve_IsSqueezing.bind(k12,data.k),

	K12.init();

	return K12;
}

//-------------- byte xbox ------------------------------

function BlockShuffle_ByteShuffler( ctx ) {
	//struct byte_shuffle_key *key = New( struct byte_shuffle_key );
	var key = { map:[], dmap:[] };
	var n;
	var srcMap;
	for( n = 0; n < 256; n++ )
		key.map[n] = n;

	// simple-in-place shuffler.

	for( n = 0; n < 256; n++ ) {
		var m;
		var t;
		m = ctx.getByte(); //console.log( "swap:", n, m );
		t = key.map[m];
		key.map[m] = key.map[n];
		key.map[n] = t;
	}

	for( n = 0; n < 256; n++ )
		key.dmap[key.map[n]] = n;
	return key;
}

function  BlockShuffle_SubByte( key, bytes_input ) {
	return key.map[bytes_input];
}
function  BlockShuffle_BusByte( key, bytes_input ) {
	return key.dmap[bytes_input];
}

function BlockShuffle_SubBytes( key, bytes_input, bytes_output, offset, byteCount ) 
{
	var n;
	const map = key.map;
	for( n = 0; n < byteCount; n++ ) {
		bytes_output[offset+n] = map[bytes_input[offset+n]];
	}
}


function BlockShuffle_BusBytes( key, bytes_input, bytes_output, offset, byteCount ) 
{
	var n;
	const map = key.dmap;
	for( n = 0; n < byteCount; n++ ) {
		bytes_output[offset+n] = map[bytes_input[offset+n]];
	}
}

//----------------- K12-XBOX Utilities -----------------------------

// bit size of masking hash.
const RNGHASH = 256
const localCiphers = [];
function SRG_XSWS_encryptData( objBuf, tick, keyBuf ) {
	if( objBuf.buffer.byteLength & 0x7 ) {
		throw new Error( "buffer to encode must be a multiple of 64 bits; (should also include last byte of padding specification)" );
	}
	function encryptBlock( bytKey
		, output, output8, offset, outlen 
		,  bufKey
	) {
		var n;
		var dolen = outlen/4
		for( n = 0; n < dolen; n++ ) output[n] ^= bufKey [ ((n) % (RNGHASH / 32)) ];
		var p = 0x55;
		for( n = 0; n < outlen; n++ )  p = output8[n] = bytKey.map[output8[n] ^ p];
		p = 0xAA;
		for( n = outlen-1; n >= 0; n-- ) p = output8[n] = bytKey.map[output8[n] ^ p];
	}

	var signEntropy = localCiphers.pop();
	if( !signEntropy ) {
		signEntropy = SaltyRNG( null, {mode:1} );
		signEntropy.initialEntropy = null;
	}
	signEntropy.reset();	
	signEntropy.feed( tick );
	signEntropy.feed( keyBuf );

	var bufKey = new Uint32Array( signEntropy.getBuffer( RNGHASH ) );
	var bytKey = BlockShuffle_ByteShuffler( signEntropy );

	var outBufLen = objBuf.length * 4;
	//outBuf[0] = (uint8_t*)HeapAllocateAligned( NULL, (*outBufLen), 4096 );
	var outBuf = objBuf;//new Uint32Array( outBufLen / 4 );
	var outBuf8 = new Uint8Array( outBuf.buffer );

	for( var b = 0; b < outBufLen; b += 4096 ) {
		var bs = outBufLen - b;
		if( bs > 4096 )
			encryptBlock( bytKey, outBuf, outBuf8, b, 4096, bufKey );
		else
			encryptBlock( bytKey, outBuf, outBuf8, b, bs, bufKey );
	}

	localCiphers.push( signEntropy );

	return outBuf8;
}

function SRG_XSWS_encryptString( objBuf, tick, keyBuf ) {
	var tickBuf = new Uint32Array( 2 );
	
	tickBuf[0] = tick & 0xFFFFFFFF;
	tickBuf[1] = ( tick / 0x100000000 ) & 0xFFFFFFFF;
	var ob = myTextEncoder( objBuf );
	//console.log( "INPUT BUF?", ob.length );
	var ob32 = new Uint32Array( ob.buffer );
	//console.log( "BUF?", ob32, ob32[0], tickBuf, keyBuf );
	return SRG_XSWS_encryptData( ob32, tickBuf, keyBuf );
}

function SRG_XSWS_decryptData( objBuf,  tick, keyBuf ) {
	function decryptBlock( bytKey
		, input, offset,  len
		, output, output8
		, bufKey
	) {
		var n;
		for( n = 0; n < (len - 1); n++ ) output8[offset+n] = bytKey.dmap[input[offset+n]] ^ input[offset+n+1];
        	output8[offset+n] = bytKey.dmap[output[offset+n]] ^ 0xAA;
		for( n = (len - 1); n > 0; n-- ) output8[offset+n] = bytKey.dmap[output8[offset+n]] ^ output8[offset+n-1];
		output8[offset+0] = bytKey.dmap[output8[offset+0]] ^ 0x55;
		var dolen = len / 4;
		for( n = 0; n < dolen; n ++ ) output[offset+n] ^= bufKey [ ((n) % (RNGHASH / 32)) ];
	}

	
	var signEntropy = localCiphers.pop();
	if( !signEntropy ) {
		signEntropy = SaltyRNG( null, {mode:1} );
		signEntropy.initialEntropy = null;
	}
	
	signEntropy.reset();	
	signEntropy.feed( tick );
	signEntropy.feed( keyBuf );

	var bufKey = new Uint32Array( signEntropy.getBuffer( RNGHASH ) );
	var bytKey = BlockShuffle_ByteShuffler( signEntropy );

	var outBuf = new Uint32Array( objBuf.length );
	var outBuf8 = new Uint8Array( outBuf.buffer );
	var blockLen = objBuf.buffer.byteLength;
	for( var b = 0; b < blockLen; b += 4096 ) {
		var bs = blockLen - b;
		if( bs > 4096 )
			decryptBlock( bytKey, objBuf, b, 4096, outBuf, outBuf8, bufKey );
		else
			decryptBlock( bytKey, objBuf, b, bs, outBuf, outBuf8, bufKey );
	}
	outBuf = new Uint8Array( outBuf8, 0, outBuf.length - outBuf[0] + objBuf.buffer.length - 1 );

	localCiphers.push( signEntropy );
	return outBuf;
}

function SRG_XSWS_decryptString( objBuf, tick, keyBuf ) {
	var tickBuf = new Uint32Array( 2 );
	
	tickBuf[0] = tick & 0xFFFFFFFF;
	tickBuf[1] = ( tick / 0x100000000 ) & 0xFFFFFFFF;
	var outBuf = SRG_XSWS_decryptData( objBuf, tickBuf, keyBuf );
	return myTextDecoder( outBuf );
}

        // string->Uint8
	function myTextEncoder(s) {	
		var chars = [...s];
		var len = 0;
		for( var n = 0; n < chars.length; n++ ) {
			var chInt = chars[n].codePointAt(0);
			if( chInt < 128 ) 
				len++;
			else if( chInt < 0x800 ) 
				len += 2;
			else if( chInt < 0x10000 ) 
				len += 3;
			else if( chInt < 0x110000 ) 
				len += 4;
		}
		len+=2;
		var out = new Uint8Array( len + ( len&7?(8-(len&7)):0 ) );
		len = 0;			
		for( var n = 0; n < chars.length; n++ ) {
			var chInt = chars[n].codePointAt(0);
			if( chInt < 128 ) 
				out[len++] = chInt;
			else if( chInt < 0x800 ) {
				out[len++] = ( (chInt & 0x7c0) >> 6 ) | 0xc0;
				out[len++] = ( (chInt & 0x03f) ) | 0x80;
			} else if( chInt < 0x10000 ) {
				out[len++] = ( (chInt & 0xf000) >> 12 ) | 0xE0;
				out[len++] = ( (chInt & 0x0fc0) >> 6 ) | 0x80;
				out[len++] = ( (chInt & 0x003f) ) | 0x80;
			} else if( chInt < 0x110000 ) {
				out[len++] = ( (chInt & 0x01c0000) >> 18 ) | 0xF0;
				out[len++] = ( (chInt & 0x003f000) >> 12 ) | 0xE0;
				out[len++] = ( (chInt & 0x0000fc0) >> 6 ) | 0x80;
				out[len++] = ( (chInt & 0x000003f) ) | 0x80;
			}
		}
		out[len] = 0xFF;
		out[out.length-1] = out.length - (len);
		return out;
	}
        // uInt8 ->string
	function myTextDecoder(buf) {
		var out = '';
		var len;
		for( var n = 0; n < buf.length; n++ ) {
			if( buf[n] === 0xFF ) break;
			if( ( buf[n]& 0x80 ) == 0 )
				out += String.fromCodePoint( buf[n] );
			else if( ( buf[n] & 0xC0 ) == 0x80 ) {
				// invalid character... should already be skipped
			} else if( ( buf[n] & 0xE0 ) == 0xC0 ) {
				out += String.fromCodePoint( ( ( buf[n] & 0x1f ) << 6 ) | ( buf[n+1] & 0x3f ) );
				n++;
			} else if( ( buf[n] & 0xF0 ) == 0xE0 ) {
				out += String.fromCodePoint( ( ( buf[n] & 0xf ) << 12 ) | ( ( buf[n+1] & 0x3f ) << 6 ) | ( buf[n+2] & 0x3f ) );
				n+=2;
			} else if( ( buf[n] & 0xF8 ) == 0xF0 ) {
				out += String.fromCodePoint( ( ( buf[n] & 0x7 ) << 18 ) | ( ( buf[n+1] & 0x3f ) << 12 ) | ( ( buf[n+2] & 0x3f ) << 6 ) | ( buf[n+3] & 0x3f ) );
				n+=3;
			} else if( ( buf[n] & 0xFC ) == 0xF8 ) {
				out += String.fromCodePoint( ( ( buf[n] & 0x3 ) << 24 ) | ( ( buf[n+1] & 0x3f ) << 18 ) | ( ( buf[n+2] & 0x3f ) << 12 ) | ( ( buf[n+3] & 0x3f ) << 6 ) | ( buf[n+4] & 0x3f ) );
				n+=4;
			}
		}
		return out;
	}

function GetCurrentTick() {
	var now = new Date();
	var tick = now.getTime() * 256;
	tick |= ( -now.getTimezoneOffset() /15 ) & 0xFF;
	return tick;
}


function TickToTime( tick ) {
	var now = new Date( tick / 256 );
}
export {SRG_XSWS_encryptString}
export {SRG_XSWS_decryptString}
export {SRG_XSWS_encryptData}
export {SRG_XSWS_decryptData}
export {TickToTime}
export {GetCurrentTick}
export {SaltyRNG}


if( 0 ) {
	var keybuf = new Uint8Array(1);
	
	var output = SRG_XSWS_encryptString( "test", 123, keybuf );
	console.log( "ENC OUTPUT? ", output );
	var input = SRG_XSWS_decryptString( output, 123, keybuf );
	console.log( "DEC OUTPUT? ", input, input.length );
	
	output = SRG_XSWS_encryptString( "test1", 123, keybuf );
	console.log( "ENC OUTPUT? ", output );
	input = SRG_XSWS_decryptString( output, 123, keybuf );
	console.log( "DEC OUTPUT? ", input, input.length );
	
	output = SRG_XSWS_encryptString( "test12", 123, keybuf );
	console.log( "ENC OUTPUT? ", output );
	input = SRG_XSWS_decryptString( output, 123, keybuf );
	console.log( "DEC OUTPUT? ", input, input.length );
	
	output = SRG_XSWS_encryptString( "test123", 123, keybuf );
	console.log( "ENC OUTPUT? ", output );
	input = SRG_XSWS_decryptString( output, 123, keybuf );
	console.log( "DEC OUTPUT? ", input, input.length );
	
	output = SRG_XSWS_encryptString( "test1234", 123, keybuf );
	console.log( "ENC OUTPUT? ", output );
	input = SRG_XSWS_decryptString( output, 123, keybuf );
	console.log( "DEC OUTPUT? ", input, input.length );
	        
	output = SRG_XSWS_encryptString( "test1235", 123, keybuf );
	console.log( "ENC OUTPUT? ", output );
	input = SRG_XSWS_decryptString( output, 123, keybuf );
	console.log( "DEC OUTPUT? ", input, input.length );
}

