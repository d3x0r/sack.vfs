
let currentStorage = null;

module.exports = function(sack) {
const _debug = false;
const _debug_dangling = false;
const _debug_output = _debug || false;
const _debug_object_convert = _debug || false;
const _debug_ll = false; // remote receive message logging.
const _debug_map = false;
const _debug_replace = false;
sack.SaltyRNG.setSigningThreads( require( "os" ).cpus().length );

// save original object.
const _objectStorage = sack.ObjectStorage;
const nativeVol = sack.Volume();
const remoteExtensionsSrc = nativeVol.read( __dirname+"/object-storage-remote.js" );
const remoteExtensions = remoteExtensionsSrc?remoteExtensionsSrc.toString():"// No COde Found";
const jsonRemoteExtensions = JSON.stringify( remoteExtensions );

const RNG= sack.SaltyRNG(  (saltbuf)=>saltbuf.push( "0" ), { mode:1 } );

function shortId( s ) {
	return sack.Id();
      return base64ArrayBuffer( RNG.getBuffer(8*(12)) );
}

const getId = shortId;


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
	//_debug_object_convert && console.log( "Something creating a new container..", o, opts );
	if( "string" === typeof o ){
		if( !o ) throw new Error( "Blank string lookup" );
		// still, if the proir didn't resolve, need to resolve this..
		let existing = newStorage.cachedContainer.get( o );
		if( existing ) {
			o = existing.data;
			if( existing.resolve )
				return;
		}
	}
	if( o ) {
		this.data = o; // reviving we get null opts.
	} else {
		//console.trace( "Hope this path fixes things up..." );
		this.data = "pending reference"; // reviving we get null opts.
	}

	currentContainer = this;
	Object.defineProperty( this, "encoding", { writable:true, value:false } );
	if( opts && opts.sign ) {
		if( !this.nonce ) {
			//console.log( "SIGNING FAILS?" );
			let s;
			const signature = sack.SaltyRNG.sign( s = newStorage.stringifier.stringify(o), opts.sign.pad1, opts.sign.pad2 );
			this.nonce = signature.key;
			//console.log( "Signing returned:", signature );
			this.sign = Object.assign( {e:signature.extent,c:signature.classifier},opts.sign );
			//console.log( "MAKING A NONCE:", s );
			//var v = sack.SaltyRNG.verify( newStorage.stringifier.stringify(o), this.nonce, opts.sign.pad1, opts.sign.pad2 );
			Object.defineProperty( this, "id", { value:signature.id } );
			//console.trace( "This makes it go boom?", opts, this.nonce, o, v.key );
		} else {
		//	var v = sack.SaltyRNG.verify( newStorage.stringifier.stringify(o), this.nonce, opts.sign.pad1, opts.sign.pad2 );
		//	Object.defineProperty( this, "id", { value:v.key } );
		}
	} else {
		if( opts && opts.id ) {
			Object.defineProperty( this, "id", { value:opts.id } );
		}		
	}
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
		_debug_replace && console.log( "doing a map on a thing... amybe this trace?", this.id );
		return new Promise( (res,rej)=>{
			const waiting = [];
			_debug_map && console.log( "Map Object called... dangling:", dangling.length)
			
			if( ( "paths" in opts ) &&  opts.paths.length ){
				let nPath = 0;
				const handled = [];
				while(nPath < opts.paths.length ){
					var path = opts.paths[nPath++];
					let part = 0;
					var obj = this.data;
					for( var step of path ){
						if( obj ) {
							obj = obj[step];
							part++;
						} else 
							break;
					}
					if( obj ) {
						for( let load of dangling ){
							if( load.d.p === obj ){
								//console.log( "This should get marked resolved maybe?")
								handled.push( load );
								load.d.rootMap = rootMap;
								waiting.push( loadPending( load, {depth:opts.depth,paths:part<path.length?[path.slice(part)]:undefined} ) );
								obj = null;
								break;
							}
						}
						
						//	obj will probably also be resolved here, because it's not in pending...
						if( obj ) {
								const ref = newStorage.storedIn.get( obj );
								console.log( "Failed to find promise?", obj, ref );
			
						}
						
					}
					else console.log( "Failed to find path in object:", this.data, path );
					for( let h of handled ) {
						_debug_replace && console.log( "in map - removing dangling reference?" );
						var idx = dangling.findIndex( p=>p===h );
						if( idx >= 0 ) {
							dangling.splice( idx, 1 );
							_debug_replace && console.log( "in map - removed dangling reference?" );
						}
						else _debug_replace && console.log( "resolved load was not found???" );
					}
				}
			}
			else{  // load everything that's pending on this object.
				_debug_replace && console.log( "Load all dangling things anyway:", dangling );
				for( let load of dangling ){
					if( !load.d.rootMap ) {
						load.d.rootMap = rootMap;
						waiting.push( loadPending(load,opts, res) );
					} else {
						//console.log( "Dangling has already been triggered to load", load );
					}
				}
				// wait until the promise resolves to delete this
				//dangling.length = 0;
			}
			if( !waiting.length ){
				//console.log( "Nothing scheduled to really wait, go ahead and resolve");
				res( rootMap.data );
			}
			else {
				_debug_replace && console.log( "(AWAIT!)Waiting on some events to resolve...", waiting );
				Promise.all( waiting ).then( ( last)=>{
					_debug_replace && console.log( "And now we properly resolve.", last );
					res( rootMap.data )
				} );
			}
			

		})
	}
	if(0) // untested.
	if( opts.paths ) {
		// if the first level is a promised value - it would definitely be in dangling and have been checked above.
		// so here, we check path step + 1...
		const nextMaps = opts.paths.reduce( (acc,path)=>path.length>1?(acc.push(path.slice(1)),acc):acc, [] );
		if( nextMaps.length ) { // if any paths more than one level...
			const checks = opts.paths.reduce( (acc,path)=>path.length>1?(acc.push(this.data),acc):acc, [] );
			const nextPaths = opts.paths.reduce( (acc,path)=>path.length>1?(acc.push(path[0]),acc):acc, [] );
			for( let n = 0; n < nextPaths.length; n++ ) {
				if( !checks[n] ) continue; // already traced to the end.
				const step = checks[n][nextPaths[n]]; // this is a field dereference... N is an object.
				if( step instanceof Promise ) {
					const isObject = newStorage.storedIn.get( step );
					if( isObject ) {
						console.log( "This thing should be loaded too?? (ignoring this promise, it is also probalby already handled above..." );
						checks[n] = null;
					}
				}
			}
		}
	}
	_debug_dangling && console.log( "Nothing dangling found on object");
	return this.data; // this function is async, just return.
}


			function loadPending(load, opts) {
				//console.log( "doing real get, which results with ANOTHER promise", load );
				return newStorage.get( {id:load.d.id}).then( (obj)=>{
					//console.log( "Storage requested:", load.d.id );
					if( load.d.res ) {
						load.d.res(obj); // result with real value.
					} else {
						load.d.p.then( o2=>{if( obj!==o2) throw new Error( "resolved and loaded object mismatch");return o2})
						return load.d.p;
					}

					//const existobj = newStorage.stored.get( load.d.p );
					//const exist = newStorage.stored.get( obj );
					//console.log( "This is the promise object?", load.d.id, exist, existobj );
					
					const objc = newStorage.cachedContainer.get( load.d.id );
					_debug_dangling && console.log( "Replaced promise with container", objc, obj );

					// resolving this promis on load.d will set this.
					if( opts && opts.depth ) {
						_debug_replace && console.log( "Mapping the object, if there is any more" );
						objc.map( {depth:opts.depth-1} ).then( (objc)=>{
							_debug_replace && console.log( "mapped sub-object resultinged...", objc );
							load.d.waiting--;
							if( !load.d.waiting ) {
								_debug_dangling && console.log( "1map is resolving with : ", objc, load.d.rootMap.data );
								_debug_replace && console.log( "Resolving map request...", load.d.rootMap.data );
							}else console.log( "waiting for something..." );
						});
					} else {
						_debug_replace && console.log( "waiting for something, not resolving" );
					}
					//console.log( "This is just an object - from a then callback" );
					return obj;
				}).catch( (err)=>{
					console.log( "Error:", err );
				})
			}


	var mapping = false;
	preloadStorage = null;
	newStorage.objectStorageContainer = objectStorageContainer;
	newStorage.cached = new Map();
	newStorage.cachedContainer = new Map();
	newStorage.stored = new WeakMap(); // objects which have alredy been through put()/get()
	newStorage.storedIn = new WeakMap(); // objects which have alredy been through put()/get()
	newStorage.decoding = [];
	newStorage.pending = [];
	newStorage.setupStringifier = setupStringifier;
	newStorage.stringifier = sack.JSOX.stringifier();
	newStorage.parser = null; // this will be filled when .get() is called.
	newStorage.currentParser = null;
	newStorage.root = null; // this gets filled when the root file system is enabled.
	newStorage.encoders = [];
	newStorage.decoders = [];

	const fullObjectMaps = new WeakMap();
	
	let rootContainer = false;
	let rootObjectSet = false;
	let rootObject = null;
	let rootObjectContainer = null;
	let mapFullObject = false;

	function getObjectMap() {
		return {
			members:new WeakMap(),
			
		}
	}

	function objectToJSOX( stringifier ){
		_debug_object_convert && console.log( "Convert Any Object to JSOX:", !!rootObject, typeof this, this );
		if( rootContainer ) {
			rootContainer = false;
			rootObject = this.data;
			rootObjectContainer = this;
			fullObjectMaps.set( rootObjectContainer, this.id );
		}else {
			if( rootObjectSet ) {
				rootObject = this;
				rootObjectContainer = this;
				fullObjectMaps.set( rootObjectContainer, "RootID?" );
			}
			//else
			//	console.log( "Sub Object of something...", this !== rootObjectContainer );
		}
		if( mapFullObject ) 
			if( this !== rootObjectContainer ) {
				const curMap = fullObjectMaps.get( rootObjectContainer );
				const ref = stringifier.getReference( this );
				console.log( "(full map)saving internal reference:", ref, this );
				fullObjectMaps.set( this, ref );
			}
		const isRoot = (rootObjectContainer === this);
		
		if( this instanceof Promise ) {
			_debug_object_convert && console.log( "This is still a pending object reference(?)", this );
		}
		//  see if we alread stored this... (or are currently storing this.) (back references container)
		var exist = newStorage.stored.get( this );
		_debug_object_convert && console.log( "THIS GOT CALLED?", this, Object.getPrototypeOf( this ), exist );
		if( exist ) {
			if( exist[0] == '~' ) {
				// this is a dangling promise
				return exist;
			} 
			var obj = newStorage.cachedContainer.get( exist );
			_debug_object_convert && console.log( "Object stored independantly... RECOVERED:", exist, obj&&obj.encoding );
			_debug_object_convert && console.log( "existing reference...", exist );
			if( obj.encoding ) 
				return this;
			else {
				//console.log( "Why is this an ~or and not ~os?");
				return '~or"'+exist+'"';
			}
		}
		else {
			exist = fullObjectMaps.get( this );
			if( exist ) {
				console.log( "Deep remote reference." );
			} else {
				 _debug_object_convert && console.log( "not a stored object, encode itself." );
			}
		}
		return this;
	}
	function storageObjectToJSOX( stringifier ){
		//  see if we alread stored this... (or are currently storing this.) (back references container)
		_debug_object_convert && console.trace( "THIS GOT CALLED?", this, Object.getPrototypeOf( this ) );
		var exist = newStorage.stored.get( this );
		if( exist ) {
			console.log( "Think this already is existing?", exist );
			if( exist[0] == '~' ) {
				// is a pending promise... 
				return exist;
			}
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
				if( this.nonce )
					return {data:this.data,nonce:this.nonce,sign:this.sign}; // this object will be stringified, and have our type prefix prepended.
				else
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
		if( !rootId ) { console.trace( "Object was not stored, cannot map", o ); return Promise.resolve( o ); }
		if( rootId[0] === '~' ) {
			//console.log( "this is mapping a directly referenced field; which means we have to resolve this at least.", rootId );
			const loading = newStorage.storedIn.get( o );
			//console.log( "Get result:", loading );
			return loadPending( loading );
		}
		//console.trace( "cached container on root ID check", rootId );

		return newStorage.cachedContainer.get( rootId ).map( opts );
	}

	newStorage.handleMessage = handleMessage;
	newStorage.StoredObject = StoredObject;
	
	return newStorage;


	function handleMessage( ws, msg ) {
            _debug_ll && console.log( "Storage Remote Message:", msg );
            try {
		if( msg.op === "connect" ) {
			ws.send( `{op:connected,code:${jsonRemoteExtensions}}` );
		return true;
		}
		if( msg.op === "get" ) {
			newStorage.readRaw( currentReadId = msg.opts.id
				, (data)=>{
					const msgout = newStorage.stringifier.stringify( { op:"GET", id:msg.id, data:data } );
				   //console.log( "Read ID:", msg.opts.id, typeof data, msgout );
				   ws.send( msgout );
			} )
			return true;
		}
		if( msg.op === "put" ) {
			// want to get back a file ID if possible...
			// and/or use the data for encoding/salting/etc... which can determine the result ID.
			//console.log( "PUT THIGN:", msg.id );
			newStorage.writeRaw( msg.rid, msg.data);
			ws.send( { op:"PUT", id:msg.id, r:msg.rid } );
			return true;
		}
                }catch(err) { console.log( "Failed?", err ) }
		return false;
	}
} // closes module.exports


sack.ObjectStorage.StoredObject = StoredObject

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

_objectStorage.prototype.getContainerRef = function( obj, options ) {
	var containerId = this.stored.get( obj );
	if( containerId ) {
		const container = this.storedIn.get( obj );
		//console.log( "ID:", containerId, container );
		return container;
		
	}
	return null;
}
_objectStorage.prototype.getContainer = function( obj, options ) {
	var container = this.stored.get( obj );
	var storage;
	if( container ) {
		container = this.cachedContainer.get( container );
		return container;
	}
	//console.trace( "Getting a new container...", options );
	container = new this.objectStorageContainer(obj,options);
	//console.log( " *** SETTING ID HERE", container.id);
	//if( container.id === undefined ) throw new Error( "Error along path of setting container ID");
	if( container.id ) {
		this.stored.set( obj, container.id );
		this.cached.set( container.id, container.data );
		this.cachedContainer.set( container.id, container );
	}else {
		// should fix 'undefined' as a index in stored
		//console.log( "Storage still has to come from real storage operation... not setting cached ID");
	}
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
			if( container.nonce ) {
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
				//console.log( " *** SETTING ID HERE", container.id);
				this_.stored.set( obj, container.id );
				if( container.id === undefined ) throw new Error( "Error along path of setting container ID");
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
		this_.encoders.push( f );
		//console.log( "encoders:", encoderList );
		this_.stringifier.registerToJSOX( f.tag, f.p, f.f ) ;
	});

}
_objectStorage.prototype.addDecoders = function(encoderList) {
	const this_ = this;
	encoderList.forEach( f=>{
		function redirect(f) {
			return function redirect(field,val) {
				if(f ) {
					if( !field ) { 
						return f.call( this, field, this_ ); 
					} else return f.call(this,field,val ) 
				}else {
					if( !field ) {
						if( this instanceof StoredObject ){
							this.loaded( this_, currentReadId );
						}
						return this;
					} else return val;
				}
			}
		}
		f.f = redirect(f.f);
		this_.decoders.push( f );
		if( this.parser )
			this.parser.fromJSOX( f.tag, f.p, f.f );
	});

}

_objectStorage.prototype.getCurrentParseRef = function() {
	if( this.currentParser ){
		return this.currentParser.currentRef();
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
	const output = stringifier.stringify( container );
	console.log( "stringify default:", output, container ) ;
	return output;
	
	
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
			this_.writeRaw( opts.id, obj );
			return res?res( opts.id ):null;
		}
		var container = this_.stored.get( obj );
		if( !container && opts && opts.id ) {
			const oldObj = this_.cached.get( opts.id );
			if( oldObj )
				container = this_.stored.get( oldObj );
		}
		_debug && console.log( "Put found object?", container, obj, opts );
		if( container ) {
			container = this_.cachedContainer.get( container );
			if( obj !== container.data ) {
				//console.log( "Overwrite old data with new?", container.data, obj );
				container.data = obj;
			}
			if( !container.nonce ) {
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

				rootContainer = true;
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
			rootObjectSet = true;
			storage = stringifier.stringify( obj );
			if( !opts.id || opts.id === "null" ) {
				console.trace( "Container has no ID or is nUll", container );
			}
			_debug_output && console.trace( "WRite:", opts, storage );
			//console.log( "Storing with an ID already?", obj, opts );
			this_.writeRaw( opts.id, storage );
			res && res( opts.id );
		} else if( !opts || !opts.id ) {
			if( opts && opts.mapObject ) {
				
			}
			_debug && console.log( "New bare object, create a container...", opts );
                        if( !opts ) opts = { id : shortId() }
                        else opts.id = shortId();
                        //console.log( "Storage ID is picked here:", opts );
                        if( "object" === typeof obj ) {
				container = new this_.objectStorageContainer(obj,opts);
				if( !container.id || "string" !== typeof container.id ) throw new Error( "Failed to get a conatiner ID for some reason" );
				//console.log( "New container looks like... ", container.id, container );
				
				//console.log( "saving stored container.id", typeof obj, obj, container.id );
			        
				//this.stored.delete( obj );
				//console.log( " *** SETTING ID HERE", container.id);
				this_.stored.set( obj, container.id );
				if( container.id === undefined ) throw new Error( "Error along path of setting container ID");
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
				rootContainer = true;
				storage = stringifier.stringify( container );
				container.encoding = false;
			} else {
				container = new this_.objectStorageContainer(obj,opts);
				storage = obj;
			}
			if( !container.id || container.id === "null" ) {
				console.trace( "Container has no ID or is nUll", container );
			}
			_debug_output && console.trace( "Outut container to storage... ", container, storage );
			try {
				this_.writeRaw( container.id, storage );
				if( container.id === undefined ) throw new Error( "Error along path of setting container ID");
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
		//console.log( "already found?", priorLoad );
		if( priorLoad ) return Promise.resolve( priorLoad.data );
	}
	//console.trace( "TEST going to get:", opts )
	const priorDecode = this.decoding.find( d=>d.opts.id === opts.id );
	if( priorDecode ){
		
		console.trace( "already decoding...(use same promised result) things?", priorDecode );
		return priorDecode.p;
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
		_debug_dangling && console.log( "Container ref:", s );
		try {
			const existing = os.cachedContainer.get(s);
			const here = os.getCurrentParseRef();
			const thisDangling = dangling;
			//console.log( "Conainer ref, this will resolve in-place")
			this.d = {id:s,p:null,res:null,rej:null,r:{o:here.o,f:here.f},rootMap:null };

			if( !existing ) {
				_debug_dangling && console.log( "PUSHING DANGLING REFERNCE", this );
				const requests = allDangling.get( this.d.id ) || [];
				if( !requests.length ) allDangling.set( this.d.id, requests );
				const p = this.d.p = new Promise( (res,rej)=>{
					_debug_dangling && console.log( "setting up pending promise to resolve:", s );
					this.d.res = res;
					this.d.rej = rej;
				}).then( (obj)=>{
					_debug_dangling && console.log( "replace value at reference with real value:", here, obj, thisDangling.length, s )

					//const dr = thisDangling.findIndex( d=> d.d.id === s );
					//if( dr >= 0 ) thisDangling.splice( dr, 1 );
					//else _debug_dangling && console.log( "FAILED TO FIND DANGLING REFERENCE 1" );

					// this is the last time the promise exists...
					os.storedIn.delete( p );
					os.stored.delete( p );
					const inObject = dangling.find( d=>d === this );
					if(inObject >= 0 ) {
						_debug_replace && console.log( "Removing request from container dangling" );
						dangling.spice( inObject, 1 );
					}
					_debug_replace && console.log( "-------- REPLACE VALUE HERE:", this.d.r.o[this.d.r.f], this.d.r.o, this.d.r.f, obj );
					return (this.d.r.o[this.d.r.f] = obj);
				}).catch( (err)=>{
					console.log( "CATCH UNCAUGHT PLEASE DO", err);
				});
				os.storedIn.set( this.d.p, this );
				//console.log( " *** SETTING ID HERE", s);
				os.stored.set( this.d.p, '~or"'+s+'"' );
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
		//console.log( "(JS)Revive container:", this, field, val );
		if( !field ) {
			// finished.
			if( objectRefs ) {
				/* sets dangling property on container */
				_debug_dangling && console.log( "Collapse dangling:", dangling.length );
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

			//console.log( "handle object ??", this );
			if( this.nonce ) {
				let s = null;
	       			var v = sack.SaltyRNG.verify( s = os.stringifier.stringify(this.data), this.nonce, this.sign.pad1, this.sign.pad2 );
				if( v.key !== currentReadId )  {
					throw new Error( `Data corrupted: s:'${s}' v:${v}, this Id:${currentReadId}` );
				}
				else console.log( "Valid record.", v, this.sign );

			}

			const request = allDangling.get( currentReadId );

			//console.log( "Revive container final pass... does this resolve?", this, request, allDangling, currentReadId ); 
			if( request ) {
				_debug_replace && console.log( "Dispatching all resolutions waiting on this object..." );
				allDangling.delete( currentReadId );
				for( let load of request ) {
					load.d.res( this.data );
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
					//console.log( "This should have fixed the 'pending value' value" );
					this.data = val;
					return undefined;
				}
		
				this[field] = val;
				return undefined;
		}
	}

	function reviveContainerRef( field, val ) {
		//console.trace( "Revival of a container reference:", this, field, val );
		if( !field ) {
			const existing = os.cachedContainer.get( this.d.id );
			if( existing ){
				//console.log( "duplicate reference... return the existing promise(but this is still used later)", existing, this.d.id);
				// even better, don't even store the reference, return the real
				//console.log( "So, just return with the real object to assign. (and remove fom dangling)");
				const id = dangling.find( d=>d.d === this.d );
				objectRefs--;
				if( id >= 0 ) dangling.splice( id, 1 );
				//console.log( "this is the real value anyway1...", existing.data );
				return existing.data;
			} 
			os.stored.set( this.d.p, '~or"'+this.d.id+'"' );
			//console.log( "this is the real value anyway2...", this.d.p );
			return this.d.p;
		}
		else {
			//console.log( "this is the real value anyway3...", val );
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
	this.currentParser = parser;
	//console.log( "(get)Read Key:", opts.id );
	const p = new Promise( function(res,rej) {
		resolve = res;  reject = rej;

		const priorReadId = currentReadId;
		try {
			currentStorage = os; // this part is synchronous... but leaves JS heap from os.read(), does callbacks using parser, but results with a parsed object
				              // all synchronously.
			os.read( currentReadId = opts.id
				, parser, (obj,times)=>{
					// with a new parser, only a partial decode before revive again...
					_debug && console.log( "Read resulted with an object:", obj, times );
					let deleteId = -1;
					currentStorage = null;
					const extraResolutions = [];
					for( let n = 0; n < os.decoding.length; n++ ) {
						const decode = os.decoding[n];
						//console.log( "pending decode?", decode, opts, decode.opts === opts )
						if( decode.opts === opts )
							deleteId = n;
						else if( decode.opts.id === opts.id ) {
							console.log( "pending decode resolve?");
							decode.res( obj );
						}
					}
					
					if( deleteId >= 0 )  {
						console.log( "Something else is alwso waiting for this result...", os.decoding, deleteId);
						os.decoding.splice( deleteId, 1 );
					}
					
					var found;
					do {
						var found = os.pending.findIndex( pending=>{ console.log( "what is in pending?", key, opts.id, pending ); return pending.id === opts.id } );
						if( found >= 0 ) {
							os.pending[found].ref.o[os.pending[found].ref.f] = obj.data;
							os.pending.splice( found, 1 );
						}
					} while( found >= 0 );

					if( obj && ( obj instanceof os.objectStorageContainer ) ){
						//console.log( "GOTzz:", obj, obj.id, obj.data );
						if( !("id" in obj ))
							Object.defineProperty( obj, "id", { value:currentReadId } );

						//console.log( "Object.data is bad?", obj.data, typeof obj.data, "\n!!!!!!!!!!", obj.id );
						//console.log( " *** SETTING ID HERE");
						os.stored.set( obj.data, obj.id );
						os.cachedContainer.set( obj.id, obj );
						currentReadId = priorReadId;
						for( let res in extraResolutions ) res.res(obj.data);
						resolve(obj.data);
					} else {
						currentReadId = priorReadId;
						for( let res in extraResolutions ) res.res(obj);
						//console.log( "RESOLVE WITH OBJECT NEED DATA?", ( obj instanceof os.objectStorageContainer ) );
						resolve(obj)
					}
			} );
		}catch(err) {
			//console.log( "ERROR:", err );
			currentReadId = priorReadId;
			rej(err);
		}
	} );
	//console.log( "PUSHING THING TO LOAD LATER (decoding)?", opts );

	this.decoding.push( { p:p, opts:opts } );
	p.then( (r)=>{
		//console.log( "Removing decoding object with then...");
		const doneDecoding = os.decoding.findIndex( d=>d.opts === opts );
		//console.log( "decode should already be resolved:", doneDecoding, os.decoding, opts, r );

		if( doneDecoding >=0 ) os.decoding.splice( doneDecoding, 1 );
		else console.log( "Failed to find decoding object?" );
		return r;
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
			//console.log( "Reading ...", this_.id );
			if( this_.id )
				return this_.folder.volume.get( {id:this_.id} ).then( res ).catch( rej );
			//console.log( "Rejecting, no ID, (no data)", this_ );
			//console.log( "no file data in it?" );
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
	return !!this.files.find( (f)=>(f.name == file ) );
	
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
                return file;
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
	//console.log( "OPEN?", file );
	if( !file ) {
		return Promise.reject( new Error( "File not found:" + fileName ) );
	}
	return Promise.resolve( file );
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



//import {sack} from "../../../vfs_module.mjs"
//import {BloomNHash} from "./bloomNHash.mjs"


class StoredObject {
	#id = null;
	#storage = null;
	get id() { 
		return this.#id;
	} 
	async store(opts) {
		if( !this.#id ) {
			// might have been reloaded...
			const container = this.#storage.getContainer( this );
			if( container ) this.#id = container.id;
			opts = opts || {id:this.#id};
			opts.id = opts.id || this.#id;
			const id = await this.#storage.put( this, opts );
			if( !this.#id ) this.#id = id;
			else if( this.#id !== id ) { console.log( "Object has been duplicated: old/new id:", this.#id, id ); }
			return id;
		} else {
			opts = opts || {id:this.#id};
			opts.id = opts.id || this.#id;
			const id = await this.#storage.put( this, opts );
			if( this.#id !== id ) { console.log( "Object has been duplicated: old/new id:", this.#id, id ); }
			return id;
		}
	}
	hook( storage ) {
		if( storage instanceof StoredObject ) {
			this.#storage = storage.#storage;
		}else
			this.#storage = storage;
	}

	loaded( storage,id ) {
		this.#storage = storage;
		this.#id = id;
	}
	constructor( storage ) {
        	if( !storage ) {
	        	//console.trace( "should have storage on create..", storage, currentStorage );
			storage = currentStorage;
	        	if( !storage ) {
		        	console.trace( "should have storage on create..", storage, currentStorage );
			}
		}
		if( storage ) this.#storage = storage;
	}
	get storage() {
		return this.#storage;
	}
}


