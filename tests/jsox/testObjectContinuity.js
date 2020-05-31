const JSOX=require('../..').JSOX;

const object = '{object:true}\n{object:false}';

var parser = JSOX.begin( (obj)=>{
 console.log( "stuff", obj );
} );
parser.write( object );

