var sack=require( ".." );
var monitor = new sack.FileMonitor( ".", 100 );
//console.log( "monitor:", monitor, Object.keys( Object.getPrototypeOf( monitor ) ) );

monitor.addFilter( "*", (info)=>{
	console.log( "Any File Changed:", info );
        } );
