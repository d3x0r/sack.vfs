
module.exports = function(sack) {
const _debug = false;

sack.SaltyRNG.setSigningThreads( require( "os" ).cpus().length );

// save original object.
const _objectStorage = sack.ObjectStorage;
const fillerObject = {};
Object.freeze( fillerObject );

var dangling = [];
var objectRefs = 0;
var currentContainer = null;
var preloadStorage = null;
// manufacture a JS interface to _objectStorage.
sack.ObjectStorage = function (...args) {


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
    currentContainer = this;
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
		console.log( "Uncaught exception:", err );
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
		opts = opts || { depth:0, paths:[] };
		const rootMap = this;
		return new Promise( (res,rej)=>{
			let waiting = 0;
			if( ( "paths" in opts ) &&  opts.paths.length ){
				let nPath = 0;
				const handled = [];
				while(nPath < opts.paths.length ){
					var path = opts.paths[nPath++];
					var obj = this.data;
					for( var step of path ){
						if( obj )
							obj = obj[step];
						else 
							break;
					}
					if( obj ) {
						for( let load of pending ){
							if( load.d.p === obj ){
								//console.log( "This should get marked resolved maybe?")
								handled.push( load );
								loadPending( load );
								obj = null;
								break;
							}
						}
						if( obj ) console.log( "Failed to find promise?");
					}
					else console.log( "Failed to find path in object:", this.data, path );
					for( let h of handled ) {
						var idx = pending.findIndex( p=>p===h );
						if( idx >= 0 ) pending.splice( idx, 1 );
						else console.log( "resolved load was not found???" );
					}
				}
			}
			else{  // load everything that's pending on this object.
				for( let load of pending ) {
					loadPending(load);
				}
				pending.length = 0;
			}
			function loadPending(load) {
				waiting++;
				newStorage.get( {id:load.d.id}).then( (obj)=>{
					load.d.res(obj); // result with real value.
					const exist = newStorage.stored.get( obj );
					const objc = newStorage.cachedContainer.get( exist );
					// resolving this promis on load.d will set this.
					//load.d.r.o[load.d.r.f] = obj;
					if( opts && opts.depth ) {
						objc.map( {depth:opts.depth-1} ).then( (objc)=>{
							waiting--;
							if( !waiting ) {
								//console.log( "1map is resolving with : ", a, rootMap.data );
								res( rootMap.data );
							}
						});
					} else {
						waiting--;
						if( !waiting ) {
							//console.log( "2map is resolving with : ", rootMap.data );
							res( rootMap.data );
						}
					}
				})
			}
		})
	}
	console.log( "Nothing dangling found on object");
	return this.data; // this function is async, just return.
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
	newStorage.parser = null; // this will be filled when .get() is called.
	newStorage.currentParser = null;
	newStorage.root = null; // this gets filled when the root file system is enabled.
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
				newStorage.stored.set( this.data, this.id ); // maybe update the ID though.
				return {data:this.data};
			}
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
	const defaultOpts = {depth:0};
	newStorage.map = function( o, opts ) {
		//newStorage.mapping = true;
		//this.parser.
		if( !opts ) opts = defaultOpts;
		//if( opts && opts.depth === 0 ) return;
		//console.log( "External API invoked map...");
		const rootId = newStorage.stored.get( o );
		if( !rootId ) { console.log( "Object was not stored, cannot map" ); return Promise.resolve( o ); }
		return newStorage.cachedContainer.get( rootId ).map( opts );
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
		if( this.parser )
			this.parser.fromJSOX( f.tag, f.p, f.f );
	});

}

_objectStorage.prototype.getCurrentParseRef = function() {
	if( this.currentParser ){
		return this.currentParser.getCurrentRef();
	}
	return null;
}

// this hides the original 'put'
_objectStorage.prototype.put = function( obj, opts ) {
	const this_ = this;
    if( currentContainer && currentContainer.data === this ) return Promise.resolve( currentContainer.id );

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
			_debug && console.trace( "WRite:", opts, storage );
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
			try {
			this_.writeRaw( container.id, storage );
			}catch(err) { console.log( "WRITE RAW?", this_ )}
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

var currentReadId ;

_objectStorage.prototype.get = function( opts ) {
	//this.parser.
	var resolve;
	var reject;
	var os = this;

	if( "string" === typeof opts ) {
		opts = { id:opts
		       , extraDecoders : null };
	}
	if( !opts ){
		console.trace( "Must specify options ", opts);
		return null;
	}

	if( !this.parser ){
		this.parser = sack.JSOX.begin();
		this.parser.fromJSOX( "~os", this.objectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
		this.parser.fromJSOX( "~or", objectStorageContainerRef, reviveContainerRef ); // I don't know ahead of time which this is.
		for( let f of this.decoders )
			this.parser.fromJSOX( f.tag, f.p, f.f );
	}

	function objectStorageContainerRef( s ) {
		//console.trace( "Container ref:", s );
		try {
			const existing = os.cachedContainer.get(s);
			const here = os.getCurrentParseRef();
			//console.log( "Conainer ref, this will resolve in-place")
			this.d = {id:s,p:null,res:null,rej:null};
			if( !existing ) {
				this.d.p = new Promise( (res,rej)=>{
					this.d.res = res;
					this.d.rej = rej;
				}).then( (obj)=>{
					console.log( "(DOES THIS HAPPEN?)OBJ REPLACE OBJECT WITH:", here, obj )
					return (here.o[here.f] = obj) 
				});
			} else {
				this.d.p = Promise.resolve( existing.data );  // this will still have to be swapped.

			}
			dangling.push( this );
			objectRefs++;
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
			Object.defineProperty( this, "id", { value:currentReadId } );
			return this;
		}
		else {
			{
				const this_ = this;
				// new value isn't anything special; just set the value.
				//console.log( "This sort of thing... val is just a thing - like a key part identifier...; but that should have been a container.");
				if( val instanceof Promise ) {
					console.log( "Value is a promise, and needs resolution.")
					var dangle = dangling.find( d=>d.d.p===val );
					if( dangle )
						dangle.d.n = field;
					this_.data[field] = val
					return val.then( (val)=>{
						console.log( "(DOESN'T HAPPEN NOW?) THIS SHOULD BE WHAT REPLACES THE VALUE", field, val );
						this_.data[field] = val 
					});
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
			return this[field] = val;
		}
	}


	let parser = this.parser;
	if( opts.extraDecoders ) {
		parser = sack.JSOX.begin(  );
		parser.fromJSOX( "~os", this.objectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
		parser.fromJSOX( "~or", objectStorageContainerRef, reviveContainerRef ); // I don't know ahead of time which this is.
		//console.log( "this has no decoders? ", this );
		if( this.decoders && this.decoders.length )
			this.decoders.forEach( f=>parser.fromJSOX( f.tag, f.p, f.f ) );
		// allow extra to override default.
		if( opts && opts.extraDecoders && opts.extraDecoders.length ) {
			opts.extraDecoders.forEach( f=>parser.fromJSOX( f.tag, f.p, f.f ) );
		}
		//console.log( "Created a parser for revival..." );
	}

	this.decoding.push( opts );
	this.currentParser = parser;
	//console.log( "(get)Read Key:", os );
	var p = new Promise( function(res,rej) {
		resolve = res;  reject = rej;
		//console.log( "doing read? (decodes json using a parser...", opts, parser, os );

		const priorReadId = currentReadId;
		try {
			os.read( currentReadId = opts.id
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
					if( !("id" in obj ))
						Object.defineProperty( obj, "id", { value:currentReadId } );
					os.stored.set( obj.data, obj.id );
					os.cachedContainer.set( obj.id, obj );
					currentReadId = priorReadId;
					res(obj.data);
				} else {
					currentReadId = priorReadId;
					res(obj)
				}
			} );
		}catch(err) {
				currentReadId = priorReadId;
			rej(err);
		}
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

var failures = 0;
fileEntry.prototype.read = function( from, len ) {
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
			if( this_.id )
				return this_.folder.volume.get( {id:this_.id} ).then( res ).catch( rej );
			//console.log( "Rejecting, no ID, (no data)", this_ );
			res( undefined ) // no data
		}
	} );

}

fileEntry.prototype.write = function( o ) {
	if( this.id )
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
	try {
		if( v ) Object.defineProperty( this, "volume", {value:v} );
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
		return this.folder.volume.get( {id:file.contents } )
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
	return this.volume.put( this, { id:this.id } );
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
				return  dir.volume.get( {id:file.contents } )
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
					_this.volume.get( {id:file.contents } )
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
	this.addEncoders( [ { tag: "d", p:fileDirectory, f:null}, { tag: "f", p:fileEntry, f:null} ] );
	this.addDecoders( [ {tag: "d", p:fileDirectory }, {tag: "f", p:fileEntry } ] )
	var result = new fileDirectory( this, "?" );
	var this_ = this;
	loading = [];
	//console.trace( "Who gets this promise for getting root?");
	return new Promise( (resolve,reject)=>{
		//console.log( "Getting root directory.." );
		return this_.get( { id:result.id } )
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

