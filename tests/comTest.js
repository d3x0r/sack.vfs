
var sack = require( ".." );
console.log( "sack is:", sack );
var com1 = sack.ComPort( "COM3" );
com1.onRead( (data)=>{ console.log( "got", data ) } );

var buf = new Uint8Array(5);
buf[0] = '['.codePointAt(0);
buf[1] = 'K'.codePointAt(0);
buf[2] = ']'.codePointAt(0);
buf[3] = 's'.codePointAt(0);
buf[4] = '!'.codePointAt(0);

setInterval( tick, 1000 );

function tick() {
com1.write( buf );
}
