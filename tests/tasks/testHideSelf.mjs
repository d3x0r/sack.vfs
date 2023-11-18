
import {sack} from "sack.vfs"

let id = sack.Task.processId()

if( sack.Task.getTitle( id) === "No Window" )
	for( let pid = id; pid; pid = sack.Task.parentId( pid ) ) {
		if( sack.Task.getTitle( pid) !== "No Window" ) {
			//nwtasks.push( {id:pid} );
			id=pid;
			break;
		}
	}

{
	const styles = sack.Task.getStyles( id );
	const use_styles = Object.assign( {}, styles );
	use_styles.window &= ~sack.Task.style.window.WS_VISIBLE;
	sack.Task.setStyles( id, use_styles.window );//, -1, -1 ); 
	setTimeout( ()=>{ 
		sack.Task.setStyles( id, styles.window, -1, -1 );//, -1, -1 );
		setTimeout( ()=>{ }, 2000 );
	}, 5000 );
}


