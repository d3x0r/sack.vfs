
import {sack} from "sack.vfs" 

export function listOptions( path ) {
	if( !path ) path = "";
	const parts = path.split('/' );
	let part = 0;
	const result = {};
	let partName = parts[part].toLowerCase();
	//console.log( "Looking at something? " );
	if( partName.length === 0 ) {
		//console.log( "From Root..." );
		part++;
		if( part < parts.length ) {
			partName = parts[part].toLowerCase();
			sack.DB.eo( check );
		} else sack.DB.eo(list);
	}
	else {
		const start = partName;
		part++;
		if( part < parts.length ) {
			partName = parts[part].toLowerCase();		
			sack.DB.eo( start, check ); 
		}else sack.DB.eo( start, list );
	}
	function check( node) {
		//console.log( "check:", node, partName );
		if( node.name.toLowerCase() == partName ) {
			part++;
			if( part < parts.length ) {
				partName = parts[part].toLowerCase();
				node.eo( check );
			} else {
				//console.log( "GOing to list the result" );
				node.eo( list );
			}
		}
	}
	function list(node){
		//console.log( "enum to list?", node );
		const val = node.value;
		function isNum(str) {
			  return !Number.isNaN(Number(str)) && str.trim() !== "";
		}
		result[node.name] = isNum( val )?Number(val):val;
	}
	return result;
}

const path = process.argv[2];
console.log( "Options under:", path, listOptions(path ) );
