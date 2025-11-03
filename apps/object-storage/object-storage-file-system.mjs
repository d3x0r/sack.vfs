


// these are used during store events because they are delay-flushed the same object may get re-written.
const pendingStore = [];
let lastStore = 0;
const storedPromise = Promise.resolve(undefined);


export class FileEntry {
	constructor ( d ) {
		this.name = null;
		this.id = null; // object identifier of this.
		this.contents = null;
		this.created = new Date();
		this.updated = new Date();
		if( d ) Object.defineProperty( this, "folder", {value:d} );
	}

	getLength () {
		return this.data.length;
	}

	read( from, len ) {
		const this_ = this;
		//console.trace( "READ RESULTING A PROMISE... PLEASE CATCH");
		return new Promise( (res,rej)=>{
			//console.log( "reading:", this );
			if( this_.isFolder ) {
				this_.folder.volume.get( { id:this_.id } )
					.catch( rej )
					.then( (dir)=>{
						//console.log( "Read directory, set folder property..." );
						for( var file of dir.files )  Object.defineProperty( file, "folder", {value:dir} );
						res(dir)
					} );
        
			} else {
				//console.log( "Reading ...", this_.id );
				if( this_.id )
					return this_.folder.volume.get( {id:this_.id} ).then( res ).catch( rej );
				//console.log( "Rejecting, no ID, (no data)", this_ );
				//console.log( "no file data in it?" );
				res( undefined ) // no data
			}
		} );
	}

	write( o ) {
		if( this.id )
			try {
				if( "string" === typeof o ) {
					console.log( "direct write of string data:", this.id, o );
					this.folder.volume.storage.writeRaw( this.id, o );
					return Promise.resolve( this.id );
	        
				} else if( o instanceof ArrayBuffer ) {
					console.log( "Write raw buffer", this );
					this.folder.volume.storage.writeRaw( this.id, o );
					return Promise.resolve( this.id );
	        
				} else if( "object" === typeof o )
					;//console.log( "expected encode of object:", o );
			} catch(err) {
				console.log( "Did the above do something?" )
			}
/*
	if( !("number"===typeof(len))) {
		if( "number"===typeof(at) ) {
			len = at;
			at = 0;
		} else  {
			len = undefined;
			at = undefined;
		}
	}
*/
		const this_ = this;
		//console.log( "Returning a promised volume put:", o, this );
		if( this.id )
			return this.folder.volume.put( o, this );
		else
			return this.folder.volume.put( o, this )
				.then( id=>{
					//console.log( "Re-setting id?", this_, id );
					this_.id=id;
					this_.folder.store();
					return id; } )
				.catch( (err)=>{
					console.log( "Error doing put?", err );
				})
	}
	async open( fileName ) {
		if( this.root ) return file.root.open( fileName );
		if( this.contents ) {
			return this.folder.volume.get( {id:file.contents } )
				.then( async (dir)=>{
					Object.defineProperty( file, "root", {value:dir} );
					return dir;
				} );
		}
		return this.folder.open( file.name );
	}
}


export class FileDirectory {
	files = [];
	#id = null;
	#volume = null;
	constructor( v, id ) {
		this.#volume = v;
		this.#id = id;
	}
	get id() {
		return this.#id;
	}
	find( file ) {
		// simple check; for a more advanced pathname matching use has(file) instead.
		return !!this.files.find( (f)=>(f.name == file ) );		
	}

	async create( fileName ) {
		var file = this.files.find( (f)=>(f.name == fileName ) );
		if( file ) {
			//console.log( "File already exists, not creating." );
			return null; // can't creeate already exists.
		} else {
			file = new FileEntry( this );
			file.name = fileName;
			this.files.push(file);
			this.store();
			return file;
		}
	}

	async open( fileName ) {
		const file = this.files.find( (f)=>(f.name == fileName ) );
		const _this = this;
		//console.log( "OPEN?", file );
		if( !file ) {
			return Promise.reject( new Error( "File not found:" + fileName ) );
		}
		return Promise.resolve( file );
	}

	store(force) {
		// save changes to this.
		if( !force ) {
			if( !pendingStore.find( p=>p===this)) 
				pendingStore.push( this );
			if( !lastStore ) {
				checkPendingStore();
			}
			lastStore = Date.now();
			return storedPromise;
		}
		return this.#volume.put( this, { id:this.#id } );
	}

	remove ( fileName ) {
		var parts = splitPath( fileName );
		var part;
		var pathIndex = 0;
		var dir = this;
		async function getOnePath() {
			if( pathIndex >= parts.length ) return false;
			if( !dir ) return false;
	        
			part = parts[pathIndex++];
			const fileId = dir.files.findIndex( (f)=>( f.name == part ) );
			if( fileId >= 0 && pathIndex >= parts.length ) {
				const file = dir.files[fileId];
				dir.#volume.remove( file.id );
				dir.files.splice( fileId, 1 );
				//if( !dir.files.length ) {
				//	dir.volume.remove( dir.id );
				//}
				return true;
			}
	        
			if( file.root ) dir = file.root;
			else {
				if( file.contents ) {
					return  dir.#volume.get( {id:file.contents } )
						.then( async (readdir)=>{
							Object.defineProperty( file, "root", {value:readdir} );
							dir = readdir;
							return getOnePath();
						});
				}
				else
					dir = null;
			}
			return getOnePath();
		}
		return getOnePath();
	}

	async has ( fileName ) {
		var parts = splitPath( fileName );
		var part;
		var pathIndex = 0;
		var dir = this;
		async function getOnePath() {
		console.log( "Checking for file:", fileName, dir );
			if( pathIndex >= parts.length ) return true;
			if( !dir ) return false;
	        
			part = parts[pathIndex++];
			var file = dir.files.find( (f)=>( f.name == part ) );
		console.log( "Checking for file:", fileName, file, dir );
			if( !file ) return false;
			if( file.root ) dir = file.root;
			else {
				if( file.contents ) {
					return  dir.#volume.get( {id:file.contents } )
						.then( async (readdir)=>{
							Object.defineProperty( file, "root", {value:readdir} );
							dir = readdir;
							return getOnePath();
						});
				}
				else
					dir = null;
			}
			return getOnePath();
		}
		return getOnePath();
	}

	folder( fileName ) {
		const _this = this;
		return new Promise( (res,rej)=>{
			const path = splitPath( fileName );
			const pathIndex = 0;
			const here = _this;
			function getOnePath() {
				const part = path[pathIndex++];
				let file = here.files.find( (f)=>(f.name == part ) );
				if( file ) {
					if( file.contents ) {
						_this.#volume.get( {id:file.contents } )
							.then( (dir)=>{
								if( pathIndex < path.length ) {
									here = dir;
									return getOnePath();
								}
								else
									res( dir );
							} );
					} else
						return Promise.reject( "Folder not found" );
				}else {
					file = new FileEntry( this );
					file.name = part;
					var newDir = new FileDirectory( _this.#volume, file.contents );
					Object.defineProperty( file, "root", {value:newDir} );
					_this.files.push(file);
					newDir.store().then( (id)=>{
						file.contents = id;
						_this.store().then( ()=>{
							if( pathIndex < path.length ) {
								here = file.root;
								return getOnePath();
							}
							res( file.root );
						} );
					} );
				}
			}
			getOnePath();
		} );
	}





}

function splitPath( path ) {
	return path.split( /[\\\/]/ );
}

function checkPendingStore() {
	if( lastStore ) {
		const now = Date.now();
		if( (now-lastStore) > 100 ){
			for( let p of pendingStore ) {
				p.volume.put( p, { id:p.id } );
			}
			pendingStore.length = 0;
			lastStore = 0;
		}
	}
	if( pendingStore.length ) {
		setTimeout( checkPendingStore, 50 );
	}
}

