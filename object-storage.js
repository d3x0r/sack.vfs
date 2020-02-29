
module.exports = function(sack) {


sack.SaltyRNG.setSigningThreads( require( "os" ).cpus().length );

// save original object.
const _objectStorage = sack.ObjectStorage;


// associates object data with storage data for later put(obj) to re-use the same informations.
function objectStorageContainer(o,opts) {
	if( !this instanceof objectStorageContainer ) return new objectStorageContainer(o,opts);
	this.def = {
		indexes : [],
		dirty : false,
	}
	this.data = {	
		nonce : opts?opts.sign?sack.SaltyRNG.sign( sack.JSOX.stringify(o), 3, 3 ):null:null,
		data : o
	}
	if( opts && opts.sign ) {
		var v = sack.SaltyRNG.verify( sack.JSOX.stringify(o), this.data.nonce, 3, 3 );
		//console.log( "TEST:", v );
		Object.defineProperty( this, "id", { value:v.key } );
		v.key = this.data.nonce;
		this.data.nonce = v;
	} else {
		if( opts && opts.id ) {
			Object.defineProperty( this, "id", { value:opts.id } );
		}
	}
	//console.log( "Container:", this );
}

objectStorageContainer.prototype.createIndex = function( storage, fieldName, opts ) {
	if( !fieldName ) throw new Error( "Must specify an object field to index" );

	var path;
	if( !fieldName.isArray() ) {
		if( typeof(fieldName) != "String") throw new Error( "Index field name must be a string, or an array." );
		path = fieldName.split('.' );	
	}else		
		path = fieldName;	
	
	const indexList = this.def.indexes;

	var referringObject = null;

	var end = path.reduce( (acc,val)=>{ 
		referringObject = acc;
		if( acc ) {
			let tmp = acc[val];
			if( tmp === undefined )
				if( val < (path.length-1) ) { // automatically build object path
					if( typeof(path[val+1]) === "String" )
						acc[val] = tmp = {};
					else
						acc[val] = tmp = [];
				}
			acc = tmp;
		}
		return acc;

	}, this.data.data );
	if( end ) {
		if( !end.isArray() ){
			throw new Error( "Indexes can only be applied to arrays.");
		}
	}else if( referringObject ){
		// automatically assign a value
		end = referringObject[path[path.length-1]] = [];
	}else {
		throw new Error( "Path to index could not be found")
	}


	var index = {
		data : end,
		name : fieldName,
		opts : opts,
	};

	const storageIndex = storage.createIndex( this.id, index );
	indexList.push( index );
}



sack.ObjectStorage = function (...args) {
	var mapping = false;
	var newStorage = new _objectStorage(...args);
	newStorage.cached = new Map();
	newStorage.cachedContainer = new Map();
	newStorage.stored = new WeakMap(); // objects which have alredy been through put()/get()
	newStorage.decoding = [];
	newStorage.pending = [];
	newStorage.setupStringifier = setupStringifier;
	newStorage.stringifier = sack.JSOX.stringifier();
	newStorage.root = null;
	function objectToJSOX( stringifier ){
		
		//console.log( "THIS GOT CALLED?", this );
		var exist = newStorage.stored.get( this );
		//console.log( "THIS GOT CALLED? RECOVERED:", exist );
		if( exist ) {
			var obj = newStorage.cachedContainer.get( exist );
			if( stringifier.isEncoding( obj ) ) return this;
			return '~os"'+exist+'"';
		} else {			
			if( this instanceof objectStorageContainer ) {
				//console.log( "THIS SHOULD ALREADY BE IN THE STORAGE!", this, newStorage.stored.get( this.data.data ) );
				//newStorage.stored.set( this.data.data, this.id );
				//newStorage.cached.set( this.id, this.data.data );
				//newStorage.cachedContainer.set( this.id, this );
				newStorage.stored.set( this.data.data, this.id );
			} else {
				newStorage.cached.set( this.id, this );
			}
			//console.log( "Commit as stored; first" );
		}
		return this;
	}
	function setupStringifier( stringifier ) {
		stringifier.setDefaultObjectToJSOX( objectToJSOX );
		stringifier.registerToJSOX( "~os", objectStorageContainer, objectToJSOX );
	}
	setupStringifier( newStorage.stringifier );


	newStorage.map = function( expr ) {
		newStorage.mapping = true;
		//this.parser.
		var resolve;

		this.get( expr ).then( (obj)=>{
			resolve(obj);
			newStorage.mapping = false;

		} );
		return new Promise( function(res,rej) {
			resolve = res;
		} );
	}

	return newStorage;
	//Object.assign( newStorage
}


// define a class... to be handled by stringification
_objectStorage.prototype.defineClasss = function(a,b) {
	this.stringifier.defineClass(a,b);
}

_objectStorage.prototype.scan = function( from ) {
	var fromTime = ( from.getTime() * 256 );
	//this.loadSince( fromTime ); 
}

_objectStorage.prototype.getContainer = function( obj, options ) {
	var container = this_.stored.get( obj );
	var storage;
	if( container ) {
		container = this_.cachedContainer.get( container ); 
		return container;
	}
	console.log( "Getting a new container..." );
	container = new objectStorageContainer(obj,options);
	this_.stored.set( obj, container.id );
	this_.cached.set( container.id, container.data.data );
	this_.cachedContainer.set( container.id, container );
}

_objectStorage.prototype.createIndex = function( id, index ){

}

_objectStorage.prototype.index = function( obj, fieldName, opts ) {
	var this_ = this;
	return new Promise( function(res,rej){

		var container = this_.stored.get( obj );
		
		//console.log( "Put found object?", container, obj, options );
		if( container ) {
			container = this_.cachedContainer.get( container ); 
			if( container.data.nonce ) { 
				rej( new Error( "Sealed records cannot be modified" ) );
			}
		}
		else
		{
			if( !opts.id ) {
				console.log( "Create index, creating new container" );
				container = new objectStorageContainer(obj,options);
				
				//console.log( "saving stored container.id", obj, container.id );
				
				//this.stored.delete( obj );
				this_.stored.set( obj, container.id );
				this_.cached.set( container.id, container.data.data );
				this_.cachedContainer.set( container.id, container );
			}
			
		}

		container.createIndex( this_, fieldName, opts );

		//console.log( "OUTPUT:", storage );
		res();

	})

}

_objectStorage.prototype.put = function( obj, opts ) {
	var this_ = this;
	return new Promise( function(res,rej){

		var container = this_.stored.get( obj );
		
		//console.log( "Put found object?", container, obj, opts );
		if( container ) {
			container = this_.cachedContainer.get( container ); 
			
			if( !container.data.nonce ) {
				// make sure every item that is in an index
				// has been written... 
				if( this_.def )
					this_.def.indexes.forEach( index=>{
						index.data.forEach( (item)=>{
							this_.put( item );
						});
					})


				var stringifier;
				if( opts.extraEncoders ) {
					stringifier = sack.JSOX.stringifier(); 
					this_.setupStringifier( stringifier );
					opts.extraEncoders.forEach( f=>{
						stringifier.registerToJSOX( f.tag, f.p, f.f ) 
					});
				}else {
					stringifier = this_.stringifier;
				}

				storage = stringifier.stringify( container );
				this_.writeRaw( container.id, storage );
				return container.id;
			} else { 
				throw new Error( "record is signed, cannot put" );
			}
		}
		
		if( opts && opts.id ) {
			var stringifier;
			if( opts.extraEncoders ) {
				stringifier = sack.JSOX.stringifier(); 
				this_.setupStringifier( stringifier );
				opts.extraEncoders.forEach( f=>{
					stringifier.registerToJSOX( f.tag, f.p, f.f ) 
				});
			}else {
				stringifier = this_.stringifier;
			}

			
			storage = stringifier.stringify( obj );
			this_.writeRaw( opts.id, storage );
			res( opts.id );
		} else if( !opts || !opts.id ) {
			console.log( "New bare object, create a container...", opts );
                        if( !opts ) opts = { id : sack.id() }
                        else opts.id = sack.id();
			container = new objectStorageContainer(obj,opts);
				
			//console.log( "saving stored container.id", obj, container.id );
				
			//this.stored.delete( obj );
			this_.stored.set( obj, container.id );
			this_.cached.set( container.id, container.data.data );
			this_.cachedContainer.set( container.id, container );

			var stringifier;
			if( opts.extraEncoders ) {
				stringifier = sack.JSOX.stringifier(); 
				this_.setupStringifier( stringifier );
				opts.extraEncoders.forEach( f=>{
					stringifier.registerToJSOX( f.tag, f.p, f.f ) 
				});
			}else {
				stringifier = this_.stringifier;
			}

			
			storage = stringifier.stringify( container );
			this_.writeRaw( container.id, storage );
			//console.log( "OUTPUT:", storage );
			res(  container.id );
		}
	})
}

/*
_objectStorage.prototype.update( objId, obj ) {
	
	var container = new objectStorageContainer(sack.JSOX.stringify(obj),sign);
	this.stored.set( obj, container.id );
	this.cached.set( container.id, container );
	return container.id;
}

*/


_objectStorage.prototype.get = function( opts ) {
	//this.parser.
	var resolve;
	var reject;

	if( "string" === typeof opts ) {
		opts = { id:opts
		       , extraDecoders : null };
	}

	function parserObject( obj ) {
	}

	function reviveContainer( field, val ) {
		if( !field ) {
			Object.defineProperty( this, "id", { value:opts.id } );		
			return this;
		} 
		else {
			this[field] = val;
			// this is a sub-field of this object to revive...
			//console.log( "Field:", field, " is data?", val)
		}
	}


	var parser = sack.JSOX.begin( parserObject );
	parser.fromJSOX( "~os", objectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
	if( opts.extraDecoders ) {
		opts.extraDecoders.forEach( f=>parser.fromJSOX( f.tag, f.p, f.f ) );
	}

	function decodeStoredObjectKey(objId,res,rej){
		console.log( "Promised Reviver:", objId, this.mapping );

		if( this.mapping ) {
			var exist = this.cached.get( objId );
			if( !exist ) {
				console.log( "Chained get...",objid );
				this.get( objId ).then( (obj)=>{
					console.log( "Storage returned:", this, obj );
					this.cached.set( objId, obj );
					this.stored.set( obj, objId );
					
					res( obj );
				} );
			}
			//console.log( "Otherwise returning existing:", exist );
			//return exist;
		} else {
			console.log( "Returning existing" );
			res( objId );
			//return objId;
		}
	};


	this.decoding.push( opts );
	function decodeStoredObjectKeyImmediate(objId,ref){
		console.log( "Revive:", objId, ref, this.mapping, this.decoding );

		if( this.decoding.find( pending=>pending===objId ) ) {
			//console.log( "Push a pending resolution for:",  {id:objId, ref: ref } );
			this.pending.push( {id:objId, ref: ref } );
			return objId;
		}
		this.decoding.push( objId );
		if( this.mapping ) {
			var exist = this.cached.get( objId );
			if( !exist ) {
				console.log( "Chained get..." );

				var parser = sack.JSOX.begin( parserObject );
				parser.fromJSOX( "~os", objectStorageContainer, decodeStoredObjectKeyImmediate );
				//console.log( "Something.22.. ",objId );
				this.read( objId, parser, (obj)=>{
					//console.log( "Immediate result" );
					// with a new parser, only a partial decode before revive again...
					if( obj ){
						Object.setPrototypeOf( obj, objectStorageContainer.prototype );
						exist = obj.data.data;

						this.stored.set( obj.data.data, obj.id );
						this.cachedContainer.set( obj.id, obj ); 

					}
				} );
				this.decoding.pop();
				if( !this.decoding.length ) {
					console.log( "So... I am?", sack.JSOX.stringify( obj ) );
				}
				var found;

				do {
					var found = this.pending.findIndex( pending=>pending.id === objId );
					if( found >= 0 ) {
						this.pending[found].ref.o[this.pending[found].ref.f] = exist;
						this.pending.splice( found, 1 );
					}
				} while( found >= 0 );
			}
			console.log( "Otherwise returning existing:", exist );
			return exist;
		} else {
			console.log( "Returning existing" );
			//resolve( objId );
			return objId;
		}
	};

	//console.log( "(get)Read Key:", opts );
	var os = this;
	var p = new Promise( function(res,rej) {
		resolve = res;  reject = rej;
		//console.log( "doing read? (decodes json using a parser...", opts, parser );
		os.read( opts.id
			, parser, (obj)=>{
			// with a new parser, only a partial decode before revive again...
				var found;
				do {
					var found = os.pending.findIndex( pending=>pending.id === key );
					if( found >= 0 ) {
						os.pending[found].ref.o[this.pending[found].ref.f] = obj.data.data;
						os.pending.splice( found, 1 );
					}
				} while( found >= 0 );
	        	
			if( obj && ( obj instanceof objectStorageContainer ) ){
				//console.log( "GOTzz:", obj, obj.id );
				os.stored.set( obj.data.data, obj.id );
				os.cachedContainer.set( obj.id, obj ); 
				//console.log( "and resolve")
				res(obj.data.data);
			} else if( obj )
				res( obj );
			else {
				rej();
			}
		} );
	} );
	return p;
}


function fileEntry( d ) {
	this.name = null;
	this.id = null; // object identifier of this.
	this.contents = null;
	this.created = new Date();
	this.updated = new Date();

	if( d ) Object.defineProperty( this, "folder", {value:d} );
}

fileEntry.prototype.getLength = function() {
	return this.data.length;
}

fileEntry.prototype.read = function( from, len ) {
	return new Promise( (res,rej)=>{
		//console.log( "reading:", this );
		if( this.isFolder ) {
			this.folder.volume.get( { id:this.id, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
				.catch( rej )
				.then( (dir)=>{
					console.log( "Read directory, set folder property..." );
					for( var file of dir.files )  Object.defineProperty( file, "folder", {value:dir} );
					res(dir) 
				} );
		
		} else {
			if( this.id )
				return this.folder.volume.get( {id:this.id} ).then( res ).catch( rej );
			console.log( "Rejecting, no ID, (no data)", this );
			rej(); // no data
		}
	} );

}

fileEntry.prototype.write = function( o ) {
	if( "string" === typeof o ) {
		//console.log( "direct write of string data:", o );
		this.folder.volume.writeRaw( this.id, o );
		return Promise.resolve( this.id );
		
	} else if( o instanceof ArrayBuffer ) {
		console.log( "Write raw buffer" );
		this.folder.volume.writeRaw( this.id, o );
		return Promise.resolve( this.id );
	
	} else if( "object" === typeof o ) 
		;//console.log( "expected encode of object:", o );

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
	if( this.id )
		return this.folder.volume.put( o, this );
	else
		return this.folder.volume.put( o, this )
			.then( id=>{
				console.log( "Re-setting id?", this, id );
				this.id=id;
				this.folder.store();
				return id; } )
}



function fileDirectory( v, id ) {
	this.files = [];
	// cache chanes instead of flushing every time?
	//Object.defineProperty( this, "changed", {value:false, writable:true} );
	if( v ) Object.defineProperty( this, "volume", {value:v} );
	//console.log( "This already has an ID?", this );
	if( id ) Object.defineProperty( this, "id", { value:id } );

}


fileDirectory.prototype.find = function( file ) {

}

fileDirectory.prototype.create = function( fileName ) {
	var file = this.files.find( (f)=>(f.name == fileName ) );
	if( file ) {
		return null;
	} else {
		file = new fileEntry( this );
		file.name = fileName;
		this.files.push(file);
		//this.changed = true;
	}
}

fileEntry.prototype.open = async function( fileName ) {
	if( this.root ) return file.root.open( fileName );
	if( this.contents ) {
		return this.folder.volume.get( {id:file.contents, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
			.then( async (dir)=>{  
				Object.defineProperty( file, "root", {value:dir} );
				return dir; 
			} );
	}  
	return this.folder.open( file.name );
}

fileDirectory.prototype.open = async function( fileName ) {
	var file = this.files.find( (f)=>(f.name == fileName ) );
	const _this = this;
	if( !file ) {
		file = new fileEntry( this );
		file.name = fileName;
		this.files.push(file);
		this.store();
	}
	return file;
}


function splitPath( path ) {
	return path.split( /[\\\/]/ );
}

fileDirectory.prototype.store = function() {
	return this.volume.put( this, { id:this.id, extraEncoders:[ { tag: "d", p:fileDirectory, f:null} 
					, { tag: "f", p:fileEntry, f:null} ] } );
}


fileDirectory.prototype.has = async function( fileName ) {
	var parts = splitPath( fileName );
	var part;
	var pathIndex = 0;
	var dir = this;
	async function getOnePath() {
		if( pathIndex >= path.length ) return true;
		if( !dir ) return false;

		part = path[pathIndex++];
		var file = dir.files.find( (f)=>( f.name == part ) );
		if( !file ) return false;
		if( file.root ) dir = file.root;
		else {
			if( file.contents ) {
				return  dir.volume.get( {id:file.contents, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
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

fileDirectory.prototype.folder = function( fileName ) {
	const _this = this;
	return new Promise( (res,rej)=>{
		var path = splitPath( fileName );
		var pathIndex = 0;
		var here = _this;
		function getOnePath() {
			let part = path[pathIndex++];
			var file = here.files.find( (f)=>(f.name == part ) );
			if( file ) {
				if( file.contents ) {
					_this.volume.get( {id:file.contents, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
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
				file = new fileEntry( this );
				file.name = part;
				var newDir = new fileDirectory( _this.volume, file.contents );
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



var loading = null;
_objectStorage.prototype.getRoot = async function() {
	if( this.root ) return this.root;
	if( loading ) {
		return new Promise( (res,rej)=>{
			loading.push(  {res:res, rej:rej} );
		} );
	}
	var result = new fileDirectory( this, "?" );
	var this_ = this;
	loading = [];
	return new Promise( (resolve,reject)=>{
		this_.get( { id:result.id, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
			.catch( ()=>{
				return result.store()
					.then( function(id){
						//console.log( "1) Assigning ID to directory", id );
						Object.defineProperty( result, "id", { value:id } );
						resolve(result)
					} )
					.catch( reject );
				} )
			.then( (dir)=>{
				//console.log( "get root directory got:", dir, "(WILL DEFINE FOLDER)" );
				if( !dir ) dir = result;

				// foreach file, set file.folder
				else for( var file of dir.files ) Object.defineProperty( file, "folder", {value:dir} );
				//console.log( "2) Assigning ID to directory(sub)", result.id );
				Object.defineProperty( dir, "id", { value:result.id } );
				Object.defineProperty( dir, "volume", {value:this_} );
				this_.root = dir;
				

				resolve(dir)  // first come, first resolved
				// notify anyone else that was asking for this... 
				if( loading && loading.length ) for( var l of loading ) l.res(dir);
				loading = null; // dont' need this anymore.
			} );
	} );	
}





}

