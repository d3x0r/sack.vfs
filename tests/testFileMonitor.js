var sack=require( ".." );
var monitor = new sack.FileMonitor( ".", 100 );
//console.log( "monitor:", monitor, Object.keys( Object.getPrototypeOf( monitor ) ) );

monitor.addFilter( "*", (info)=>{
	console.log( "Any File Changed:", info );
        } );

var dir = sack.Volume().dir();
for( var file in dir ) {
	file = dir[file];
	if( file.name == '.' || file.name == '..' ) continue;
	if( file.folder ) {
		var subdir = sack.Volume().dir( file.name )
		console.log( "Sub?", subdir );
	}
}
console.log( "DIR:", dir );
