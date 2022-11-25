
const _debug_osr = false; // object storage remote ( general debugging added in forked copy)

var dangling = [];
var objectRefs = 0;
var currentContainer = null;
var preloadStorage = null;
//let remote = null;
let sack = null;
let unique = 0;
const _debug_ll = false; // /low level message receive
const _debug_put = false;
const _debug_get = false;
const _debug_get_ll = false; // after read before parse
const _debug_stringify = false;
const _debug_danging = false;
const _debug_map = false; // reloading related object debugging

let util; import( "util" ).then( u=>{ util = u
	decoder = new u.TextDecoder();
} );
//import util from "util";
//import process from "process";
//process.stdout.write( "blah?" );
try {
	JSOX.updateContext();
}catch(err) {
	console.log( "Let me guess.... no update?", err );
}

const this_ = {
	this || globalThis;
}

this_.ObjectStorage = ObjectStorage;
console.log( "Updating this object to include ObjectStorage?", this_ );
//this.storage = new ObjectStorage();

export function ObjectStorage( sack_, ws ) {
	const sack = sack_;
        this.sack = sack_;
	this.ws = ws;
	ws.storage = this;
	//console.log( "Initialize as object storage?", this );
	//const newStorage = await connect( remote );
	const newStorage = this;
        const requests = new Map();

	// associates object data with storage data for later put(obj) to re-use the same informations.
	function objectStorageContainer(o,opts) {
		if( !( this instanceof objectStorageContainer
                      ||  Object.getPrototypeOf( this ).constructor.name === "objectStorageContainer" )
                  ) return new newStorage.objectStorageContainer(o,opts);
                if( "undefined" !== typeof o ) {
                        //  this callback is used often from JSOX, and all parameters are undefined....
                        // content of the container is filed in later instead of in the constructor.
                        //console.log( "making a new storage container:-----------------------------------------------------", o );
			//console.trace( "Something creating a new container..", o, opts );
			try {
			if( "string" === typeof o ){
				// still, if the proir didn't resolve, need to resolve this..
				let existing = newStorage.cachedContainer.get( o );
				if( existing ) {
					o = existing.data;
					if( existing.resolve )
						return ;
				}
			}

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
				//nonce : opts?opts.sign?sack.SaltyRNG.sign( JSOX.stringify(o), 3, 3 ):null:null,
				data : o
			}
		*/
			if( opts && opts.sign ) {
				var v = this_.sack.SaltyRNG.verify( JSOX.stringify(o), this.data.nonce, 3, 3 );
				//console.log( "TEST:", v );
				Object.defineProperty( this, "id", { value:v.key } );
				v.key = this.data.nonce;
				this.data.nonce = v;
			} else {
				if( opts && opts.id ) {
                                	_debug_get && console.log( "SOMETHING", opts );
					Object.defineProperty( this, "id", { value:opts.id } );
                                        _debug_get && console.log( "Conditioning THis with a ID:", opts.id, this.id );
				}
			}
			}catch(err) {
				console.log( "Uncaught exception:", err );
			}
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
		const pending = this.dangling;
                _debug_get && console.log( "pending:", this, this.pending );
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
						{
							const existing = newStorage.cachedContainer.get( load.d.id );
							if( existing ) {
								if( load.d.res ) load.d.res( existing.data );
								else {load.d.p.then( o2=>{
									if( existing.data!==o2) 
										throw new Error( "resolved and loaded object mismatch");
										return o2
									})
									return load.d.p;
								}
							}
						}
						loadPending(load);
					}
					pending.length = 0;
				}
				if( !waiting ){
					console.log( "Nothing scheduled to really wait, go ahead and resolve");
					res( rootMap.data );
				}

				function loadPending(load) {
					waiting++;
					newStorage.get( {id:load.d.id}).then( (obj)=>{
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
	newStorage.stringifier = JSOX.stringifier();
	newStorage.parser = null; // this will be filled when .get() is called.
	newStorage.currentParser = null;
	newStorage.root = null; // this gets filled when the root file system is enabled.
	newStorage.encoders = [];
	newStorage.decoders = [];
	newStorage.remotes = [newStorage];


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
			if( this instanceof objectStorageContainer
                            || Object.getPrototypeOf( this ).constructor.name === "objectStorageContainer"
                          ) {
				//console.log( "THIS SHOULD ALREADY BE IN THE STORAGE!", this, newStorage.stored.get( this.data ) );
                            	_debug_put && console.log( "Storing object:", this.data, this.id );
				newStorage.stored.set( this.data, this.id ); // maybe update the ID though.
				//return thsi{data:this.data};
			}
		}
		//console.log( "not a container...");
		return this;
	}
	function setupStringifier( stringifier ) {
            	_debug_stringify && console.log( "Stringifier SETUP - add ~os" );
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
		_debug_map && console.log( "External API invoked map...", o, newStorage.stored );
		const rootId = newStorage.stored.get( o );
		if( !rootId ) { console.log( "Object was not stored, cannot map" ); return Promise.resolve( o ); }
		return newStorage.cachedContainer.get( rootId ).map( opts );
	}

	newStorage.handleMessage = handleMessage
        
        // this is an internal interface, used to do a request for real information.
	newStorage. remote = {
		get(opts){
                    //console.log( "this get?" );
			return new Promise( (res,rej)=>{
				const msg = {op:"get", id:unique++, opts:opts, res:res,rej:rej };
				ws.send( msg );
				requests.set( msg.id, msg );
			} )
		},
		put(obj, rid,res,rej){
			const msg = {op:"put", id:unique++, data:obj, rid:rid, res:res,rej:rej };
			ws.send( msg );
			requests.set( msg.id, msg );
		},
		handleMessage : handleMessage
	}

	
	function handleMessage( msg ) {
            _debug_ll && console.log( "Storage Remote Handler:", msg );
            try {
		if( msg.op === "GET" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
                                        //console.log( "Found request?", req );
                                        _debug_get && console.log( "result:", msg );
			const data = (msg.data && ( msg.data instanceof ArrayBuffer
                                      || Object.getPrototypeOf( msg.data ).constructor.name === "ArrayBuffer"

                                     ))?decoder.decode( msg.data )
                                            : msg.data;
                        		//if(
                        		
                        		_debug_get && console.log( "Resolving request?", req, data, decoder.decode(msg.data) );
					if( req ) req.res( data );
                                        return true;
				}
			if( msg.op === "Get" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
                                        _debug_get && console.log( "Rejecting get reply..", msg.err );
					if( req ) req.rej( msg.err );
                                        return true;
				}
			if( msg.op === "PUT" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
					if( req && req.res ) {
                                            // sometimes it's write-only?
                                            _debug_put && console.log( "Something",req );
                                            req.res( msg.r );
                                        }
                                        return true;
				}
			if( msg.op === "Put" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
					if( req ) {
                                            _debug_put && console.log( "Req:", req );
                                            req.rej( msg.err );
                                        }
                                        return true;
				}
                }catch(err){
	                console.log( "something message error:", err );
                }
                return false;
	}

	return newStorage;
}



ObjectStorage.prototype.getContainer = function( obj, options ) {
	var container = this.stored.get( obj );
	var storage;
	if( container ) {
		container = this.cachedContainer.get( container );
		return container;
	}
        if( !options.exist ) {
		//console.log( "Getting a new container...", container.id );
		container = new objectStorageContainer(obj,options);
                console.log( "Manufacture new container.", object, container.id );
		this.stored.set( obj, container.id );
		this.cached.set( container.id, container.data );
		this.cachedContainer.set( container.id, container );
        	return container;
        }
}

ObjectStorage.prototype.remove = function( opts ) {
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
ObjectStorage.prototype.addEncoders = function(encoderList) {
	const this_ = this;
	encoderList.forEach( f=>{
		this_.encoders.push(f);
		this_.stringifier.registerToJSOX( f.tag, f.p, f.f ) ;
	});

}
ObjectStorage.prototype.addDecoders = function(encoderList) {
	const this_ = this;
	encoderList.forEach( f=>{
		this_.decoders.push(f);
		if( this.parser )
			this.parser.fromJSOX( f.tag, f.p, f.f );
	});

}

ObjectStorage.prototype.getCurrentParseRef = function() {
	if( this.currentParser ){
		return this.currentParser.getCurrentRef();
	}
	return null;
}

ObjectStorage.prototype.stringify = function( obj ) {
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
ObjectStorage.prototype.put = function( obj, opts ) {
	const this_ = this;
    if( currentContainer && currentContainer.data === this ) return Promise.resolve( currentContainer.id );

	return new Promise( function(res,rej){

		var container = this_.stored.get( obj );

		if( !container && opts && opts.id ) {
                	const oldObj = this_.cached.get( opts.id );
                        if( oldObj )
	                        container = this_.stored.get( oldObj );
                        console.log( "new object, old ID, ..." );
               	}

		_debug_osr && console.log( "Server Put found object?", container, obj, opts );
		if( container ) {
			container = this_.cachedContainer.get( container );
			if( obj !== container.data ) {
                            	console.log( "Overwrite old data with new?", container.data, obj );
                                coontainer.data = obj;
                       	}

			if( !container.nonce ) {
				// make sure every item that is in an index
				// has been written...

				var stringifier;
				if( opts && opts.extraEncoders ) {
                                    	_debug_stringify && console.log( "New stringifier because of optional encoders" );
					stringifier = JSOX.stringifier();
					this_.setupStringifier( stringifier );
					opts.extraEncoders.forEach( f=>{
						stringifier.registerToJSOX( f.tag, f.p, f.f )
					});
				}else {
                                    	_debug_stringify && console.log( "use stroage stringifier" );
					stringifier = this_.stringifier;
				}
				container.encoding = true;
				storage = stringifier.stringify( container );
                                _debug_stringify && console.log( "Stringified container into:", storage, container );
				container.encoding = false;
				if( !container.id || container.id === "null" ) {
					console.trace( "0) Container has no ID or is nUll", container );
				}
				_debug_osr && console.log( "WRite:", container, storage );
				for( var remote of this_.remotes ) {
					remote.remote.put( storage, container.id, res, rej );
					res = null;
					rej = null;
				}
				//this_.writeRaw( container.id, storage );
				//return res( container.id );
			} else {
				throw new Error( "record is signed, cannot put" );
			}
		}

		if( opts && opts.id ) {
			var stringifier;
			if( opts.extraEncoders ) {
				stringifier = JSOX.stringifier();
				this_.setupStringifier( stringifier );
				opts.extraEncoders.forEach( f=>{
					stringifier.registerToJSOX( f.tag, f.p, f.f )
				});
			}else {
				stringifier = this_.stringifier;
			}

			// raw object; no encoding to set.
			storage = stringifier.stringify( obj );
                        _debug_stringify && console.log( "Stringified object into ", storage, obj );
			if( !opts.id || opts.id === "null" ) {
				console.trace( "Container has no ID or is nUll", container );
			}
			_debug_osr && console.trace( "WRite:", opts, storage );
	                for( var remote of this_.remotes ) {
                           try {
        	            remote.remote.put( storage, opts.id, res, rej )
                            }catch(err){console.log( "ERR:",err)}
                            res = null;
                            rej = null;
        	        }
			//this_.writeRaw( opts.id, storage );
			//res( opts.id );
		} else if( !opts || !opts.id ) {
			_debug_osr && console.log( "New bare object, create a container...", opts );
                        if( !opts ) opts = { id : this_.sack.id() }
						else opts.id = this_.sack.id();

			container = new this_.objectStorageContainer(obj,opts);
			//console.log( "New container looks like... ", container.id, container );

			//console.log( "saving stored container.id", obj, container.id );

			//this.stored.delete( obj );
                        _debug_put && console.log( "Created new container for object(2)", obj );
			this_.stored.set( obj, container.id );
			this_.cached.set( container.id, container.data );
			this_.cachedContainer.set( container.id, container );

			var stringifier;
			if( opts.extraEncoders ) {
                            	_debug_stringify && console.log( "Need a custom stringifier because of options" );
				stringifier = JSOX.stringifier();
				this_.setupStringifier( stringifier );
				opts.extraEncoders.forEach( f=>{
					stringifier.registerToJSOX( f.tag, f.p, f.f )
				});
			}else {
                            	_debug_stringify && console.log( "Using this_ stringifier" );
				stringifier = this_.stringifier;
			}

			container.encoding = true;
			storage = stringifier.stringify( container );
                        _debug_stringify && console.log( "Sringified container(2) into", storage, container );
			container.encoding = false;
			if( !container.id || container.id === "null" ) {
				console.trace( "Container has no ID or is nUll", container );
			}
			_debug_osr && console.trace( "Outut container to storage... ", container, storage );
			try {
		                for( var remote of this_.remotes ) {
        		            remote.remote.put( storage, container.id, res, rej );
                	            res = null;
                        	    rej = null;
	        	        }
			//this_.writeRaw( container.id, storage );
			}catch(err) { 
				console.log( "error WRITE RAW?", this_ )
			}
			//console.log( "OUTPUT:", storage );
			//res(  container.id );
		}
	})
}

/*
ObjectStorage.prototype.update( objId, obj ) {

	var container = new this.objectStorageContainer(JSOX.stringify(obj),sign);
	this.stored.set( obj, container.id );
	this.cached.set( container.id, container );
	return container.id;
}

*/

const updatedPrototypes = new WeakMap();

var currentReadId ;
const ackObjects = [];

ObjectStorage.prototype.get = function( opts ) {
	//this.parser.
	const os = this;
        const extraDecoders = opts && opts.extraDecoders || null;
	_debug_stringify && console.log( "This is the big get... ");
	if( "string" === typeof opts ) {
		opts = { id:opts
		       };
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
		this.parser = JSOX.begin(reviveObject);
		this.parser.fromJSOX( "~os", this.objectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
		this.parser.fromJSOX( "~or", objectStorageContainerRef, reviveContainerRef ); // I don't know ahead of time which this is.
		for( let f of this.decoders )
			this.parser.fromJSOX( f.tag, f.p, f.f );
	}


	function reviveObject(o ) {
		// this callback updates from time to time so....
            	// this is JSOX parse callback, called every time there is a object revived.
                // this will be the next object to satisfy ack.

		_debug_get_ll && console.log( "Resolved buffer (pending):", ackObjects.length, o, o.dangling );

		const ackObject = ackObjects.shift();
		if( ackObject ) {
                    	//console.log( "ACK:", ackObject );
			ackObject.cb(o,ackObject.id); // this is internal callback
			ackObject.res( o ); // this is final return to waiter.
		}
	}

	function objectStorageContainerRef( s ) {
		_debug_get && console.log( "Container ref:", s );
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
					_debug_get && console.log( "(DOES THIS HAPPEN?)OBJ REPLACE OBJECT WITH:", here, obj )
					return (here.o[here.f] = obj) 
				});
			} else {
				this.d.p = Promise.resolve( existing.data );  // this will still have to be swapped.
			}
			dangling.push( this );
                        _debug_get && console.log( "adding a dangling object...", dangling );
			objectRefs++;
		} catch(err) { console.log( "Init failed:", err)}
	}

	function reviveContainer( field, val ) {
		//console.trace( "Revival of a container's field:", this, field, val );
		if( !field ) {
			// finished.
                    	_debug_get && console.log( "Final container revive:", objectRefs, this.id );
			if( objectRefs ) {
				Object.defineProperty( this, "dangling", { value:dangling } );
				dangling = [];
				objectRefs = 0;
			}
                        _debug_get && console.log( "CURRENT READID:",currentReadId );
			Object.defineProperty( this, "id", { value:currentReadId } );
			return this;
		}
		else {
			{
				const this_ = this;
				// new value isn't anything special; just set the value.
				//console.log( "This sort of thing... val is just a thing - like a key part identifier...; but that should have been a container.");
				if( val instanceof Promise
                                    || Object.getPrototypeOf( val ).constructor.name === "Promise"

                                  ) {
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
			const existing = os.cachedContainer.get( this.d.id );
			if( existing ){
				// even better, don't even store the reference, return the real
				console.log( "So, just return with the real object to assign. (and remove fom dangling)");
				const id = dangling.find( d=>d.d === this.d );
				objectRefs--;
				if( id >= 0 ) dangling.slice( id, 1 );
				return existing.data;
			} 
			// finished.
                        _debug_get && console.log( "This promise is hung, this reference has..." );
			return this.d.p; // set actual field to the promise
		}
		else {
			// this is a sub-field of this object to revive...
			//console.log( "Field:", field, " is data?", val)
			return this[field] = val;
		}
	}


	let parser = this.parser;
	if( extraDecoders ) {
            	// uses a specific parser for this instance...
		parser = JSOX.begin(reviveObject  );
		parser.fromJSOX( "~os", this.objectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
		parser.fromJSOX( "~or", objectStorageContainerRef, reviveContainerRef ); // I don't know ahead of time which this is.
		//console.log( "this has no decoders? ", this );
		// allow extra to override default.
		if( this.decoders && this.decoders.length )
			for( let f of this.decoders ) { parser.fromJSOX( f.tag, f.p, f.f ) };
		if( extraDecoders.length ) {
			for( let f of extraDecoders ) { parser.fromJSOX( f.tag, f.p, f.f ); }
		}
		//console.log( "Created a parser for revival..." );
	}

	this.decoding.push( opts );
	this.currentParser = parser;
	//console.log( "(get)Read Key:", os );
	const p = new Promise( function(res,rej) {
		//console.log( "doing read? (decodes json using a parser...", opts, parser, os );

		const priorReadId = currentReadId;
		try {
			if( !opts.id ) {
				console.trace( "NO ID PASSED TO LOAD", opts );
			}
			//console.log( "LOADING : ", opts.id );
			for( var remote of os.remotes ) {
                        	_debug_get && console.log( "So for some remote... use ws.get" );
				remote.remote.get( opts ).then( (obj)=>{
					const p = new Promise( (res,rej)=>{
						ackObjects.push( {res:res,rej:rej,cb:resultCallback,id:opts.id } );
					});
					currentReadId = opts.id;
                                        //_debug_get_ll &&
                                        console.log( "RELOAD:", obj );
					parser.write( obj );
					return p
			        } ).catch( err=>{
					console.log( "remote 'get' Error:", err );
					resultCallback( null );
				} );
			}
			//os.read( currentReadId = opts.id
			//	, parser
			function resultCallback(obj,id){
				// with a new parser, only a partial decode before revive again...
				//console.log( "Read resulted with an object:", obj );
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
                                    	//console.log( "Is a container...", currentReadId, obj.id, id );
					//console.log( "GOTzz:", obj, obj.id, obj.data );
					if( !("id" in obj ))
						Object.defineProperty( obj, "id", { value:id } );
                                        else if( "undefined" === typeof obj.id )
					     throw new Error( "Object has no identity" );//Object.defineProperty( obj, "id", { value:id } );

                                        //console.log( "Setting stored object:", obj.data , id, obj.id );
					os.stored.set( obj.data, obj.id );
					os.cachedContainer.set( obj.id, obj );
					//currentReadId = priorReadId;
					for( let res in extraResolutions ) res.res(obj.data);
                                        _debug_get_ll && console.log( "calling resolve.1" );
					res(obj.data);
				} else {
                                    	//console.log( "Isn't a container..." );
					//console.log( "Result object", obj );
					currentReadId = priorReadId;
					for( let res in extraResolutions ) res.res(obj);
                                        _debug_get_ll && console.log( "calling resolve." );
					res(obj)
				}
			}
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
			//console.log( "folder volume:", this_.folder.volume );
			//debugger;
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
			let first = null;
			if( "string" === typeof o ) {
				//console.log( "direct write of string data:", o );
		                for( var remote of this.folder.volume.remotes ) {
        		            let newsend = remote.remote.put( o, this.id, res, rej );
                                    if( !first ) first = newsend;
                	            res = null;
                        	    rej = null;
	        	        }

				//this.folder.volume.writeRaw( this.id, o );
				//return Promise.resolve( this.id );
                                return first;

			} else if( o instanceof ArrayBuffer
                                 || Object.getPrototypeOf(o).constructor.name === "ArrayBuffer" ) {
				//console.log( "Write raw buffer" );
		                for( var remote of this.folder.volume.remotes ) {
        		            let newsend = remote.remote.put(  o, this.id,res, rej );
                                    if( !first ) first = newsend;
                	            res = null;
                        	    rej = null;
	        	        }
				//this.folder.volume.writeRaw( this.id, o );
				//return Promise.resolve( this.id );
                                return first;

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
ObjectStorage.prototype.getRoot = async function() {
	if( this.root ) return this.root;
	if( loading ) {
		//console.log( "Still loading?" );
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
	
		//console.log( "Getting root directory..", this_.ws, this_.ws.get );
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


function handleMessage( msg_ ) {
	const msg = JSOX.parse( msg_ );
        const storage = this;
        if( msg.op === "connected" ) {
        }else if( msg.op === "got" ) {

        }else if( msg.op === "pull" ) {
        }
}



ObjectStorage.connect = function(remote) {
	return new ObjectStorage( remote )
}

