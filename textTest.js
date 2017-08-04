
var text=`console.log( "Hello World; This is a test file." + "𝟘𝟙𝟚𝟛𝟜𝟝𝟞𝟟𝟠𝟡" );`

var vfs = require('.');
var vol = vfs.Volume();
var vol2 = vfs.Volume( "vfs.dat" );
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


