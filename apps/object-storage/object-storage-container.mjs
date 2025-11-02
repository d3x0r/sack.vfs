


// associates object data with storage data for later put(obj) to re-use the same informations.
export class ObjectStorageContainer {
	#storage = null;
	#stored_data = null;
	data = "pending reference"; // reviving we get null opts.

	constructor(storage,o,opts) {
		//if( !this instanceof ObjectStorageContainer ) return new ObjectStorageContainer(thisStorage,o,opts);
		// created from JSOX; there aare no parameters passed on many usages...
		//console.trace( "This should always get storage...", storage )
		if( storage ) {
			this.#storage = storage;
			//_debug_object_convert && console.log( "Something creating a new container..", o, opts );
			if( "string" === typeof o ){
				if( !o ) throw new Error( "Blank string lookup" );
				// still, if the proir didn't resolve, need to resolve this..
				let existing = storage.cachedContainer.get( o );
				if( existing ) {
					o = existing.data;
					if( existing.resolve )
						return;
				}
			}
			if( o ) {
				if( o === this ){
					console.trace( "This is so bad(2)...");
					process.exit(0);
				}
				this.data = o; // reviving we get null opts.
			} 
			if( opts && opts.sign ) {
				if( !this.nonce ) {
					//console.log( "SIGNING FAILS?" );
					let s;
					const signature = storage.sign( s = this.#storage.stringifier.stringify(o), opts.sign.pad1, opts.sign.pad2 );
					this.nonce = signature.key;
					//console.log( "Signing returned:", signature );
					this.sign = Object.assign( {e:signature.extent,c:signature.classifier},opts.sign );
					//console.log( "MAKING A NONCE:", s );
					//var v = sack.SaltyRNG.verify( this.#storage.stringifier.stringify(o), this.nonce, opts.sign.pad1, opts.sign.pad2 );
					Object.defineProperty( this, "id", { value:signature.id } );
					//console.trace( "This makes it go boom?", opts, this.nonce, o, v.key );
				} else {
				//	var v = sack.SaltyRNG.verify( thisStorage.stringifier.stringify(o), this.nonce, opts.sign.pad1, opts.sign.pad2 );
				//	Object.defineProperty( this, "id", { value:v.key } );
				}
			} else {
				if( opts && opts.id ) {
					Object.defineProperty( this, "id", { value:opts.id } );
				}		
			}
		}
		// pass this through a global for jsox fixup at end.        
		// currentContainer = this;
		Object.defineProperty( this, "encoding", { writable:true, value:false } );
	}

	get stored() {
		return this.#storage.stored;
	}
	set storage( val ) {
		this.#storage = val;
	}
	get storage( ) {
		return this.#storage;
	}

	getStore () {
		return this.#storage;
	}
	
	async map( opts ) {

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
									waiting.push( loadPending( this.storage, load, {depth:opts.depth,paths:part<path.length?[path.slice(part)]:undefined} ) );
									obj = null;
									break;
								}
							}
							
							//	obj will probably also be resolved here, because it's not in pending...
							if( obj ) {
									const ref = this.#storage.storedIn.get( obj );
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
							waiting.push( loadPending( this.#storage,load,opts, res) );
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
						const isObject = this.#storage.storedIn.get( step );
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


}
