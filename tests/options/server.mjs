
import {sack} from "sack.vfs"
import {Protocol} from "sack.vfs/protocol"

const protocol = new Protocol( {port:5554}, "internal-options" );

const optionMap = new Map();

protocol.on( "get", (ws,msg)=>{
	const tree = getBranch( msg.id );
	ws.send( {op:get,root:tree} );

} );


function getBranch(id){
	const opt = optionMap.get( id );
	const list = [];
	if( opt )
		opt.eo( buildList );
	else
		sack.DB.eo( buildList );
	function buildList( opt, name ) {
		if( name === "." ) return;
		const id = sack.Id();
		optionMap.set( id, opt );
		list.push( { name, value:opt.value, id:id } );
	}
	return list;
}

