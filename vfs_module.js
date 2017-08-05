"use strict";

var vfs;
try{
  vfs = require( "./build/Debug/sack_vfs.node" );
} catch(err) { try { /*console.log( err ); */vfs = require( "./build/Release/sack_vfs.node" ); } catch( err ){  console.log( err )} }

let tmpParse6 = vfs.JSON6.parse;
let tmpParse = vfs.JSON.parse;

function JSONReviverWrapper (parser) {
   return (string,reviver)=> {
	var result = parser( string );
	if( !result && string ) throw new Error( "fault parsing json" );
        return typeof reviver === 'function' ? (function walk(holder, key) {
            var k, v, value = holder[key];
            if (value && typeof value === 'object') {
                for (k in value) {
                    if (Object.prototype.hasOwnProperty.call(value, k)) {
                        v = walk(value, k);
                        if (v !== undefined) {
                            value[k] = v;
                        } else {
                            delete value[k];
                        }
                    }
                }
            }
            return reviver.call(holder, key, value);
        }({'': result}, '')) : result;
   }
}

vfs.JSON6.parse = JSONReviverWrapper( tmpParse6 );
vfs.JSON6.stringify = JSON.stringify;
vfs.JSON.parse = JSONReviverWrapper( tmpParse );
vfs.JSON.stringify = JSON.stringify;


process.on( 'beforeExit', ()=> {
	vfs.Thread() 
});
process.on( 'exit', ()=> {
});
process.on( 'uncaughtException', (err)=> {
	console.log( err )	
});
const thread = vfs.Thread(process._tickDomainCallback);
module.exports =exports= vfs;

