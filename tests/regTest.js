
var vfs = require( ".." )
console.log( "Got:", vfs );

function update( newVal ) {
	var oldval = vfs.registry.get( "HKCU/Software/eQube/Gecko/TerminalID" );
	console.log( "Old Value is:", oldval );
	if( oldval != newVal )
		vfs.registry.set( "HKCU/Software/eQube/Gecko/TerminalID", newVal );
}
update( 420 );
