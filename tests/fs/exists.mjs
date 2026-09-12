import {sack} from "sack.vfs"

const disk = sack.Volume();
console.log( "a.out?", disk.exists("a.out" ) );
console.log( "write a.out?", disk.write("a.out", "blah" ) );
console.log( "a.out?", disk.exists("a.out" ) );
console.log( "unlink a.out?", disk.unlink( "a.out" ) );
console.log( "a.out?", disk.exists("a.out" ) );
console.log( "mkdir a.out?", disk.mkdir("a.out" ) );
console.log( "a.out?", disk.exists("a.out" ) || disk.isDir( "a.out" ) );
console.log( "unlink a.out?", disk.unlink( "a.out" ) || disk.rmdir("a.out"));


console.log( "a.out?", disk.exists("a.out" ) );

function exists(a) {
	if( disk.isDir(a) ) return true;
	if( disk.exists(a) ) return true;
	return false;
}
