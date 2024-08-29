
const SRG=require( "sack.vfs" ).SaltyRNG;
//import {SaltyRNG as SRG2} from "./salty_random_generator2.mjs";


const test1 = SRG( ()=>{}, {mode:1} );
//const test1a = SRG( ()=>{}, {mode:2} );

//const test2 = SRG2( ()=>{}, {mode:1} );
//const test2a = SRG2( ()=>{}, {mode:2} );
{
const start = new Date();
let n;
for(  n = 0; n < 100000; n++ ) {
	const b1 = test1.getBuffer(256);
	//console.log( 'buf:',b1);
}
const end = new Date();
console.log( "SRG.getBuffer(256) did :", n * 256, "in", end.getTime()-start.getTime(), n*256/(end.getTime()-start.getTime()) );
}

{
const start = new Date();
let n;
for(  n = 0; n < 100000; n++ ) {
	for( let n = 0; n < 64; n++ ) Math.random();
	//console.log( 'buf:',b1);
}
const end = new Date();
console.log( "math.random() did :", n * 256, "in", end.getTime()-start.getTime(), n*256/(end.getTime()-start.getTime()) );
}


//console.log("Made:", test1, test2 ) ;
console.log( "Test1:", SRG.id( "test" ) );
console.log( "Test1:", SRG.id( "test2" ) );
console.log( "Test1:", SRG.id( "test3" ) );
//console.log( "Test1:", SRG2.id( "test" ) );
