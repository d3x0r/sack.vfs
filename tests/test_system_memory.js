
var sack=require('sack');

var ismem = sack.system.openMemory( "Blob1" );
if( ismem ) {
	console.log( "Another program instance is running" );
       	var getmem = new Uint8Array( ismem );
	console.log( "First values:", getmem[0], getmem[1], getmem[2], getmem[3] );
}
else {
	var nomem = sack.system.createMemory( "Blob1", "data.blob", 8 );
        console.log( "New memory:", nomem );
        if( nomem ) {
        	var setmem = new Uint8Array( nomem );
                setmem[0] = '1';
                setmem[1] = '2';
                setmem[2] = '3'.codePointAt(0);
                setmem[3] = '4';
                setmem[4+0] = 'A'.codePointAt(0);
                setmem[4+1] = 'B'.codePointAt(0);
                setmem[4+2] = 'C'.codePointAt(0);
                setmem[4+3] = 'D'.codePointAt(0);
        } else 
        	console.log( "Failed to create memory." );
	setTimeout( ()=>{
		console.log( "Times up." ); }, 10000 );
}
