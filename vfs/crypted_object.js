"use strict";

var SRG = require( './salty_random_generator' );
var RNG = SRG.SaltyRNG( );

//exports.CryptObject
//var
exports.encryptObject = function( o ) {
    RNG.reset();
    let s = JSON.stringify( o );
    let char_s = decodeURIComponent(escape(s));
    let l = s.length;
    let output = new ArrayBuffer( l * 2 );
    let outview = new Uint16Array( output );
    for( let i = 0; i < l; i++ ) {
        outview[i] = s.charCodeAt( i ) ^ RNG.getBits( 10 );
    }
    return String.fromCharCode.apply(null, outview);
}


exports.decryptObject = function( str ) {
    let l = str.length;
    var buf = new ArrayBuffer(l*2); // 2 bytes for each char
    var bufView = new Uint16Array(buf);
    for (var i=0; i < l; i++) {
        bufView[i] = str.charCodeAt(i);
    }

    RNG.reset();

    let output = new ArrayBuffer( (l) * 2 );
    let outview = new Uint16Array( output );
    for( let i = 0; i < l; i++ ) {
        outview[i] = bufView[ i ] ^ RNG.getBits( 10 );
    }

    let s = String.fromCharCode.apply(null, outview );
    let obj = JSON.parse( s );
    return obj;
}
