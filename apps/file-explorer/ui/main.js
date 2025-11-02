
import {ExplorerProtocol} from "./protocol.js"


const protocol = new ExplorerProtocol();

protocol.on( "open", (ws)=>{

	protocol.dir();
} );

protocol.on( "dir", (msg)=>{
	console.log( "Protocol directory replied" );
	dirList.reset();
	fileList.reset();
	for( let e of msg.dir ) {
			if( e.folder ) dirList.add( e );
			else fileList.add(e);
	}
 } );

upDir.addEventListener( "click", (evt)=>{
	protocol.upDir();
} );

class DirectoryList {
	container = dirListContainer;
	list = dirListList;
	constructor() {
	}

	reset() {
		this.list.innerHTML = "";
	}

	add( d ) {
		const listRow = document.createElement( "li" );
		listRow.className = "directory-list-row";
		const name = document.createElement( "span" );
		name.className = "directory-list-row-name";
		const date = document.createElement( "span" );
		date.className = "directory-list-row-date";
		name.textContent = d.name;
		//date.textContent = d.written.toLocaleString();
		listRow.appendChild( name );
		listRow.appendChild( date );
		this.list.appendChild( listRow );

		listRow.addEventListener( "click", (evt)=>{
			protocol.setDir( d );
		} );


	}
}


class FileList {
	container = dirFileContainer;
	list = dirFileList;

	constructor() {
	}

	reset() {
		this.list.innerHTML = "";
	}

	add( d ) {
		const listRow = document.createElement( "li" );
		listRow.className = "file-list-row";
		const name = document.createElement( "span" );
		name.className = "file-list-row-name";
		const date = document.createElement( "span" );
		date.className = "file-list-row-date";
		name.textContent = d.name;
		date.textContent = d.written.toLocaleString();
		listRow.appendChild( name );
		listRow.appendChild( date );
		this.list.appendChild( listRow );

	}

}


const fileList = new FileList();
const dirList = new DirectoryList();