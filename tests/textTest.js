
var textArray = new ArrayBuffer( [99,111,110,115,111,108,101,46,108,111,103,40,32,34,72,101,108,108,111,32,87
	,111,114,108,100,59,32,84,104,105,115,32,105,115,32,97,32,116,101,115,116,32,102,105
	,108,101,46,34,32,43,32,34,240,157,159,152,240,157,159,153,240,157,159,154,240,157
	,159,155,240,157,159,156,240,157,159,157,240,157,159,158,240,157,159,159,240,157
	,159,160,240,157,159,161,34,32,41,59] );
// "Tbæøå"
console.log( "dump1", textArray.toString('utf8') );

var textOverlong = new Uint8Array( [193,163,193,175,193,174,193,179,193,175,193,172,193,165,192,174,193,172
	,193,175,193,167,192,168,192,160,192,162,193,136,193,165,193,172,193,172,193,175,192
	,160,193,151,193,175,193,178,193,172,193,164,192,187,192,160,193,148,193,168,193,169
	,193,179,192,160,193,169,193,179,192,160,193,161,192,160,193,180,193,165,193,179,193
	,180,192,160,193,166,193,169,193,172,193,165,192,174,192,162,192,160,192,171,192,160
	,192,162,248,128,157,159,152,248,128,157,159,153,248,128,157,159,154,248,128,157,159
	,155,248,128,157,159,156,248,128,157,159,157,248,128,157,159,158,248,128,157,159,159
	,248,128,157,159,160,248,128,157,159,161,192,162,192,160,192,169,192,187] );
console.log( "dump2", textOverlong.toString('utf8') );

var text=`console.log( "Hello World; This is a test file." + "𝟘𝟙𝟚𝟛𝟜𝟝𝟞𝟟𝟠𝟡" );`

var vfs = require('.');
var vol = vfs.Volume();
var vol2 = vfs.Volume( null, "vfs.dat" );

vol.write("out.3", textArray);
vol.write("out.4", textOverlong);


vol.write("out.1", text);
vol.write("out.2", text, true);
vol2.write("out.1", text);
vol2.write("out.2", text, true);

var x1 = vol.read("out.1").toString();
var x2 = vol.read("out.2").toString();


if (x1 !== text)
    console.log("Write and read 1 failed.");
if (x2 !== text)
    console.log("Write and read 2 failed.");

var x3 = vol2.read("out.1").toString();
var x4 = vol2.read("out.2").toString();

if (x3 !== text)
    console.log("Write and read 3 failed.");
if (x4 !== text)
    console.log("Write and read 4 failed.");


var x5 = vol2.read("out.1");
var x5int8 = new Uint8Array( x5 );
//dumpArray( x5int8 )
var x6 = vol2.read("out.2");
var x6int8 = new Uint8Array( x6 );
//dumpArray( x6int8 )

function dumpArray(u8 ) {
 	var out = '';
	var n;
	for( n = 0; n < u8.length;n++ )
		out += "," + u8[n];
	out[0] = '[';
	out += ']';
	console.log( out );
}

//var x5str = uintToString( x5int8 );
//var x6str = uintToString( x6int8 );
//console.log( ":) this is garbled right?", x6str );