
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
		nonce : opts.sign?sack.SaltyRNG.sign( sack.JSOX.stringify(o), 3, 3 ):null,
		data : o
	}
	if( opts.sign ) {
		var v = sack.SaltyRNG.verify( sack.JSOX.stringify(o), this.data.nonce, 3, 3 );
		//console.log( "TEST:", v );
		this.id = v.key;
		v.key = this.data.nonce;
		this.data.nonce = v;
	} else {
		this.id = opts.id || sack.id();
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
	function objectToJSOX(){
		
		//console.log( "THIS GOT CALLED?", this );
		var exist = newStorage.stored.get( this );
		//console.log( "THIS GOT CALLED? RECOVERED:", exist );
		if( exist ) {
			var obj = newStorage.cachedContainer.get( exist );
			if( newStorage.stringifier.isEncoding( obj ) ) return this;
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
				console.log( "Doesn't this have .." );
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
		
		if( opts.id ) {
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
		}

		if( !opts.id )
		{
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
					console.log( "Register toJSOX:", f );
					stringifier.registerToJSOX( f.tag, f.p, f.f ) 
				});
				console.log( "Something... " );
			}else {
				stringifier = this_.stringifier;
			}

			
			storage = stringifier.stringify( container );
				
			console.log( "Create file:", container.id, storage );
			this_.writeRaw( container.id, storage );
			//console.log( "OUTPUT:", storage );
			console.log( "Resolve with ID that this is complete?" );
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


_objectStorage.prototype._get = function( opts ) {
	//this.parser.
	var resolve;
	var reject;

	if( "string" === typeof opts ) {
		opts = { id:opts
		       , extraDecoders : null };
	}

	function parserObject( obj ) {
	}


	var parser = sack.JSOX.begin( parserObject );
	parser.fromJSOX( "~os", objectStorageContainer );
	if( opts.extraDecoders ) {
		opts.extraDecoders.forEach( f=>parser.fromJSOX( f.tag, f.p, f.f ) );
		console.log( "Something... " );
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
		//console.log( "Revive:", objId, ref, this.mapping, this.decoding );
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
				console.log( "Something.22.. ",objId );
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
		console.log( "doing read?", opts );
		os.read( opts.id
			, parser, (obj)=>{
			// with a new parser, only a partial decode before revive again...
					var found;
	        			console.log( "Reader callback", obj );
					do {
						var found = os.pending.findIndex( pending=>pending.id === key );
						if( found >= 0 ) {
							os.pending[found].ref.o[this.pending[found].ref.f] = obj.data.data;
							os.pending.splice( found, 1 );
						}
					} while( found >= 0 );
	        	
			if( obj && ( obj instanceof objectStorageContainer ) ){
				//console.log( "GOTzz:", obj );
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

	console.log( "returning promise." );
	return p;
}


function fileEntry( d ) {
	this.name = null;
	this.id = sack.id(); // object identifier of this.
	this.isFolder = false;
	this.readOnly = false;
	this.files = [];
	this.size = 0;
	this.created = new Date();
	this.updated = new Date();
	this.readkey = null;
	if( d )
		Object.defineProperty( this, "directory", {value:d} );
}

fileEntry.prototype.read = function( from, len ) {
	return new Promise( (res,rej)=>{
		if( this.isFolder ) {
			this.directory.volume._get( { id:this.id, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
				.catch( rej )
				.then( (dir)=>{
					fixDirRef(dir);
					res(dir) 
				} );
		
		} else {
			
			this.directory.volume.readRaw( this.id, r=>r?res(r):rej() );
		}
	} );

}

fileEntry.prototype.write = function( o, at, len ) {
	if( !("number"===typeof(len))) {
		if( !("number"===typeof(from))) {
			len = at;
			at = 0;
		} else  {
			len = o.length;
			at = 0;
		}
	}
	return Promise.resolve( this.directory.volume.writeRaw( this.id, o ) );
}

function fileDirectory( v ) {
	this.files = [];
	if( v )
		Object.defineProperty( this, "volume", {value:v} );
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
		this.changed = true;
	}
}

fileDirectory.prototype.open = function( fileName ) {
	var file = this.files.find( (f)=>(f.name == fileName ) );
	if( file ) {
	}else {
		file = new fileEntry( this );
		file.name = fileName;
		this.files.push(file);
		this.changed = true;
		this.store();
	}
	return file;
}

function encodeDirectory(){
	console.log( "Encode called with:", this );
}

fileDirectory.prototype.store = function() {
	console.log( "Write directory:", this );
	return this.volume.put( this, { id:"?", extraEncoders:[ { tag: "d", p:fileDirectory, f:null} 
					, { tag: "f", p:fileEntry, f:null} ] } );
}

function saveDirectory( d ) {
	
}

//defineProperty
var loadingDir = null;
function decodeDirectory(objId,ref){
	console.log( "Revive:", objId, ref, this.mapping, this.decoding );
	if( this.decoding.find( pending=>pending===objId ) ) {
			
	}
	return ref;
}


function fixDirRef(dir) {
	dir.files.forEach( file=>{
		Object.defineProperty( file, "directory", {value:dir} );
	} );
}

_objectStorage.prototype.getDirectory = function() {
	var result = new fileDirectory( this );
	loadingDir = result;
	return new Promise( (resolve,reject)=>{
		this._get( { id:"?", extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } ).catch( ()=>{
			result.store()
				.then( function(){resolve(result)} )
				.catch( reject );
			//resolve( result );
		} ).then( (dir)=>{
			if( !dir ) dir = result;
			fixDirRef(dir);
			Object.defineProperty( dir, "volume", {value:this} );
			resolve(dir) 
		} );
	} );	
}





}

