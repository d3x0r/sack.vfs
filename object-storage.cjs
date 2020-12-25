
module.exports = function(sack) {
const _debug = false;
const _debug_dangling = false;
const _debug_output = _debug || false;
const _debug_object_convert = _debug || false;

sack.SaltyRNG.setSigningThreads( require( "os" ).cpus().length );

// save original object.
const _objectStorage = sack.ObjectStorage;
const nativeVol = sack.Volume();
const remoteExtensionsSrc = nativeVol.read( __dirname+"/object-storage-remote.js" );
const remoteExtensions = remoteExtensionsSrc?remoteExtensionsSrc.toString():"// No COde Found";
const jsonRemoteExtensions = JSON.stringify( remoteExtensions );


var allDangling = new Map();
var dangling = [];
var objectRefs = 0;
var currentContainer = null;
var preloadStorage = null; // storage thrown from main thread

// manufacture a JS interface to _objectStorage.
sack.ObjectStorage = function (...args) {

	const newStorage = preloadStorage || new _objectStorage(...args);

// associates object data with storage data for later put(obj) to re-use the same informations.
function objectStorageContainer(o,opts) {
	if( !this instanceof objectStorageContainer ) return new newStorage.objectStorageContainer(o,opts);
	_debug_object_convert && console.trace( "Something creating a new container..", o, opts );
	try {
	if( "string" === typeof o ){
		// still, if the proir didn't resolve, need to resolve this..
		let existing = newStorage.cachedContainer.get( o );
		if( existing ) {
			o = existing.data;
			if( existing.resolve )
				return;
		}
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
	return newStorage;
}
objectStorageContainer.getStore = function() {
	return newStorage;
}

objectStorageContainer.prototype.map = async function( opts ) {
	const dangling = this.dangling; /* this is a property set dynamically */
	if( dangling && dangling.length )  {
		opts = opts || { depth:0, paths:[] };
		const rootMap = this;
		return new Promise( (res,rej)=>{
			let waiting = 0;
			console.trace( "Map Object called... ", dangling)

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
						for( let load of dangling ){
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
						var idx = dangling.findIndex( p=>p===h );
						if( idx >= 0 ) dangling.splice( idx, 1 );
						else console.log( "resolved load was not found???" );
					}
				}
			}
			else{  // load everything that's pending on this object.
				for( let load of dangling ) {
					{
						console.log( "Checking pending..." );
						const existing = newStorage.cachedContainer.get( load.d.id );
						if( existing ) {
							console.log( "Found it as existing, resolve it?", existing.data, load.d );
							continue;
							/*
							if( load.d.res ) load.d.res( existing.data );
							else {
								//return load.d.p;
								load.d.p.then( o2=>{
								console.trace( "Promise was already resolved, add some more then?", o2, existing );

								if( existing.data!==o2) 
									throw new Error( "resolved and loaded object mismatch");
									return o2
								})
								return load.d.p;
							}
							*/
						}
					}
					//console.log( "something:", load );
					loadPending(load);
				}
				// wait until the promise resolves to delete this
				//dangling.length = 0;
			}
			if( !waiting ){
				console.log( "Nothing scheduled to really wait, go ahead and resolve");
				res( rootMap.data );
			}

			function loadPending(load) {
				waiting++;
				newStorage.get( {id:load.d.id}).then( (obj)=>{
					//console.log( "Storage requested:", load.d.id );
					if( load.d.res )
						load.d.res(obj); // result with real value.
					else {
						load.d.p.then( o2=>{if( obj!==o2) throw new Error( "resolved and loaded object mismatch");return o2})
						return load.d.p;
					}
					const exist = newStorage.stored.get( obj );
					const objc = newStorage.cachedContainer.get( exist );
					// resolving this promis on load.d will set this.
					//load.d.r.o[load.d.r.f] = obj;
					if( opts && opts.depth ) {
						objc.map( {depth:opts.depth-1} ).then( (objc)=>{
							waiting--;
							if( !waiting ) {
								//console.log( "1map is resolving with : ", objc, rootMap.data );
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
	_debug_dangling && console.log( "Nothing dangling found on object");
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
		if( this instanceof Promise ) {

			console.log( "This is still a pending object reference(?)", this );
		}
		//  see if we alread stored this... (or are currently storing this.) (back references container)
		_debug_object_convert && console.trace( "THIS GOT CALLED?", this, Object.getPrototypeOf( this ) );
		var exist = newStorage.stored.get( this );
		if( exist ) {
			var obj = newStorage.cachedContainer.get( exist );
			_debug_object_convert && console.log( "Object stored independantly... RECOVERED:", exist, obj.encoding );
			if( obj.encoding ) 
				return this;
			else {
				//console.log( "Why is this an ~or and not ~os?");
				return '~or"'+exist+'"';
			}
		}
		return this;
	}
	function storageObjectToJSOX( stringifier ){
		//  see if we alread stored this... (or are currently storing this.) (back references container)
		_debug_object_convert && console.trace( "THIS GOT CALLED?", this, Object.getPrototypeOf( this ) );
		var exist = newStorage.stored.get( this );
		if( exist ) {
			var obj = newStorage.cachedContainer.get( exist );
			_debug_object_convert && console.log( "Object stored independantly... RECOVERED:", exist, obj.encoding );
			if( obj.encoding ) 
				return this;
			else {
				//console.log( "Why is this an ~or and not ~os?");
				return '~or"'+exist+'"';
			}
		} else {
			if( this instanceof objectStorageContainer ) {
				//console.log( "THIS SHOULD ALREADY BE IN THE STORAGE!", this, newStorage.stored.get( this.data ) );
				// this is probably the final result when encoding this object.
				//newStorage.stored.set( this.data, this.id ); // maybe update the ID though.
				return {data:this.data}; // this object will be stringified, and have our type prefix prepended.
			}
		}
		//console.log( "not a container...");
		return this;
	}

	function setupStringifier( stringifier ) {
		stringifier.setDefaultObjectToJSOX( objectToJSOX );
		stringifier.registerToJSOX( "~os", objectStorageContainer, storageObjectToJSOX );
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

	newStorage.handleMessage = handleMessage

	return newStorage;


	function handleMessage( ws, msg ) {
            console.log( "Storage Remote Message:", msg );
		if( msg.op === "connect" ) {
			ws.send( `{op:connected,code:${jsonRemoteExtensions}}` );
		return true;
		}
		if( msg.op === "get" ) {
			newStorage.readRaw( currentReadId = msg.opts.id
				, (data)=>{
					//console.log( "Read ID:", msg.opts.id, data );
				   ws.send( newStorage.stringifier.stringify( { op:"GET", id:msg.id, data:data } ) );
			} )
			return true;
		}
		if( msg.op === "put" ) {
			// want to get back a file ID if possible...
			// and/or use the data for encoding/salting/etc... which can determine the result ID.
			//console.log( "PUT THIGN:", msg );
			newStorage.writeRaw( msg.rid, msg.data);
			ws.send( { op:"PUT", id:msg.id, r:msg.rid } );
			return true;
		}
		return false;
	}

}
sack.ObjectStorage.getRemoteFragment = function() {
	return remoteExtensions;
}

sack.ObjectStorage.Thread = {
	post: _objectStorage.Thread.post,
	accept(cb) {
		 _objectStorage.Thread.accept((a,b)=>{
			// posted from core thread ( worker can't access disk itself? )
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

_objectStorage.prototype.stringify = function( obj ) {
	const containerId = this.stored.get( obj );
	if( containerId ) {
		const container = this.cachedContainer.get( containerId );
		const stringifier = this.stringifier;
		if( container ) {
			container.encoding = true;
			const storage = stringifier.stringify( container );
			container.encoding = false;
			return storage;
		}
	}
	return stringifier.stringify( container );
	
	
}

// this hides the original 'put'
_objectStorage.prototype.put = function( obj, opts ) {
	const this_ = this;
	if( currentContainer && currentContainer.data === this ) {
			saveObject( null, null );
			_debug && console.log( "Returning same id; queued save to background...")
        	return Promise.resolve( currentContainer.id );
	}else {

	}
	return new Promise( function(res,rej){
		saveObject(res,rej);
	});

	function saveObject(res,rej) {
		if( "string" === typeof obj && opts.id ) {
			console.log( "SAVING A STRING OBJECT" );
			// this isn't cached on this side.
			// we don't know the real object.
			console.log( "Saving a string object" );
			this_.writeRaw( opts.id, obj );
			return res?res( opts.id ):null;
		}
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
				_debug_output && console.trace( "WRite:", container.id, storage );
				this_.writeRaw( container.id, storage );
				return res?res( container.id ):null;
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
			_debug_output && console.trace( "WRite:", opts, storage );
			this_.writeRaw( opts.id, storage );
			res && res( opts.id );
		} else if( !opts || !opts.id ) {
			_debug && console.log( "New bare object, create a container...", opts );
                        if( !opts ) opts = { id : sack.id() }
                        else opts.id = sack.id();
                        if( "object" === typeof obj ) {
				container = new this_.objectStorageContainer(obj,opts);
				//console.log( "New container looks like... ", container.id, container );
				
				//console.log( "saving stored container.id", typeof obj, obj, container.id );
			        
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
			} else {
				container = new this_.objectStorageContainer(obj,opts);
				storage = obj;
			}
			if( !container.id || container.id === "null" ) {
				console.trace( "Container has no ID or is nUll", container );
			}
			_debug && console.trace( "Outut container to storage... ", container, storage );
			try {
				this_.writeRaw( container.id, storage );
				this_.cached.set( container.id, container.data );
				this_.cachedContainer.set( container.id, container );
			}catch(err) { console.log( "WRITE RAW?", this_ )}
			//console.log( "OUTPUT:", storage );
			res && res(  container.id );
		}
	}
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
	const os = this;

	if( "string" === typeof opts ) {
		opts = { id:opts
		       , extraDecoders : null };
	}
	if( !opts ){
		console.trace( "Must specify options ", opts);
		return null;
	}
	{
		const priorLoad = os.cachedContainer.get( opts.id );
		if( priorLoad ) return Promise.resolve( priorLoad.data );
	}

	const priorDecode = this.decoding.find( d=>d.id === opts.id );
	if( priorDecode ){
		return new Promise( (res,rej)=>{
			priorDecode.res = res;
			priorDecode.rej = rej;
		}).then( (obj)=>{
			let deleteId = -1;
			for( let n = 0; n < this.decoding.length; n++ ) {
				if( decode === opts ){
					deleteId = n;
					break;
				}
			}
			if( deleteId >= 0 )  this.decoding.splice( deleteId, 1 );
		})
	}

	if( !this.parser ){
		this.parser = sack.JSOX.begin();
		//console.log( "ADDING ~os");
		this.parser.fromJSOX( "~os", this.objectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
		this.parser.fromJSOX( "~or", objectStorageContainerRef, reviveContainerRef ); // I don't know ahead of time which this is.
		for( let f of this.decoders )
			this.parser.fromJSOX( f.tag, f.p, f.f );
	}

	function objectStorageContainerRef( s ) {
		_debug_dangling && console.trace( "Container ref:", s );
		try {
			const existing = os.cachedContainer.get(s);
			const here = os.getCurrentParseRef();
			const thisDangling = dangling;
			//console.log( "Conainer ref, this will resolve in-place")
			this.d = {id:s,p:null,res:null,rej:null,refobj:null, reffield:null};
			if( !existing ) {
				_debug_dangling && console.log( "PUSHING DANGLING REFERNCE", this );
				const requests = allDangling.get( this.d.id ) || [];
				if( !requests.length ) allDangling.set( this.d.id, requests );
				const p = this.d.p = new Promise( (res,rej)=>{
					_debug_dangling && console.log( "setting up pending promise to resolve:", s );
					this.d.res = res;
					this.d.rej = rej;
				}).then( (obj)=>{
					_debug_dangling && console.log( "(DOES THIS HAPPEN?)OBJ REPLACE OBJECT WITH:", here, obj, thisDangling, s )
					const dr = thisDangling.findIndex( d=> d.d.id === s );
					if( dr >= 0 ) thisDangling.splice( dr, 1 );
					else console.log( "FAILED TO FIND DANGLING REFERENCE" );

					const dp = requests.findIndex( d=> d.d.p ===  p );
					if( dp >= 0 ) {
						requests.splice( dr, 1 );
						if( !requests.length ) {
							_debug_dangling && console.log( "This object fully resolved." );
							allDangling.delete( s );
						}
					}
			
					else console.log( "FAILED TO FIND ALLDANGLING REFERENCE" );

					return (here.o[here.f] = obj) 
				}).catch( (err)=>{
					console.log( "CATCH UNCAUGHT PLEASE DO", err);
				});

				requests.push( this );
				dangling.push( this );
				objectRefs++;

			} else {
				_debug_dangling && console.log( "NOT DANGLING; existing object already exists... " );
				this.d.p = Promise.resolve( existing.data );  // this will still have to be swapped.
			}
		} catch(err) { console.log( "Init failed:", err)}
	}

	function reviveContainer( field, val ) {
		if( !field ) {
			// finished.
			if( objectRefs ) {
				/* sets dangling property on container */
				_debug_dangling && console.log( "Collapse dangling", dangling );
				if( !this.dangling )
					Object.defineProperty( this, "dangling", { value:dangling } );
				else{
					this.dangling.push.apply(this.dangling, dangling )
					//console.log( "Added more to the dangling things..." );
				}
                                // resets to store new objects for next load.
				dangling = [];
				objectRefs = 0;
			}
			Object.defineProperty( this, "id", { value:currentReadId } );

			const request = allDangling.get( currentReadId );
			//console.log( "Revive container final pass... does this resolve?", this, request, allDangling, currentReadId ); 
			if( request ) {
				for( let load of request ) {
					load = load.d;
					//console.log( "Checking dangling:", load, currentReadId )
					//console.log( "Resolve pending promise.", load);
					load.res( this.data );
				}
			}
			return this;
		}
		else {
				const this_ = this;
				// new value isn't anything special; just set the value.
				//console.log( "This sort of thing... val is just a thing - like a key part identifier...; but that should have been a container.");
				if( val instanceof Promise ) {
					//_debug_dangling && console.log( "Value is a promise, and needs resolution.")
					var dangle = dangling.find( d=>d.d.p===val );
					if( dangle )
						dangle.d.n = field;
					this_.data[field] = val
					return val.then( (val)=>{
						_debug_dangling && console.log( "(DOESN'T HAPPEN NOW?) THIS SHOULD BE WHAT REPLACES THE VALUE", field, val );
						this_.data[field] = val 
					});
				}
				// a custom type might want something else...
				if( field === "data" )
				{
					this.data = val;
					return undefined;
				}
				this.data[field] = val;
				return undefined;
		}
	}

	function reviveContainerRef( field, val ) {
		//console.trace( "Revival of a container reference:", this, field, val );
		if( !field ) {
			const existing = os.cachedContainer.get( this.d.id );
			//console.trace( "...", existing);
			if( existing ){
				// even better, don't even store the reference, return the real
				//console.log( "So, just return with the real object to assign. (and remove fom dangling)");
				const id = dangling.find( d=>d.d === this.d );
				objectRefs--;
				if( id >= 0 ) dangling.slice( id, 1 );
				return existing.data;
			} 
			return this.d.p;
		}
		else {
			return val;
		}
	}


	let parser = this.parser;
	if( opts.extraDecoders ) {
		parser = sack.JSOX.begin(  );
		//console.log( "Adding ~os handler");
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
	const p = new Promise( function(res,rej) {
		resolve = res;  reject = rej;
		//console.log( "doing read? (decodes json using a parser...", opts, parser, os );

		const priorReadId = currentReadId;
		try {
			//console.log( "LOADING : ", opts.id, parser.localFromProtoTypes );
			os.read( currentReadId = opts.id
				, parser, (obj,times)=>{
					// with a new parser, only a partial decode before revive again...
					_debug && console.log( "Read resulted with an object:", obj, times );
					let deleteId = -1;
					const extraResolutions = [];
					for( let n = 0; n < os.decoding.length; n++ ) {
						const decode = os.decoding[n];
						if( decode === opts )
							deleteId = n;
						else if( decode.id === opts.id ) {
							decode.res( obj );
						}
					}
					if( deleteId >= 0 )  os.decoding.splice( deleteId, 1 );

					var found;
					do {
						var found = os.pending.findIndex( pending=>{ console.log( "what is in pending?", pending ); return pending.id === key } );
						if( found >= 0 ) {
							os.pending[found].ref.o[os.pending[found].ref.f] = obj.data;
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
					for( let res in extraResolutions ) res.res(obj.data);
					res(obj.data);
				} else {
					currentReadId = priorReadId;
					for( let res in extraResolutions ) res.res(obj);
					//console.log( "RESOLVE WITH OBJECT NEED DATA?", ( obj instanceof os.objectStorageContainer ) );
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

fileDirectory.prototype.create = async function( fileName ) {

	var file = this.files.find( (f)=>(f.name == fileName ) );
	if( file ) {
		console.log( "File already exists, not creating." );
		return null; // can't creeate already exists.
	} else {
		file = new fileEntry( this );
		file.name = fileName;
		this.files.push(file);
		this.store();
		return file;
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

const pendingStore = [];
let lastStore = 0;

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

const storedPromise = Promise.resolve(undefined);
fileDirectory.prototype.store = function(force) {
	if( !force ) {
		if( !pendingStore.find( p=>p===this)) 
			pendingStore.push( this );
		if( !lastStore ) {
			checkPendingStore();
		}
		lastStore = Date.now();
		return storedPromise;
	}
	return this.volume.put( this, { id:this.id } );
}

fileDirectory.prototype.remove = function( fileName ) {
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
			dir.volume.remove( file.id );
			dir.files.splice( fileId, 1 );
			//if( !dir.files.length ) {
			//	dir.volume.remove( dir.id );
			//}
			return true;
		}

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

fileDirectory.prototype.has = async function( fileName ) {
	var parts = splitPath( fileName );
	var part;
	var pathIndex = 0;
	var dir = this;
	async function getOnePath() {
		if( pathIndex >= parts.length ) return true;
		if( !dir ) return false;

		part = parts[pathIndex++];
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
					result.store(true)
						.then( function(id){
							//console.log( "1) Assigning ID to directory", id );
							Object.defineProperty( result, "id", { value:id } );
							finishLoad( result );
						} )
						.catch( reject );
				}

				// foreach file, set file.folder
				else{
					for( var file of dir.files ) {
						//console.log( "File:", dir, file );
						Object.defineProperty( file, "folder", {value:dir} );
					}
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

