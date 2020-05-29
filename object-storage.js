
module.exports = function(sack) {
const _debug = false;

sack.SaltyRNG.setSigningThreads( require( "os" ).cpus().length );

// save original object.
const _objectStorage = sack.ObjectStorage;
const fillerObject = {};
Object.freeze( fillerObject );

var dangling = [];
var objectRefs = 0;

var preloadStorage = null;
// manufacture a JS interface to _objectStorage.
sack.ObjectStorage = function (...args) {

	// per object storage instance,
	// these are dangling loads.

	const danglingIds = new Map();

// associates object data with storage data for later put(obj) to re-use the same informations.
function objectStorageContainer(o,opts) {
	if( !this instanceof objectStorageContainer ) return new newStorage.objectStorageContainer(o,opts);

	//console.trace( "Something creating a new container..", o, opts );
	try {
	if( "string" === typeof o ){
		// still, if the proir didn't resolve, need to resolve this..
		let existing = store.cachedContainer.get( o );
		o = existing.data;
		if( existing.resolve )
		return ;
	}
	//this.def = {
	//	indexes : [],
	//	dirty : false,
	//}
	var resolve = o && !opts;
	this.data = o || {}; // reviving we get null opts.
	if( !o && opts && opts.ref ) {
		console.trace( "ref loading is deprecated.");
		//resolve = opts.ref;
	}

	Object.defineProperty( this, "encoding", { writable:true, value:false } );
/*
	this.data = {
		//nonce : opts?opts.sign?sack.SaltyRNG.sign( sack.JSOX.stringify(o), 3, 3 ):null:null,
		data : o
	}
*/
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
	}catch(err) {
		console.log( "Uncaugh exception:", err );
	}
	//console.log( "Container:", this );
}

objectStorageContainer.prototype.getStore = function() {
	return store;
}
objectStorageContainer.getStore = function() {
	return store;
}

objectStorageContainer.prototype.map = async function( opts ) {
	const pending = this.dangling;
	if( pending && pending.length )  {
		const rootMap = this;
		return new Promise( (res,rej)=>{
			let waiting = 0;
			for( let load of pending ){
				waiting++;
				newStorage.get( {id:load.d.id}).then( (obj)=>{
					var exist = newStorage.stored.get( obj );
					var obj = newStorage.cachedContainer.get( exist );

					//Object.defineProperty( load.c, "data", {value:obj} );
					if( opts && opts.depth ) {
						obj.map( {depth:opts.depth-1} ).then( ()=>{
							waiting--;
							if( !waiting ) res( rootMap.data );
							load.d.res(obj.data); // result with real value.
						});
					} else {
						load.d.res(obj.data); // result with real value.
						waiting--;
						if( !waiting ) res( rootMap.data );
					}
				})
			}
			pending.length = 0;
		})
	}
	return this.data;
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

	}, this.data );
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


	var mapping = false;
	var newStorage = preloadStorage || new _objectStorage(...args);
	preloadStorage = null;
	newStorage.objectStorageContainer = objectStorageContainer;
	newStorage.cached = new Map();
	newStorage.cachedContainer = new Map();
	newStorage.stored = new WeakMap(); // objects which have alredy been through put()/get()
	newStorage.decoding = [];
	newStorage.pending = [];
	newStorage.setupStringifier = setupStringifier;
	newStorage.stringifier = sack.JSOX.stringifier();
	newStorage.root = null;
	newStorage.encoders = [];
	newStorage.decoders = [];
	function objectToJSOX( stringifier ){
		//  see if we alread stored this... (or are currently storing this.) (back references container)
		//console.log( "THIS GOT CALLED?", this, Object.getPrototypeOf( this ) );
		var exist = newStorage.stored.get( this );
		if( exist ) {
			var obj = newStorage.cachedContainer.get( exist );
			//console.log( "Object stored independantly... RECOVERED:", exist, obj.encoding );
			if( obj.encoding )
				return this;
			else {
				//console.log( "Why is this an ~or and not ~os?");
				return '~or"'+exist+'"';
			}
		} else {
			if( this instanceof objectStorageContainer ) {
				//console.log( "THIS SHOULD ALREADY BE IN THE STORAGE!", this, newStorage.stored.get( this.data ) );
				//newStorage.stored.set( this.data, this.id );
				//newStorage.cached.set( this.id, this.data );
				//newStorage.cachedContainer.set( this.id, this );
				newStorage.stored.set( this.data, this.id );
				return {data:this.data};
			} else {
				//newStorage.cached.set( this.id, this );
			}
			//console.log( "Commit as stored; first" );
		}
		//console.log( "not a container...");
		return this;
	}
	function setupStringifier( stringifier ) {
		stringifier.setDefaultObjectToJSOX( objectToJSOX );
		stringifier.registerToJSOX( "~os", objectStorageContainer, objectToJSOX );
		stringifier.store = newStorage;
		for( let f of newStorage.encoders )
			stringifier.registerToJSOX( f.tag, f.p, f.f ) ;

	}
	setupStringifier( newStorage.stringifier );

	//
	// options = {
	//      depth : maximum distance to load...
	// }
	//
	newStorage.map = function( o, opts ) {
		//newStorage.mapping = true;
		//this.parser.
		if( opts && opts.depth === 0 ) return;

		const rootId = newStorage.stored.get( o );
		if( !rootId ) { console.log( "Object was not stored, cannot map" ); return; }

		const rootObj = newStorage.cachedContainer.get( rootId );
		return rootObj.map( opts );
	}

	return newStorage;
	//Object.assign( newStorage
}
sack.ObjectStorage.Thread = {
	post: _objectStorage.Thread.post,
	accept(cb) {
		 _objectStorage.Thread.accept((a,b)=>{
			preloadStorage = b;
			cb(a, sack.ObjectStorage(b) );
		 });
	}
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
	//console.log( "Getting a new container...", container.id );
	container = new objectStorageContainer(obj,options);
	this_.stored.set( obj, container.id );
	this_.cached.set( container.id, container.data );
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
				console.log( "Create index, creating new container", container.id );
				container = new this.objectStorageContainer(obj,options);

				//console.log( "saving stored container.id", obj, container.id );

				//this.stored.delete( obj );
				this_.stored.set( obj, container.id );
				this_.cached.set( container.id, container.data );
				this_.cachedContainer.set( container.id, container );
			}

		}

		container.createIndex( this_, fieldName, opts );

		//console.log( "OUTPUT:", storage );
		res();

	})

}

_objectStorage.prototype.remove = function( opts ) {
	if( "string" === typeof opts )  {
		var container = this.cached.get( opts );
		if( !container ) throw new Error( "This is not a tracked object." );
		this.delete( opts );
	}
	else if( "object" === typeof opts )  {
		var container = this.stored.get( opts );
		if( !container ) throw new Error( "This is not a tracked object." );
		this.delete( container);
	}
}
_objectStorage.prototype.addEncoders = function(encoderList) {
	const this_ = this;
	encoderList.forEach( f=>{
		this_.encoders.push(f);
		this_.stringifier.registerToJSOX( f.tag, f.p, f.f ) ;
	});

}
_objectStorage.prototype.addDecoders = function(encoderList) {
	const this_ = this;
	encoderList.forEach( f=>{
		this_.decoders.push(f);
		//parser.fromJSOX( f.tag, f.p, f.f ) );
	});

}
// this hides the original 'put'
_objectStorage.prototype.put = function( obj, opts ) {
	const this_ = this;

	return new Promise( function(res,rej){

		var container = this_.stored.get( obj );

		_debug && console.log( "Put found object?", container, obj, opts );
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
				if( opts && opts.extraEncoders ) {
					stringifier = sack.JSOX.stringifier();
					this_.setupStringifier( stringifier );
					opts.extraEncoders.forEach( f=>{
						stringifier.registerToJSOX( f.tag, f.p, f.f )
					});
				}else {
					stringifier = this_.stringifier;
				}
				container.encoding = true;
				storage = stringifier.stringify( container );
				container.encoding = false;
				if( !container.id || container.id === "null" ) {
					console.trace( "0) Container has no ID or is nUll", container );
				}
				_debug && console.trace( "WRite:", container, storage );
				this_.writeRaw( container.id, storage );
				return res( container.id );
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

			// raw object; no encoding to set.
			storage = stringifier.stringify( obj );
			if( !opts.id || opts.id === "null" ) {
				console.trace( "Container has no ID or is nUll", container );
			}
			//console.trace( "WRite:", opts, storage );
			this_.writeRaw( opts.id, storage );
			res( opts.id );
		} else if( !opts || !opts.id ) {
			_debug && console.log( "New bare object, create a container...", opts );
                        if( !opts ) opts = { id : sack.id() }
						else opts.id = sack.id();

			container = new this_.objectStorageContainer(obj,opts);
			//console.log( "New container looks like... ", container.id, container );

			//console.log( "saving stored container.id", obj, container.id );

			//this.stored.delete( obj );
			this_.stored.set( obj, container.id );
			this_.cached.set( container.id, container.data );
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

			container.encoding = true;
			storage = stringifier.stringify( container );
			container.encoding = false;
			if( !container.id || container.id === "null" ) {
				console.trace( "Container has no ID or is nUll", container );
			}
			_debug && console.trace( "Outut container to storage... ", container, storage );
			this_.writeRaw( container.id, storage );
			//console.log( "OUTPUT:", storage );
			res(  container.id );
		}
	})
}

/*
_objectStorage.prototype.update( objId, obj ) {

	var container = new this.objectStorageContainer(sack.JSOX.stringify(obj),sign);
	this.stored.set( obj, container.id );
	this.cached.set( container.id, container );
	return container.id;
}

*/

const updatedPrototypes = new WeakMap();


_objectStorage.prototype.get = function( opts ) {
	//this.parser.
	var resolve;
	var reject;

	if( "string" === typeof opts ) {
		opts = { id:opts
		       , extraDecoders : null };
	}
	if( !opts ){
		console.trace( "Must specify options ", opts);
		return null;
	}
	function parserObject( obj ) {
	}
	var reviving = null;
	var pendingRef = null;
	function objectStorageContainerRef(s) {
		//console.log( "Container ref:", s );
		//pendingRef = this;
		try {
			const existing = os.cachedContainer.get(s);
			this.d = {id:s,p:null,res:null,rej:null,i:this,o:null,f:null};
			if( !existing ) {
				this.d.p = new Promise( (res,rej)=>{
					this.d.res = res;
					this.d.rej = rej;
				})
				dangling.push( this );
				objectRefs++;
			} else
				this.d.p = Promise.resolve( existing.data );
		} catch(err) { console.log( "Init failed:", err)}
	}

	function reviveContainer( field, val ) {
		//console.trace( "Revival of a container's field:", this, field, val );
		if( !field ) {
			// finished.
			if( objectRefs ) {
				Object.defineProperty( this, "dangling", { value:dangling } );
				dangling = [];
				objectRefs = 0;
			}
			Object.defineProperty( this, "id", { value:opts.id } );
			return this;
		}
		else {
			{
				const this_ = this;
				// new value isn't anything special; just set the value.
				console.log( "This sort of thing... val is just a thing - like a key part identifier...; but that should have been a container.");
				if( val instanceof Promise ) {
					val.then( (val)=>{
						console.log( "And then later, the object's value is correct?", val,  this_);
						this_.data[field] = val;
					})
					return val;
				}
				// a custom type might want something else...
				if( field === "data" )
					return this.data = val;
				return this.data[field] = val;
			}
			// this is a sub-field of this object to revive...
			//console.log( "Field:", field, " is data?", val)
		}
	}

	function reviveContainerRef( field, val ) {
		//console.trace( "Revival of a container reference:", this, field, val );
		if( !field ) {
			// finished.
			return this.d.p;
		}
		else {
			// this is a sub-field of this object to revive...
			//console.log( "Field:", field, " is data?", val)
		}
	}


	var parser = sack.JSOX.begin( parserObject );
	parser.fromJSOX( "~os", this.objectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
	parser.fromJSOX( "~or", objectStorageContainerRef, reviveContainerRef ); // I don't know ahead of time which this is.
	//console.log( "this has no decoders? ", this );
	if( this.decoders )
		this.decoders.forEach( f=>parser.fromJSOX( f.tag, f.p, f.f ) );
	// allow extra to override default.
	if( opts && opts.extraDecoders ) {
		opts.extraDecoders.forEach( f=>parser.fromJSOX( f.tag, f.p, f.f ) );
	}
	//console.log( "Created a parser for revival..." );

	this.decoding.push( opts );

	var os = this;
	//console.log( "(get)Read Key:", os );
	var p = new Promise( function(res,rej) {
		resolve = res;  reject = rej;
		//console.log( "doing read? (decodes json using a parser...", opts, parser, os );
		try {
		os.read( opts.id
			, parser, (obj)=>{
				// with a new parser, only a partial decode before revive again...
				//console.log( "Read resulted with an object:", obj );
				var found;
				do {
					var found = os.pending.findIndex( pending=>{ console.log( "what is in pending?", pending ); return pending.id === key } );
					if( found >= 0 ) {
						os.pending[found].ref.o[this.pending[found].ref.f] = obj.data;
						os.pending.splice( found, 1 );
					}
				} while( found >= 0 );

			if( obj && ( obj instanceof os.objectStorageContainer ) ){
				//console.log( "GOTzz:", obj, obj.id, obj.data );
				Object.defineProperty( obj, "id", { value:opts.id } );
				os.stored.set( obj.data, obj.id );
				os.cachedContainer.set( obj.id, obj );
				console.log( "and resolve container content")
				res(obj.data);
			} else {
				res(obj)
			}
		} );
		}catch(err) {
			rej(err);
		}
	} );
	return p;
}




function fileEntry( d ) {
	console.log( "New file Entry... ");

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

var failures = 0;
fileEntry.prototype.read = function( from, len ) {
	const this_ = this;
	//console.trace( "READ RESULTING A PROMISE... PLEASE CATCH");
	return new Promise( (res,rej)=>{
		//console.log( "reading:", this );
		if( this_.isFolder ) {
			this_.folder.volume.get( { id:this_.id, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
				.catch( rej )
				.then( (dir)=>{
					//console.log( "Read directory, set folder property..." );
					for( var file of dir.files )  Object.defineProperty( file, "folder", {value:dir} );
					res(dir)
				} );

		} else {
			if( this_.id )
				return this_.folder.volume.get( {id:this_.id} ).then( res ).catch( rej );
			//console.log( "Rejecting, no ID, (no data)", this_ );
			res( undefined ) // no data
		}
	} );

}

fileEntry.prototype.write = function( o ) {
	try {
	if( "string" === typeof o ) {
		//console.log( "direct write of string data:", o );
		this.folder.volume.writeRaw( this.id, o );
		return Promise.resolve( this.id );

	} else if( o instanceof ArrayBuffer ) {
		//console.log( "Write raw buffer" );
		this.folder.volume.writeRaw( this.id, o );
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



function fileDirectory( v, id ) {
	this.files = [];
	// cache chanes instead of flushing every time?
	//Object.defineProperty( this, "changed", {value:false, writable:true} );
	console.trace( "New file Directory... ");
	try {
	if( v ) Object.defineProperty( this, "volume", {value:v} );
	//console.log( "This already has an ID?", this );
	if( id ) Object.defineProperty( this, "id", { value:id } );
	} catch(err) { console.log( "error:", err);}
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
	//console.trace( "Who gets this promise for getting root?");
	return new Promise( (resolve,reject)=>{
		//console.log( "Getting root directory.." );
		return this_.get( { id:result.id, extraDecoders:[ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] } )
			.catch( reject )
			.then( (dir)=>{
				//console.log( "get root directory got:", dir, "(WILL DEFINE FOLDER)" );
				if( !dir ) {
					result.store()
						.then( function(id){
							//console.log( "1) Assigning ID to directory", id );
							Object.defineProperty( result, "id", { value:id } );
							finishLoad( result );
						} )
						.catch( reject );
				}

				// foreach file, set file.folder
				else{
					for( var file of dir.files ) Object.defineProperty( file, "folder", {value:dir} );
					Object.defineProperty( dir, "id", { value:result.id } );
					finishLoad(dir);
				}
				function finishLoad(dir) {
					//console.log( "2) Assigning ID to directory(sub)", result.id );
					Object.defineProperty( dir, "volume", {value:this_} );
					this_.root = dir;

					//console.log( "Don't resolve with this yet?" );
					resolve(dir)  // first come, first resolved
					// notify anyone else that was asking for this...
					if( loading && loading.length ) for( var l of loading ) l.res(dir);
					loading = null; // dont' need this anymore.
				}
			} );
	} );
}

}

