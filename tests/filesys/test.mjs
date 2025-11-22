import {sack} from "sack.vfs"

const disk = sack.Volume();

console.log( "~ is a dir?", sack.Volume.expandPath( "~/" ) );
console.log( "; is a dir?", sack.Volume.expandPath( ";/" ) );
console.log( ", is a dir?", sack.Volume.expandPath( ",/" ) );
console.log( "@ is a dir?", sack.Volume.expandPath( "@/" ) );
console.log( "# is a dir?", sack.Volume.expandPath( "#/" ) );
console.log( "* is a dir?", sack.Volume.expandPath( "*/" ) );
console.log( "? is a dir?", sack.Volume.expandPath( "?/" ) );
console.log( ". is a dir?", sack.Volume.expandPath( "." ) );
console.log( sack.Volume.expandPath( "." ) + " is a dir?", disk.isDir( sack.Volume.expandPath( "." ) ) );
