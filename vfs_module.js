"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/sack_vfs.node" );
} catch(err) { try { /*console.log( err ); */vfs = require( "./build/Release/sack_vfs.node" ); } catch( err ){  console.log( err )} }
console.log( "vfs:", vfs );
//vfs.InitFS();
//require( "./Δ" );

process.on( 'beforeExit', ()=>vfs.Thread() );
//process.on( 'exit', ()=>vfs.Thread() );
const thread = vfs.Thread(process._tickDomainCallback);
module.exports =exports= vfs;
//exports.Sqlite = vfs.Sqlite;
//exports.Volume = vfs.Volume;

//exports.Δ = vfs.Δ;
//exports.Λ = vfs.Λ;
/*
exports.Volume.read = (f)=>{
	var buf = vfs.read( f );
	console.log( "override tostring..." );
	buf.toString = function(){ Utf8ArrayToStr( this ) }
	return buf;
}

function Utf8ArrayToStr(array) {
    var out, i, len, c;
    var char2, char3;
    var char4, char5;
    var char6, char7;
    var char8, char9;
    var outarr = [];
    out = "";
    
    len = array.length;
    i = 0;
    while(i < len) {
    c = array[i++];
    switch(c >> 4)
    { 
      case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
        // 0xxxxxxx
        outarr.push(String.fromCharCode(c));
        break;
      case 12: case 13:
        // 110x xxxx   10xx xxxx
        outarr.push(String.fromCharCode(((c & 0x1F) << 6) | (array[i++] & 0x3F)));
        break;
      case 14:
        // 1110 xxxx  10xx xxxx  10xx xxxx
        outarr.push(String.fromCharCode(((c & 0x0F) << 12) |
                       ((array[i++] & 0x3F) << 6) |
                       ((array[i++] & 0x3F) << 0)));
        break;
      case 15:
        // 1111 0xxx   10xx xxxx  10xx xxxx 10xx xxxx
	if( !(c & 0x08) ) {
        	outarr.push(String.fromCharCode(((c & 0x07) << 18)  |
                	       ((array[i++] & 0x3F) << 12) |
	                       ((array[i++] & 0x3F) << 6) |
        	               ((array[i++] & 0x3F) << 0) ));
	} else if( !(c & 0x04 ) ) {
        	outarr.push(String.fromCharCode(((c & 0x03) << 24)  |
                	       ((array[i++] & 0x3F) << 18) |
	                       ((array[i++] & 0x3F) << 12) |
        	               ((array[i++] & 0x3F) << 6) |
        	               ((array[i++] & 0x3F) << 0) ));
	} else if( !(c & 0x02 ) ) {
        	outarr.push(String.fromCharCode(((c & 0x01) << 30)  |
                	       ((array[i++] & 0x3F) << 24) |
	                       ((array[i++] & 0x3F) << 18) |
        	               ((array[i++] & 0x3F) << 12) |
        	               ((array[i++] & 0x3F) << 6) |
        	               ((array[i++] & 0x3F) << 0) ));
	} else if( !(c & 0x01 ) ) {
        	outarr.push(String.fromCharCode(((array[i++] & 0x3F) << 30) |
	                       ((array[i++] & 0x3F) << 24) |
        	               ((array[i++] & 0x3F) << 18) |
        	               ((array[i++] & 0x3F) << 12) |
        	               ((array[i++] & 0x3F) << 6) |
        	               ((array[i++] & 0x3F) << 0) ));
	} else {
        	outarr.push(String.fromCharCode(((array[i++] & 0x3F) << 36) |
	                       ((array[i++] & 0x3F) << 30) |
        	               ((array[i++] & 0x3F) << 24) |
        	               ((array[i++] & 0x3F) << 18) |
        	               ((array[i++] & 0x3F) << 12) |
        	               ((array[i++] & 0x3F) << 6) |
        	               ((array[i++] & 0x3F) << 0) ));
	}
        break;
    }
    }
    out = outarr.join();

    return out;
}
*/
