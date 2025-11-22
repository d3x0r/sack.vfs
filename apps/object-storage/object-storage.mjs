

import {sack} from "sack.vfs";
//console.log( "sack?", sack );
import {ObjectStore} from "./object-storage-data.mjs"
import {StoredObject} from "./object-storage-object.mjs"
import {DbStorage} from "./object-storage-data-sql.mjs";
import {ObjectStorageContainer} from "./object-storage-container.mjs"
import {FileEntry,FileDirectory} from "./object-storage-file-system.mjs"
import {l} from "./object-storage-local.mjs"

//export default function( sack ) {

const _debug = false;
const _debug_dangling = false;
const _debug_output = _debug || false;
const _debug_object_convert = _debug || false;
const _debug_ll = false; // remote receive message logging.
const _debug_map = false;
const _debug_replace = false;

if( "undefined" === typeof log )
{
	const os = import( "os" ).then( (module)=>{
		sack.SaltyRNG.setSigningThreads( module.cpus().length );
	} );
} 

// save original object.
const _objectStorage = sack.ObjectStorage_native;
const nativeVol = sack.Volume();

const path = import.meta?.url?.replace(/file\:\/\/\//, '' ).split("/") || ".";
//console.log( "path?", path );
//const path = import.meta.url.replace(/file\:\/\/\//, '' ).split("/");
const root = path;// __dirname;//(path.splice(path.length-1,1),path.join('/')+"/");
const remoteExtensionsSrc = nativeVol.read( root+"/object-storage-remote.js" );
//const remoteExtensionsSrc = nativeVol.read( __dirname+"/object-storage-remote.js" );
const remoteExtensions = remoteExtensionsSrc?remoteExtensionsSrc.toString():"// No COde Found";
const jsonRemoteExtensions = JSON.stringify( remoteExtensions );

function shortId( s ) {
	return sack.Id( s );
}


let allDangling = new Map();
let dangling = [];
let objectRefs = 0;
let currentContainer = null;
let preloadStorage = null; // storage thrown from main thread

const updatedPrototypes = new WeakMap();

// ---- This block of variables is for parsing JSOX object and foriegn reference revivals during map
const fullObjectMaps = new WeakMap();

let rootContainer = false;
let rootObjectSet = false;
let rootObjectContainer = null;
//let mapFullObject = false;
// ------ end foreign reference tracking ^^^ 


let currentReadId ;

let loading = null; // mulitple requests for getRoot stack here until the first is resolved....


// default options used  when map is not passed options.
const defaultMapOpts = {depth:0};


// manufacture a JS interface to _objectStorage.
class ObjectStorage {
	storage = null; // old method
	#stores = []; // all stores associated with this object-verse
	cached = new Map();
	cachedContainer = new Map();
	stored = new WeakMap(); // objects which have alredy been through put()/get()
	storedIn = new WeakMap(); // objects which have alredy been through put()/get()
	decoding = [];
	pending = [];
	stringifier = sack.JSOX.stringifier();
	parser = null; // this will be filled when .get() is called.
	currentParser = null;
	root = null; // this gets filled when the root file system is enabled.
	encoders = [];
	decoders = [];


	constructor  (...args) {
		const arg0 = args[0];
		if( arg0 instanceof sack.Sqlite ) {
			_debug && console.log( "Using a db storage interface..." );
			preloadStorage = new DbStorage( arg0 );
		} else if( arg0 instanceof ObjectStore ) {
			preloadStorage = new arg0( );
		} else
			_debug && console.log( "Using a file interface?" );
		const newStorage = preloadStorage || _objectStorage(...args);
		if( newStorage === preloadStorage ) preloadStorage = null;
		const thisStorage = this;
		this.storage = newStorage;

		setupStringifier( this, this.stringifier );

		this.handleMessage = handleMessage;
		this.StoredObject = StoredObject;
	
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
				newStorage.storage.writeRaw( msg.rid, msg.data);
				ws.send( { op:"PUT", id:msg.id, r:msg.rid } );
				return true;
			}
                        }catch(err) { console.log( "Failed?", err ) }
			return false;
		}
		//console.log( "constructed storage..." );
	}

	setupStringifier(s) { return setupStringifier( this, s ); }

	static Thread = {
		post: _objectStorage.Thread.post,
		accept(cb) {
			 _objectStorage.Thread.accept((a,b)=>{
				// posted from core thread ( worker can't access disk itself? )
				preloadStorage = b;
				cb(a, new ObjectStorage(b) );
			 });
		}
	}

	static getRemoteFragment() {
		return remoteExtensions;
	}

	async map( o, opts ) {
	//
	// options = {
	//      depth : maximum distance to load...
	// }
	//
		//this.parser.
		if( !opts ) opts = defaultMapOpts;
		//if( opts && opts.depth === 0 ) return;
		//console.log( "External API invoked map...");
		const rootId = this.stored.get( o );
		if( !rootId ) { console.trace( "Object was not stored, cannot map", o ); return Promise.resolve( o ); }
		if( rootId[0] === '~' ) {
			//console.log( "this is mapping a directly referenced field; which means we have to resolve this at least.", rootId );
			const loading = this.storedIn.get( o );
			//console.log( "Get result:", loading );
			return loadPending( this, loading );
		}
		//console.trace( "cached container on root ID check", rootId );

		return this.cachedContainer.get( rootId ).map( opts );
	}

	async getRoot() {
		const this_ = this;
		if( this_.root ) return this_.root;
		//console.log( "Getting root...!!!!", this_.root );

		if( loading ) {
			return new Promise( (res,rej)=>{
				console.log( "Still loading, wait to getroot..." );
				loading.push(  {res:res, rej:rej} );
			} );
		}

		//console.log( "Getting root..." );
		this_.addEncoders( [ { tag: "d", p:FileDirectory, f:null}, { tag: "f", p:FileEntry, f:null} ] );
		this_.addDecoders( [ {tag: "d", p:FileDirectory, f:FileDirectory.fromJSOX }, {tag: "f", p:FileEntry } ] )
		const result = new FileDirectory( this, "?" );

		//console.trace( "Who gets this promise for getting root?");
		// while in async dispatch, additional requests for this may happen... setup an array to catch them to dispatch later.
		loading = [];

		return this_.get( { id:result.id } )
				.then( (dir)=>{
					//console.log( "Get got:", result.id, dir );
					if( !dir ) {
						return result.store(true)
							.then( function(id){
								//console.log( "1) Assigning ID to directory", id );
								Object.defineProperty( result, "id", { value:id } );
								finishLoad( result );
								for( let l of loading ) l.res( result );
								loading = null;
								return result;
							} )
					}
	        
					// foreach file, set file.folder
					else{
						Object.defineProperty( dir, "id", { value:result.id } );
						finishLoad(dir);
					}
					function finishLoad(dir) {
						dir.volume = this_;
						this_.root = dir;
					}
					for( let l of loading ) l.res( result );
					loading = null;
					return dir;
				} );
	}

	async dir() {
		return (await this.getRoot).files;
	}

	// define a class... to be handled by stringification
	defineClasss (a,b) {
		this.stringifier.defineClass(a,b);
	}

	scan( from ) {
		var fromTime = ( from.getTime() * 256 );
		//this.loadSince( fromTime );
	}

	getContainerRef( obj, options ) {
		var containerId = this.stored.get( obj );
		if( containerId ) {
			const container = this.storedIn.get( obj );
			//console.log( "ID:", containerId, container );
			return container;
			
		}
		return null;
	}

	getContainer( obj, options ) {
		var container = this.stored.get( obj );
		var storage;
		if( container ) {
			container = this.cachedContainer.get( container );
			return container;
		}
		//console.trace( "Getting a new container...", options );
		container = new ObjectStorageContainer(this,obj,options);
		//console.log( " *** SETTING ID HERE", container.id);
		//if( container.id === undefined ) throw new Error( "Error along path of setting container ID");
		if( container.id ) {
			this.stored.set( obj, container.id );
			this.cached.set( container.id, container.data );
			console.log( "Container storage gets updated here ---------------------------");
			container.storage = this;
			this.cachedContainer.set( container.id, container );
		}else {
			// should fix 'undefined' as a index in stored
			//console.log( "Storage still has to come from real storage operation... not setting cached ID");
			console.log( "Container storage NOT gets updated here ---------------------------");
		}
	}

	delete( opts ) {
		return this.storage.delete(opts);
	}

	remove( opts ) {
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
	addEncoders(encoderList) {
		const this_ = this;
		encoderList.forEach( f=>{
			this_.encoders.push( f );
			//console.log( "encoders:", encoderList );
			this_.stringifier.registerToJSOX( f.tag, f.p, f.f ) ;
		});

	}
	addDecoders(encoderList) {
		const this_ = this;
		encoderList.forEach( f=>{
			function redirect(f) {
				return function redirect(field,val) {
					// call loaded anyway.  loaded can't change the object.
					if( !field && ( this instanceof StoredObject ) ) this.loaded( this_, currentReadId );
					// call registered handler; handler can change what this object is.
					if(f) {
						if( !field ) { 
							return f.call( this, field, this_ ); 
						} else return f.call(this,field,val );
					}else {
						if( !field ) return this;
						else return val;
					}
				}
			}
			f.f = redirect(f.f);
			this_.decoders.push( f );
			if( this.parser )
				this.parser.fromJSOX( f.tag, f.p, f.f );
		});

	}

	getCurrentParseRef() {
		if( this.currentParser ){
			return this.currentParser.currentRef();
		}
		return null;
	}

	stringify( obj ) {
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
		//console.log( "stringify default:", output, container ) ;
		return output;
		
		
	}

	// this hides the original 'put'
	put( obj, opts ) {
		if( "string" === typeof obj ) {
			console.trace( "BLAH?", obj );
		}
		const this_ = this;
		if( currentContainer && currentContainer.data === this ) {
			saveObject( null, null );
			_debug && console.log( "Returning same id; queued save to background...")
				return Promise.resolve( currentContainer.id );
		}else {

		}
		return new Promise( saveObject );

		function saveObject(res,rej) {

			if( "string" === typeof obj && opts.id ) {
				console.log( "SAVING A STRING OBJECT" );
				// this isn't cached on this side.
				// we don't know the real object.
				this_.storage.writeRaw( opts.id, obj );
				return res?res( opts.id ):null;
			}
			var container = this_.stored.get( obj );
			if( !container && opts && opts.id ) {
				const oldObj = this_.cached.get( opts.id );
				if( oldObj )
					container = this_.stored.get( oldObj );
			}
			let storage;
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
	//		console.log( "Why doesn't this straoge have gettime?", storage );
				container.time = this_.getTimes( container.id );
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
					throw new Error( "Container has no ID or is null" );					
				}
				_debug_output && console.trace( "Write(root set):", opts, storage );
				if( this_.versioned ) {				
					const id = opts.id.split( '.' );
					//console.log( "Storing with an ID already?", obj, opts );
					const newVersion = this_.storage.writeRaw( {id:id[0]}, storage );
					opts.id = id[0] + '.' + newVersion;
				} else 
					this_.storage.writeRaw( opts, storage );
				res && res( opts.id );
			} else if( !opts || !opts.id ) {
				// need a new ID for an object (not string)
				_debug && console.log( "New bare object, create a container...", opts );
				if( !opts ) opts = { id : shortId() }
				else opts.id = shortId();
				//console.log( "Storage ID is picked here:", opts );
				if( "object" === typeof obj ) {
					container = new ObjectStorageContainer(this_,obj,opts);
					if( !container.id || "string" !== typeof container.id ) throw new Error( "Failed to get a conatiner ID for some reason" );
					//console.log( "New container looks like... ", container.id, container );
					
					//console.log( "saving stored container.id", typeof obj, obj, container.id );
						
					//this_.stored.delete( obj );
					//console.log( " *** SETTING ID HERE", container.id);
					this_.stored.set( obj, container.id );
					if( container.id === undefined ) throw new Error( "Error along path of setting container ID");
					this_.cached.set( container.id, container.data );
					//console.log( "3CACHE IS SET HERE ------------------------- ", obj)
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
					opts.id = container.id
					container.encoding = true;
					//console.log( "Setting root container mode(2)....");
					rootContainer = true;
					storage = stringifier.stringify( container );
					container.encoding = false;
				} else {
					container = new ObjectStorageContainer(this_,obj,opts);
					storage = obj;
				}
				if( !container.id || container.id === "null" ) {
					console.trace( "Container has no ID or is nUll", container );
				}
				_debug_output && console.log( "Output container to storage... ", container, container.storage, container.id );
				try {
					this_.storage.writeRaw( opts, storage );
					if( container.id === undefined ) throw new Error( "Error along path of setting container ID");
					this_.cached.set( container.id, container.data );
					//console.log( "2CACHE IS SET HERE ------------------------- ", obj)
					this_.cachedContainer.set( container.id, container );
				}catch(err) { console.log( "WRITE RAW?", this_ )}
				//console.log( "OUTPUT:", storage );
				res && res(  container.id );
			}
		}

		/*
		sack.ObjectStorage.prototype.update( objId, obj ) {

			var container = new ObjectStorageContainer(this,sack.JSOX.stringify(obj),sign);
			this.stored.set( obj, container.id );
			this.cached.set( container.id, container );
			return container.id;
		}

		*/
	}  // end of put() method

	get( opts ) {
		//this.parser.
		let resolve;
		//let reject;
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
			const priorLoad = this.cachedContainer.get( opts.id );
			//console.log( "already found?", priorLoad );
			if( priorLoad ) return Promise.resolve( priorLoad.data );
		}
		//console.trace( "TEST going to get:", opts )
		const priorDecode = this.decoding.find( d=>d.opts.id === opts.id );
		if( priorDecode ){
			//console.trace( "already decoding...(use same promised result) things?", priorDecode );
			return priorDecode.p;
		}

		if( !this.parser ){
			this.parser = sack.JSOX.begin();
			//console.log( "ADDING ~os");
			this.parser.fromJSOX( "~os", ObjectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
			this.parser.fromJSOX( "~or", objectStorageContainerRef, reviveContainerRef ); // I don't know ahead of time which this is.
			for( let f of this.decoders )
				this.parser.fromJSOX( f.tag, f.p, f.f );
		}
		const parser = (opts.noParse)?null:this.parser;

		if( opts.extraDecoders ) {
			parser = sack.JSOX.begin(  );
			//console.log( "Adding ~os handler");
			parser.fromJSOX( "~os", ObjectStorageContainer, reviveContainer ); // I don't know ahead of time which this is.
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
			resolve = res;  //reject = rej;

			const priorReadId = currentReadId;
			try {
				l.currentStorage = os; // this part is synchronous... but leaves JS heap from os.read(), does callbacks using parser, but results with a parsed object
								// all synchronously.
				const parts = opts.id.split('.');
				//console.log( "id?", opts.id, parts );
				os.storage.read( currentReadId = parts[0], Number( parts[1] )
					, parser, opts.noParse?res:(obj)=>resultDecode(opts, priorReadId, obj,res) );
			}catch(err) {
				console.log( "ERROR:", err );
				currentReadId = priorReadId;
				rej(err);
			}
		} );
		//console.log( "PUSHING THING TO LOAD LATER (decoding)?", opts );

		this.decoding.push( { p:p, opts:opts } );
		p.then( (r)=>{
			//console.log( "Removing decoding object with then...", r);
			const doneDecoding = os.decoding.findIndex( d=>d.opts === opts );
			//console.log( "decode should already be resolved:", doneDecoding, os.decoding, opts, r );
			if( doneDecoding >=0 ) os.decoding.splice( doneDecoding, 1 );
			else console.log( "Failed to find decoding object?" );
			return r;
		} );
		return p;

		function resultDecode( opts,priorReadId, obj,resolve) {
			// with a new parser, only a partial decode before revive again...
			try {
				//console.log( "Read resulted with an object:", opts.id, obj );
				if( obj instanceof ObjectStorageContainer )  {
					//console.log( "SHOULD HAVE FIXED IT ALREDY!");
					obj.storage = os;
				}
			
			let deleteId = -1;
			l.currentStorage = null;
			//const extraResolutions = [];
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
				var found = os.pending.findIndex( pending=>{ 
					console.log( "what is in pending?", key, opts.id, pending ); 
					return pending.id === opts.id } );
				if( found >= 0 ) {
					os.pending[found].ref.o[os.pending[found].ref.f] = obj.data;
					os.pending.splice( found, 1 );
				}
			} while( found >= 0 );

			//console.log( "Object is what?", obj );
			if( obj && ( obj instanceof ObjectStorageContainer ) ){
				//console.log( "GOTzz:", obj, obj.id, obj.data );
				if( !("id" in obj ))
					Object.defineProperty( obj, "id", { value:currentReadId } );

				//console.log( "Why does object have no data?", obj.data, obj );
				os.stored.set( obj.data, obj.id );
				os.cachedContainer.set( obj.id, obj );
				currentReadId = priorReadId;
				//for( let res in extraResolutions ) res.res(obj.data);
				resolve(obj.data);
			} else {
				currentReadId = priorReadId;
				//for( let res in extraResolutions ) res.res(obj);
				//console.log( "RESOLVE WITH OBJECT NEED DATA?", ( obj instanceof ObjectStorageContainer ) );
				resolve(obj)
			}
		}catch( err) {
			console.log( "Less fatal error:", err );
		}
		}

		function objectStorageContainerRef( s ) {
			//_debug_dangling && 
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
				//console.log( "Container ref:", s, this.d.p );

			} catch(err) { console.log( "Init ObjectContainerRef failed:", err)}
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
						if( val === this ){
							console.trace( "This is so bad...");
							process.exit(0);
						}
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

	} // end of get() method
	
	flush() {
		// this doesn't really cache anything to flush?

	}
        
	sign(a,b,c,d ) {
		return sack.SaltyRNG.sign( a, b, c );

	}
	verify(a,b,c,d ) {
		return sack.SaltyRNG.verify( a, b, c, d );
	}
} // end of ObjectStorage class






export	function loadPending(store, load, opts) {
		//console.log( "doing real get, which results with ANOTHER promise", load, store );
		return store.get( {id:load.d.id}).then( (obj)=>{
			//console.log( "Storage requested:", load.d.id );
			if( load.d.res ) {
				load.d.res(obj); // result with real value.
			} else {
				load.d.p.then( o2=>{if( obj!==o2) throw new Error( "resolved and loaded object mismatch");return o2})
				return load.d.p;
			}

			//const existobj = thisStorage.stored.get( load.d.p );
			//const exist = thisStorage.stored.get( obj );
			//console.log( "This is the promise object?", load.d.id, exist, existobj );
			
			const objc = store.cachedContainer.get( load.d.id );
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



	function objectToJSOX( stringifier, storage, obj ){
		_debug_object_convert && console.log( "Convert Any Object to JSOX:", typeof obj, obj );
		if( rootContainer ) {
			//console.log( "know the next thing is is a new container, get the data..." );
			rootContainer = false;
			//rootObject = obj.data;
			rootObjectContainer = obj;
			fullObjectMaps.set( rootObjectContainer, obj.id );
		}else {
			if( rootObjectSet ) {
				//console.log( "know the next thing is is a new container, get the data..." );
				//const newId = shortId();
				console.trace( "setting root container ID to ?", rootObjectSet, rootContainer, rootObjectContainer
						, "\nbecomes", obj );
				//rootObject = obj;
				rootObjectContainer = obj;
				// I don't know this has to be in this map... it's a 
				// un-named part of an object to encode, it might have a path 
				// reference, but that's not what this is for.
				//fullObjectMaps.set( rootObjectContainer, newId );

			}
			//else
			//	console.log( "Sub Object of something...", this !== rootObjectContainer );
		}
		if( false /*mapFullObject*/ ) 
			if( obj !== rootObjectContainer ) {
				//const curMap = fullObjectMaps.get( rootObjectContainer );
				const ref = stringifier.getReference( obj );
				console.log( "(full map)saving internal reference:", ref, obj );
				fullObjectMaps.set( obj, ref );
			}

		//const isRoot = (rootObjectContainer === obj);
		if( _debug_object_convert )
			if(obj instanceof Promise ) {
				console.log( "This is still a pending object reference(?)", obj );
			}
		//  see if we alread stored this... (or are currently storing this.) (back references container)
		//console.log( "this?", obj, rootObjectContainer );

		var exist = !rootObjectSet && rootObjectContainer && storage.stored.get( obj );
		//console.log( "Exist after promise?", exist );
		_debug_object_convert && console.log( "THIS GOT CALLED?", obj, Object.getPrototypeOf( obj ), exist );
		if( exist ) {
			//console.trace( "Is it this sort of exist?", exist );
			if( exist[0] == '~' ) {
				// this is a dangling promise
				return exist;
			} 
			var objContainer = storage.cachedContainer.get( exist );
			_debug_object_convert && console.log( "Object stored independantly... RECOVERED:", exist, obj&&obj.encoding );
			_debug_object_convert && console.log( "existing reference...", exist );
			//console.log( "Cached container:", objContainer.encoding, obj )
			if( objContainer.encoding ) 
				return obj;
			else {
				return '~or"'+exist+'"';
			}
		}
		else {
			if( rootObjectContainer !== obj ) {
				exist = fullObjectMaps.get( obj );
				if( exist ) {
					// sometimes it's a storage container, other times it's a raw object...
					console.trace( "Deep remote reference.", rootObjectSet, obj, exist );
					//return '~or"'+exist+'"';
				} else {
					_debug_object_convert && console.log( "not a stored object, encode itself." );
				}
			}
		}
		return obj;
	}

	function storageObjectToJSOX( stringifier, storage, obj ){
		//  see if we alread stored this... (or are currently storing this.) (back references container)
		_debug_object_convert && console.trace( "THIS GOT CALLED?", obj, Object.getPrototypeOf( obj ) );
		var exist = storage.stored.get( obj );
		if( exist ) {
			console.log( "Think this already is existing?", exist );
			if( exist[0] == '~' ) {
				// is a pending promise... 
				return exist;
			}
			var obj = storage.cachedContainer.get( exist );
			_debug_object_convert && console.log( "Object stored independantly... RECOVERED:", exist, obj.encoding );
			if( obj.encoding ) 
				return obj;
			else {
				//console.log( "Why is this an ~or and not ~os?");
				return '~or"'+exist+'"';
			}
		} else {
			if( this instanceof ObjectStorageContainer ) {
				//console.log( "THIS SHOULD ALREADY BE IN THE STORAGE!", this, this.stored.get( this.data ) );
				// this is probably the final result when encoding this object.
				//this.stored.set( this.data, this.id ); // maybe update the ID though.
				if( obj.nonce )
					return {data:obj.data,nonce:obj.nonce,sign:obj.sign}; // this object will be stringified, and have our type prefix prepended.
				else
					return {data:obj.data}; // this object will be stringified, and have our type prefix prepended.
			}
		}
		//console.log( "not a container...");
		return obj;
	}



function setupStringifier( newStorage, stringifier ) {
	stringifier.setDefaultObjectToJSOX( function tojsox(stringifier){
		return objectToJSOX( stringifier, newStorage, this );
	} );
	stringifier.registerToJSOX( "~os", ObjectStorageContainer, function tojsox(stringifier) {
		return storageObjectToJSOX( stringifier, newStorage, this )
	} );
	stringifier.store = newStorage;
	for( let f of newStorage.encoders )
		stringifier.registerToJSOX( f.tag, f.p, f.f ) ;
}


export {ObjectStorage};
export default ObjectStorage;
