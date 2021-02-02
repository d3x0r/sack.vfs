
const _debug_lookup = false;
const _debug_root = false;
const _debug_set = false;
const _debug_reload = false;

const BloomNHash_StorageTag = "?bh"
const BloomNHash_BlockStorageTag = "?hb"

const KEY_DATA_ENTRIES_BITS = 8;
const KEY_DATA_ENTRIES = (1<<KEY_DATA_ENTRIES_BITS)-1;
const HASH_MASK_BITS = 5;
const HASH_MASK = (1<<HASH_MASK_BITS)-1;
const ROOT_ENTRY_MASK = ( 1 << (KEY_DATA_ENTRIES_BITS-1) );
const ROOT_ENTRY_INDEX = ROOT_ENTRY_MASK-1;

const FLOWER_HASH_DEBUG_MOVES = false;
const FLOWER_TICK_PERF_COUNTERS = false
const HASH_DEBUG_BLOCK_DUMP_INCLUDED = true;

// this adds a bunch of sanity checks/ASSERT sort of debugging
const FLOWER_DIAGNOSTIC_DEBUG = 1
const pft = (!FLOWER_TICK_PERF_COUNTERS)?{
	 moves_left : 0,
	 moves_right : 0,
	 lenMovedLeft: new Array(128),
	 lenMovedRight : new Array(128)
}:null;

import util from "util";
import {bitReader} from "./bits.mjs";
//import {bitReader} from "./bits.mjs";
//export {BloomNHash}
let nextStorage = null;
let blockStorage = null;

function BloomNHash( ) {
	if( !(this instanceof BloomNHash) ) return new BloomNHash();
	_debug_reload && console.log( "CREATE NEW BLOOM -------------------------------------------__" );
	const root = this;
	this.storage_ = null;
	this.root = null;//new hashBlock();
	this.hashBlock = hashBlock;

	function hashBlock( parent ){
		var n;
		_debug_reload && console.log( "New Hash block - should get a ROOT ------------ " );
		// hide these from being written.
		Object.defineProperty(this, "timer", { enumerable: false, writable: true, configurable: true, value: null });
		Object.defineProperty(this, "coalescedWrites", { enumerable: false, writable: true, configurable: false, value: [] });
		this.nextBlock = [];
		this.entries = [];
		this.keys = [];
		if( parent )
			this.parent = parent;
		else
			this.parent = null;

		for( n = 0; n <= HASH_MASK; n++ ) this.nextBlock.push(null);
		for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
			this.entries.push(null);
			this.keys.push(null);
		}
		this.used = bitReader( KEY_DATA_ENTRIES>>1 );
		this.getStorage = function() { return root.storage_ }
		if( "undefined" != typeof parent && root.storage_ ) {
			// only if created with a 'parent' - not when revived...
			root.storage_.put( this );  // establish as a stored object.
		}

		if( !root.root ) {
			//_debug_reload && console.log( "Root root:", root, root.root, this );
			//console.log( "--------- SETTING ROOT AND STORING" );
			root.root = this;
			root.storage_.put( root );
		}
	}
	if( blockStorage || nextStorage ) {
		if( !this.storage_ ) {
			this.storage_ = blockStorage || nextStorage;
			blockStorage  = blockStorage || nextStorage;
			nextStorage = null;
			this.storage_.addEncoders( [ { tag:BloomNHash_BlockStorageTag, p:this.hashBlock, f:null } ] );
			this.storage_.addDecoders( [ { tag:BloomNHash_BlockStorageTag, p:this.hashBlock, f:null } ] );
		}
		//nextStorage = null;
	}


	hashBlock.prototype.insertFlowerHashEntry = function(key,result ){
		return insertFlowerHashEntry( this, key, result );
	}
         
	hashBlock.prototype.lookupFlowerHashEntry = function( key, result ) {
		return lookupFlowerHashEntry( this, key, result );
	}
	hashBlock.prototype.DeleteFlowerHashEntry = function( key ) {
		return DeleteFlowerHashEntry( this, key );
	}
	hashBlock.prototype.store = function() {
		const wait = new Promise( (res,rej)=>{
			this.coalescedWrites.push( {res:res,rej:rej} );
		} )

		if( !this.timer ) {
			this.timer = setTimeout( doStore.bind(this), 10 );
		}	else {
			clearTimeout( this.timer );
			this.timer = setTimeout( doStore.bind(this), 10 );
		}
		return wait;

		function doStore() {
			this.timer = null;
			return root.storage_.put( this ).then( (obj)=>{
				for( let res of this.coalescedWrites ) {
					res.res( obj );// probably didn't care about this result.
				}
				this.coalescedWrites.length = 0;
				return obj;
			} ).catch( (obj)=>{
				for( let res of this.coalescedWrites ) {
					res.rej( obj );// probably didn't care about this result.
				}
				this.coalescedWrites.length = 0;
			} );
		}
	}

	return;


// please fix indent someday
function linearToTreeIndex( f_ ) 
{
	let b;
	let fMask;
	for( b = 0; b < KEY_DATA_ENTRIES_BITS; b++ ) {
		if( f_ < ( fMask = ( ( 1 << ( b + 1 ) ) - 1 ) ) )
			break;
	}
	const f = treeEnt( f_ - ( ( fMask ) >> 1 ), b, KEY_DATA_ENTRIES_BITS );
	return f;
}


// returns pointr to user data value
function lookupFlowerHashEntry( hash, key, result ) {
	_debug_lookup && console.log( "Actually doing lookup; otherwise how did it resolve already?", key );
	if( "number" === typeof key ) 
		key = '' + key;
	if( !key ) {
		console.log( "null key:", key );
		result.hash = null;
		return; // only look for things, not nothing.
	}
	// provide 'goto top' by 'continue'; otherwise returns.
	do {
		// look in the binary tree of keys in this block.
		let curName = ROOT_ENTRY_INDEX;//treeEnt( 0, 0, KEY_DATA_ENTRIES_BITS );
		let curMask = ROOT_ENTRY_MASK;

		while( curMask )
		{
			const entkey=hash.keys[curName];
			if(!entkey)  break; // no more entries to check.
			// sort first by key length... (really only compare same-length keys)
			let d = key.length - entkey.length;
			if( ( d == 0 ) && ( d = key.localeCompare(entkey ) ) == 0 ) {
				result.entryIndex = curName;
				result.entryMask = curMask;
				result.hash = hash;
				if( hash.entries[curName] instanceof Promise ) {
					_debug_lookup && console.log( "Resulting with promise to map the entry" );
					const result = root.storage_.map( hash, { depth:-1, paths:[ ["entries", curName] ] } ).then( (hash)=>{
						_debug_lookup && console.log( "This should be the value in the entry...", key );
						return hash.entries[curName];//lookupFlowerHashEntry( hash, key, result );
					} );
					return result;
				}
				_debug_lookup&& console.log( "result with resolved promise for entry:", hash.entries[curName] );
				return Promise.resolve( hash.entries[curName] );
			}
			curName &= ~( curMask >> 1 );
			if( d > 0 ) curName |= curMask;
			curMask >>= 1;
		}
		_debug_lookup && console.log( "failed to find in hash...", key );
		{
			const hid = key.codePointAt(0) & HASH_MASK;
			// follow converted hash blocks...
			const nextblock = hash.nextBlock[hid];
			if( nextblock ) {			
				if( hash.parent ) key = key.substr(1);
				if( nextblock instanceof Promise ) {
					_debug_lookup && console.log( "next block is a promise we need to load" );
					const result = root.storage_.map( hash, {depth:0, paths: [["nextBlock", hid]] } ).then( (hash)=>{
						return lookupFlowerHashEntry( hash.nextBlock[hid], key, result );
					} );
					return result;
				} 
				hash = nextblock;
				continue;
			}
		}
		result.hash = null;
		return Promise.resolve( null );
	}
	while( 1 );
}



// return the in-order index of the tree.
// x is 0 or 1.  level is 0-depth.  depth is the max size of the tree (so it knows what index is 1/2)
// this is in-place coordinate based...
// item 0 level 0 is 50% through the array
// item 0 level 1 is 25% and item 1 level 1 is 75% through the array...
// item 0 level (max) is first element of the array.
// items are ordered in-place in the array.
function  treeEnt(x,level,depth) { return  ( ( (x)*(1 << ( (depth)-(level))) + (1 << ( (depth) - (level) -1 ))-1 ) ) }




function updateEmptiness( hash,  entryIndex,  entryMask ) {
	while( ( entryMask < KEY_DATA_ENTRIES ) && ( ( entryIndex > KEY_DATA_ENTRIES ) || hash.used.getBit( entryIndex >> 1 ) ) ) {
		if( entryIndex < KEY_DATA_ENTRIES && ( entryIndex & 1 ) ) {
			hash.used.clearBit( entryIndex >> 1 );
			//dumpBlock( hash );
		}
		entryIndex = entryIndex & ~( entryMask << 1 ) | entryMask;
		entryMask <<= 1;
	}
}

function updateFullness( hash, entryIndex, entryMask ) {
	if( !hash.keys[entryIndex ^ (entryMask<<1)] ) 
		console.trace( "Shouldn't be updating fullness..." );
	{ // if other leaf is also used
		var broIndex;
		var pIndex = entryIndex;
		do {
			pIndex = ( pIndex & ~(entryMask<<1) ) | entryMask; // go to the parent
			if( pIndex < KEY_DATA_ENTRIES ) { // this is full automatically otherwise 
				if( !hash.used.getBit( pIndex >> 1 ) ) {
					hash.used.setBit( pIndex >> 1 ); // set node full status.
				} 
				else if( FLOWER_DIAGNOSTIC_DEBUG ){
					// this could just always be inserting int a place that IS full; and doesn't BECOME full...
					console.log( "Parent should NOT be full. (if it was, " );
					debugger;
					break; // parent is already full.
				}
			}
			else {

				var tmpIndex = pIndex;
				var tmpMask = entryMask;
				while( tmpMask && ( tmpIndex = tmpIndex & ~tmpMask ) ) { // go to left child of this out of bounds parent..
					if( tmpMask == 1 )  break;
					
					if( hash.used.getBit( tmpIndex >> 1 ) ) {
						break;
					}
					tmpMask >>= 1;
				}
				if( !tmpMask )
					break;
			}
			if( entryMask > KEY_DATA_ENTRIES / 2 ) break;
			entryMask <<= 1;
			broIndex = pIndex ^ ( entryMask << 1 );
			if( broIndex >= KEY_DATA_ENTRIES ) {
				var ok = 1;
				var tmpIndex = broIndex & ~entryMask;
				var tmpMask = entryMask>> 1;
				while( tmpMask && ( tmpIndex = tmpIndex & ~tmpMask ) ) { // go to left child of this out of bounds parent..
					if( tmpIndex < KEY_DATA_ENTRIES ) {
						if( tmpMask == 1 ) 
							ok = ( hash.keys[tmpIndex] != 0 );
						else
							ok = ( hash.used.getBit( tmpIndex >> 1 ) ) != 0;
						break;
					}
					tmpMask >>= 1;
				}
				if( ok ) {
					if( pIndex < KEY_DATA_ENTRIES )
						continue;
					else break;
				}
				else break;
			}
			else {
				if( ( hash.keys[broIndex]
					&& hash.used.getBit( broIndex >> 1 ) ) )
					continue;
			}
			// and then while the parent's peer also 
			if( entryMask >= (KEY_DATA_ENTRIES/2) )
				break;
			break;
		} while( 1);
	}
}



// Rotate Right.  (The right most node may need to bubble up...)
// the only way this can end up more than max, is if the initial input
// is larger than the size of the tree.
function upRightSideTree( hash, entryIndex, entryMask ) {
	// if it's not at least this deep... it doesn't need balance.
	if( entryMask == 1 ) {
		if( !( entryIndex & 2 ) ) {
			var parent = entryIndex;
			var parentMask = entryMask;
			var bit = 0;
			while( 1 ) {
				// decision bit from previous level is off
				if( !( entryIndex & ( entryMask << 1 ) ) ) { // came from parent's left
					// came from the left side.
					parent = entryIndex | entryMask;
					parentMask <<= 1;
					if( parent >= KEY_DATA_ENTRIES ) {
						break; // off the top of the tree
					}
				}
				else break;
				// if node is empty... 
				if( !hash.keys[parent] ) {
					moveOneEntry( hash, entryIndex, parent, 1 );
					hash.keys[entryIndex] = null;
					hash.entries[entryIndex] = null;
					entryIndex = parent;
					entryMask = parentMask;
				}
				else break;
			}
		}
	}
	else
		if( !hash.keys[entryIndex] ) return;
	var broIndex = entryIndex ^ ( entryMask << 1 );
	if( entryMask == 1 && ( ( broIndex >= KEY_DATA_ENTRIES ) || hash.keys[broIndex] ) ) { // if other leaf is also used
		updateFullness( hash, entryIndex, entryMask );
	}
}

// rotate left
// left most node might have to bubble up.
function upLeftSideTree( hash,  entryIndex,  entryMask ) {
	if( ( entryIndex & 0x3 ) == 2 ) {
		while( 1 ) {
			var parent = entryIndex;
			var parentMask = entryMask;
			if( ( parent & ( parentMask << 1 ) ) ) { // came from parent's right
				parent = parent & ~( parentMask << 1 ) | parentMask;
				parentMask <<= 1;
				//  check if value is empty, and swap
				if( !hash.keys[parent] ) {
					moveOneEntry( hash, entryIndex, parent, 1 );
					hash.keys[entryIndex] = null;
					hash.entries[entryIndex] = null;
					//console.log( "Move entry up to ...", entryIndex, entryMask.toString(2), parent, parentMask.toString(2) )
					entryIndex = parent;
					entryMask = parentMask;
					continue;
				}
			}
			break;
		}
	}
	var broIndex = entryIndex ^ ( entryMask << 1 );
	if( entryMask == 1 && ( ( broIndex >= KEY_DATA_ENTRIES ) || hash.keys[broIndex] ) ) { // if other leaf is also used
		updateFullness( hash, entryIndex, entryMask );
	}
}

function getLevel( N ) {
	var n;
	for( n = 0; n < 32; n++ ) if( !(N&(1<<n)) ) break;
	return n;	
}


function validateBlock( hash ) {
	return;
	var realUsed = 0;
	var prior = nll;
	var n;
	var m;
	for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
		for( m = n+1; m < KEY_DATA_ENTRIES; m++ ) {
			if( hash.keys[n] && hash.keys[m] ) {
				if( hash.keys[n] === hash.keys[m] ) {
					console.log( "Duplicate key in %d and %d", n, m );
					debugger;
				}
			}
		}
	}

	for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
		if( hash.keys[n] ) {
			var nLevel = 1 << getLevel( n );
			if( ((n & ~( nLevel << 1 ) | nLevel) < KEY_DATA_ENTRIES )&&  !hash.keys[n & ~( nLevel << 1 ) | nLevel] ) {
				console.log( "Parent should always be filled:%d", n );
				debugger;
			}
			if( prior ) {
				if( prior.length > hash.keys.length ) {
					debugger;
				}
				if( prior.localeCompare( hash.keys[n]  ) > 0 ) {
					console.log( "Entry Out of order:%d", n );
					debugger;
				}
			}
			prior = hash.keys[n];
			realUsed++;
		}
	}
	hash.info.used = realUsed;
}


function moveOneEntry( hash,  from,  to,  update ) {
	hash.keys[to] = hash.keys[from];
	//console.log( "moveOneEntry:Key:",hash.keys[from], "to", to );
	const e = hash.entries[to] = hash.entries[from];
	if( e instanceof Promise ) {
		const cr = hash.getStorage().getContainerRef( e );
		//console.log( "Care must be taken...", cr, to, from );
		if( cr )
			cr.d.r.f = to;
	}
	if( update )
		if( to < from ) {
			upLeftSideTree( hash, to, 1 << getLevel( to ) );
		}
		else {
			upRightSideTree( hash, to, 1 << getLevel( to ) );
		}
}

function moveTwoEntries( hash,  from,  to,  update ) {
	const to_ = to;
	if( from > to ) {
		hash.keys[to] = hash.keys[from];
		//console.log( "moveTwoEntries(1):Key:",hash.keys[from], "from", from, "to", to );
		const e = hash.entries[to++] = hash.entries[from++];
		if( e instanceof Promise ) {
			const cr = hash.getStorage().getContainerRef( e );
			//console.log( "2Care must be taken...", cr, to_ );
			if( cr )
				cr.d.r.f = to_;
		}
		hash.keys[to] = hash.keys[from];
		//console.log( "moveTwoEntries(2):Key:",hash.keys[from], "from", from, "to", to );
		const e2 = hash.entries[to] = hash.entries[from];
		if( e2 instanceof Promise ) {
			const cr = hash.getStorage().getContainerRef( e2 );
			//console.log( "3Care must be taken...", cr, to );
			if( cr )
				cr.d.r.f = to;
		}
	}
	else {
		//console.log( "moveTwoEntries(2-1):Key:",hash.keys[from], "from", from, "to", to );
		hash.keys[++to] = hash.keys[++from];
		const e = hash.entries[to--] = hash.entries[from--];
		if( e instanceof Promise ) {
			const cr = hash.getStorage().getContainerRef( e );
			//console.log( "4Care must be taken...", cr, to_ );
			if( cr )
				cr.d.r.f = to_;
		}
		//console.log( "moveTwoEntries(2-2):Key:",hash.keys[from], "from", from, "to", to );
		hash.keys[to] = hash.keys[from];
		const e2 = hash.entries[to] = hash.entries[from];
		if( e2 instanceof Promise ) {
			const cr = hash.getStorage().getContainerRef( e2 );
			//console.log( "5Care must be taken...", cr, to );
			if( cr )
				cr.d.r.f = to;
		}
	}
	if( update )
		if( to < from ) {
			upLeftSideTree( hash, to_, 1 << getLevel( to_ ) );
		}
		else {
			upRightSideTree( hash, to_+1, 1 << getLevel( to_+1 ) );
		}
}


function moveEntrySpan( hash,  from,  to, count ) {
	const to_ = to;
	const count_ = count;
	if( from > to ) {
		for( ; count > 1; count -= 2, from += 2, to += 2 ) moveTwoEntries( hash, from, to, 0 );
		for( ; count > 0; count-- ) moveOneEntry( hash, from++, to++, 0 );
	}
	else {
		for( from += count, to += count; count > 0; count-- ) moveOneEntry( hash, --from, --to, 0 );
	}
	if( to < from ) {
		upLeftSideTree( hash, to_, 1 << getLevel( to_ ) );
	}
	else {
		upRightSideTree( hash, to_+ count_ -1, 1 << getLevel( to_ + count_ - 1 ) );
	}
}

var c = 0;  // static global

function insertFlowerHashEntry( hash
	, key
	, result
) {
	let entryIndex = ROOT_ENTRY_INDEX;
	let entryMask = ROOT_ENTRY_MASK; // index of my zero.
	let edge = -1;
	let edgeMask;
	let leastEdge = KEY_DATA_ENTRIES;
	let leastMask;
	let mustLeft = 0;
	let mustLeftEnt = 0;
	let full = 0;
	if( HASH_DEBUG_BLOCK_DUMP_INCLUDED ) {
		var dump = 0;
	}
	let next;

	{
		if( HASH_DEBUG_BLOCK_DUMP_INCLUDED ) {
			c++;
		//	if( ( c > ( 294050 ) ) )
		//		dump = 1;
		}
	}

	while( 1 ) {
		let d_ = 1;
		let d = 1;
		full = hash.used.getBit( entryIndex >> 1 );
		while( 1 ) {

			const offset = hash.keys[entryIndex];
			// if entry is in use....
			if( mustLeftEnt || offset ) {
				// compare the keys.
				if( d && offset ) {
					// mustLeftEnt ? -1 : 
					d = key.length - offset.length;
					if( d == 0 ) {
						d = key.localeCompare( offset );
						if( d == 0 ) {
							// duplicate already found in tree, use existing entry.
							result.entryIndex = entryIndex;
							result.entryMask = entryMask;
							result.hash = hash;
							return Promise.resolve( result );
						}
						// not at a leaf...
					}
				}

				// if full, there's no free edge, don't bother to track...
				if( !full )
					if( edge < 0 )
						if( hash.used.getBit( entryIndex >> 1 ) ) {
							edgeMask = entryMask;
							// already full... definitly will need a peer.
							edge = entryIndex ^ ( entryMask << 1 );
							if( edge > KEY_DATA_ENTRIES ) {
								var backLevels;
								// 1) immediately this edge is off the chart; this means the right side is also full (and all the dangling children)
								for( backLevels = 0; ( edge > KEY_DATA_ENTRIES ) && backLevels < 8; backLevels++ ) {
									edge = ( edge & ~( edgeMask << 1 ) ) | ( edgeMask );
									edgeMask <<= 1;
								}

								for( backLevels = backLevels; backLevels > 0; backLevels-- ) {
									let next = edge & ~( edgeMask >> 1 );
									if( ( ( next | ( edgeMask ) ) < KEY_DATA_ENTRIES ) ) {
										if( !hash.used.getBit( ( next | ( edgeMask ) ) >> 1 ) ) {
											edge = next | ( edgeMask );
										}
										else edge = next;
									}
									else {
										if( hash.used.getBit(  ( next ) >> 1 ) ) {
											mustLeft = 1;
											edge = next | ( edgeMask );
										}
										else edge = next;
									}
									edgeMask >>= 1;
								}
							}
							if( !hash.keys[edge] ) {
								leastEdge = edge;
								leastMask = edgeMask;
							}
							// if this is FULL, and the edge is here, everything there IS FULL...
							// and we really need to go up 1 and left.
						}

				if( entryMask > 1 ) {
					// if edge is set, move the edge as we go down...
					if( edge >= 0 ) {
						let next = edge & ~( entryMask >> 1 ); // next left.
						if( mustLeft ) { if( next < KEY_DATA_ENTRIES ) mustLeft = 0; }
						else
							if( edge > entryIndex ) {
								next = edge & ~( entryMask >> 1 ); // next left.
								//console.log( "Is Used?: %d  %d   %lld ", next >> 1, edge >> 1, hash.used.getBit( next >> 1 ) ) ;
								if( ( next & 1 ) && hash.used.getBit(  next >> 1 ) ) {
									if( !mustLeft ) next |= entryMask; else mustLeft = 0;
									if( next >= KEY_DATA_ENTRIES )
										mustLeft = 1;
									if( FLOWER_DIAGNOSTIC_DEBUG ) {
										if( hash.used.getBit( next >> 1 ) ) {
											console.log( "The parent of this node should already be marked full." );
											debugger;
										}
									}
								}
								else {
									if( (!( next & 1 )) && hash.keys[next] ) {
										if( !mustLeft ) next |= entryMask; else mustLeft = 0;
										if( next >= KEY_DATA_ENTRIES )
											mustLeft = 1;
									}
								}
							}
							else {
								next = edge & ~( entryMask >> 1 ); // next left.
								//console.log( "Is Used?: %d  %d   %lld  %lld", ( next | entryMask ) >> 1, edge >> 1, hash.used.getBit( ( next | entryMask ) >> 1 ), hash.used.getBit( ( next ) >> 1 ) ) ;
								if( ( next | entryMask ) < KEY_DATA_ENTRIES ) {
									if( entryMask == 2 ) {
										if( !hash.keys[next | entryMask] )
											next |= entryMask;
									}
									else {
										if( !hash.used.getBit( ( next | entryMask ) >> 1 ) ) {
											if( entryMask != 2 || !hash.keys[next | entryMask] ) {
												if( !mustLeft ) next |= entryMask; else mustLeft = 0;
												if( next >= KEY_DATA_ENTRIES )
													mustLeft = 1;
											}
										}
									}
								} 
							}

						edge = next;
						edgeMask = entryMask>>1;
						//console.log( "Edge follow: %d %s %d  %s", edge, toBinary( edge ), entryIndex, toBinary( entryIndex ) );
					}

					if( d_ ) {
						if( d ) {
							entryIndex &= ~( entryMask >> 1 );
							if( !mustLeftEnt && d > 0 ) entryIndex |= entryMask;
							if( entryIndex >= KEY_DATA_ENTRIES ) mustLeftEnt = 1; else mustLeftEnt = 0;
						}
					}
					else {
						//break;
					}
					if(! hash.keys[entryIndex] )
						d_ = d;
					entryMask >>= 1;
					if( edge >= 0 && !hash.keys[edge] )
						if( edge > entryIndex )
							if( edge < leastEdge ) {
								leastEdge = edge;
								leastMask = entryMask;
							}
							else;
						else {
							if( edge > leastEdge || ( leastEdge == KEY_DATA_ENTRIES ) ) {
								leastEdge = edge;
								leastMask = entryMask;
							}
						}
					continue;
				}

				if( mustLeftEnt && entryIndex >= KEY_DATA_ENTRIES ) {
					entryIndex &= ~2;
					if( entryIndex < (KEY_DATA_ENTRIES-1) ) {
						if( d > 0 ) {
							if( edge < 0 ) {
								edge = entryIndex;
								edgeMask = entryMask;
							}
							entryIndex++;
							entryMask <<= 1;
						}
					}
					mustLeftEnt = 0;
				}
				// this key needs to go here.
				// the value that IS here... d < 0 is needs to move left
				// if the d > 0 , then this value needs to move to the right.

				if( !full ) {
					//------------ 
					//  This huge block Shuffles the data in the array to the left or right 1 space
					//  to allow inserting the entryIndex in it's currently set spot.
					if( edge < entryIndex ) {
						if( edge == -1 ) {
							// collided on a leaf, but leaves to the left or right are available.
							// so definatly my other leaf is free, just rotate through the parent.
							var side = entryIndex & ( entryMask << 1 );
							let next = entryIndex;
							var nextToRight = 0;
							var p = entryIndex;
							var pMask = entryMask;
							while( pMask < ( KEY_DATA_ENTRIES / 2 ) ) {
								if( !hash.used.getBit( p >> 1 ) )break;
								if( p >= KEY_DATA_ENTRIES ) {
									side = 1; // force from-right
									break;
								}
								p = ( p | pMask ); p &= ~( pMask = ( pMask << 1 ) );
							}

							var nextMask;
							if( hash.keys[entryIndex] ) {
								while( ( next >= 0 ) && hash.keys[next - 1] ) next--;
								if( next > 0 ) {
									nextToRight = 0;
									nextMask = 1 << getLevel( next );
								}
								else {
									next = entryIndex;
									while( ( next < KEY_DATA_ENTRIES ) && hash.keys[next + 1] ) next++;
									if( next == KEY_DATA_ENTRIES ) {
										console.log( "The tree is full to the left, full to the right, why isn't the root already full??" );
										debugger;
									}
									else {
										nextToRight = 1;
										nextMask = 1 << getLevel( next );
									}
								}

								if( side ) {
									// I'm the greater of my parent.
									if( d > 0 ) {
										//console.log( "01 Move %d to %d for %d", next - 0, next - 1, ( 1 + ( entryIndex - next ) ) );
										if( next < entryIndex ) {
											moveEntrySpan( hash, next, next - 1, 1+(entryIndex-next) );
											next = next - 1;
											nextMask = 1 << getLevel( next );
										} else {
											moveEntrySpan( hash, entryIndex, entryIndex+1, 1 + ( next - entryIndex ) );
											entryIndex = entryIndex + 1;
											entryMask = 1 << getLevel( next );
										}
									}
									else {
										//console.log( "02 Move %d to %d for %d", next - 1, next - 2, ( 1 + ( entryIndex - next ) ) );
										if( next <= 1 ) {
											if( entryIndex > next ) {
												moveEntrySpan( hash, next, next - 1, 1+( entryIndex - next ) );
												entryIndex--;
												entryMask = 1 << getLevel( next );
												next = 0;
												nextMask = 1;
												//debugger;
											}
											else {
												moveEntrySpan( hash, next, next - 1, ( next - entryIndex ) );
												entryIndex--;
												entryMask = 1 << getLevel( next );
												next = 0;
												nextMask = 1;
												//debugger;
											}
										}
										else {
											if( nextToRight ) {
												moveEntrySpan( hash, entryIndex, entryIndex + 1, 1+( next - entryIndex ) );
												next = next++;
												nextMask = 1 << getLevel( next );
											}
											else {
												if( entryIndex - next ) {
													moveEntrySpan( hash, next, next - 1, ( entryIndex - next ) );
													entryIndex--;
													entryMask = 1 << getLevel( entryIndex );
												}
												else {
													moveOneEntry( hash, next, next - 1, 1 );
												}
												next = next - 1;
												nextMask = 1 << getLevel( next );
											}
										}
									}
								}
								else {
									// I'm the lesser of my parent...
									if( d < 0 ) {
										//console.log( "03 Move %d to %d for %d  %d", entryIndex + 1, entryIndex,  2 , ( entryIndex - next )  );
										if( entryIndex >= ( KEY_DATA_ENTRIES - 1 ) ) {
											moveOneEntry( hash, next, next - 1, 1 );
											next = next - 1;
											nextMask = 1 << getLevel( next );
											entryIndex--;
											entryMask = 1 << getLevel( entryIndex );
										} else {
											if( entryIndex < next ) {
												moveEntrySpan( hash, entryIndex, entryIndex + 1, 1+(next-entryIndex) );
												next = entryIndex + 2;
												nextMask = 1 << getLevel( next );
											} else {
												moveEntrySpan( hash, next, next - 1, ( entryIndex - next ) );
												next--;
												nextMask = 1 << getLevel( next );
												entryIndex--;
												entryMask = 1 << getLevel( entryIndex );
											}
										}
									}
									else {
										//console.log( "04 Move %d to %d for %d  %d", entryIndex+1, entryIndex+2,  1 , ( entryIndex - next ) );
										if( entryIndex >= (KEY_DATA_ENTRIES-1)) {
											moveTwoEntries( hash, next, next-1, 1 );
											next--;
											nextMask = 1 << getLevel( next );
										} else {
											if( next > entryIndex ) {
												moveOneEntry( hash, entryIndex+1, entryIndex+2, 1 );
												next = entryIndex + 2;
												nextMask = 1 << getLevel( next );
												entryIndex++;
												entryMask = 1 << getLevel( entryIndex );
											}else{
												if( nextToRight ) {
													moveOneEntry( hash, next, next + 1, 1 );
													next++;
													nextMask = 1 << getLevel( next );
												}
												else {
													moveEntrySpan( hash, next, next - 1, 1+(entryIndex - next) );
													next--;
													nextMask = 1 << getLevel( next );
												}
											}
										}
									}
								}
							}
						}
						else {
							if( leastEdge != KEY_DATA_ENTRIES ) {
								edge = leastEdge;
								edgeMask = leastMask;
							}
							//console.log( "A Move %d to %d for %d  d:%d", edge + 1, edge, entryIndex - edge, d );
							if( d < 0 )
								entryIndex--;
							moveEntrySpan( hash, edge+1, edge, entryIndex-edge );
						}
					}
					else {
						//console.log( "B Move %d   %d to %d for %d", d, entryIndex, entryIndex + 1, edge - entryIndex );
						if( edge != leastEdge && leastEdge != KEY_DATA_ENTRIES )
							if( entryIndex > edge ) {
								if( leastEdge > edge ) {
									edge = leastEdge;
									edgeMask = leastMask;
								}
							} else {
								if( leastEdge < edge ) {
									edge = leastEdge;
									edgeMask = leastMask;
								}
							}
							if( d > 0 ) {
								entryIndex++;
							}
							if( edge >= KEY_DATA_ENTRIES ) {

							}
							moveEntrySpan( hash, entryIndex, entryIndex + 1, edge - entryIndex );
					}
					//console.log( "A Store at %d %s", entryIndex, toBinary( entryIndex ) );
					// this insert does not change the fullness of this node
					hash.keys[entryIndex] = key;
					if( HASH_DEBUG_BLOCK_DUMP_INCLUDED ) {
						if( dump )
							dumpBlock( hash );
						if( dump )
							console.log( "Added at %d", entryIndex );
						validateBlock( hash );
					}
					result.entryIndex = entryIndex;
					result.entryMask = entryMask;
					result.hash = hash;
					return Promise.resolve( result );
				}
				break;
			}
			else {
				// this entry is free, save here...
				hash.keys[entryIndex] = key;
				//console.log( "B Store at %d  %s", entryIndex, toBinary( entryIndex ) );
				if( !( entryIndex & 1 ) ) { // filled a leaf.... 
					if( ( entryMask == 1 && ( entryIndex ^ 2 ) >= KEY_DATA_ENTRIES ) || hash.keys[entryIndex ^ ( entryMask << 1 )] ) { // if other leaf is also used
						console.log( "Filled a leaf, update fullnesss ( but this is the other side of the tree, and, should already be full" );
						updateFullness( hash, entryIndex, entryMask );
					}
				}
				if( HASH_DEBUG_BLOCK_DUMP_INCLUDED ) {
					if( dump )
						dumpBlock( hash );
					if( dump )
						console.log( "Added at %d", entryIndex );
					validateBlock( hash );
				}
				result.entryIndex = entryIndex;
				result.entryMask = entryMask;
				result.hash = hash;
				return Promise.resolve( result );
			}
		}
		const hid = key.codePointAt(0) & HASH_MASK;
		if( !( next = hash.nextBlock[hid] ) ) {
			if( 0 ) next = convertFlowerHashBlock( hash );
			else {
				next = ( hash.nextBlock[key.codePointAt(0) & HASH_MASK] = new hashBlock( hash ) );
				_debug_set && console.log( "Update, because we added a child hash." );
				if( root.storage_ ) {
					hash.store();
				}
			}
		}
		if( hash.parent )
			key = key.substr(1);
		if( next instanceof Promise ) {
			return root.storage_.map( hash, {depth:0, paths:[["nextBlock",hid]] } ).then( ()=>insertFlowerHashEntry( hash.nextBlock[hid], key, result ) );
		}
		hash = next;
		entryIndex = ROOT_ENTRY_INDEX;
		entryMask = ROOT_ENTRY_MASK;
	}
}

// pull one of the nodes below me into this spot.
function bootstrap( hash, entryIndex, entryMask ) {
	var c1, c2 = 0, c3;
	var cMask;
	if( entryMask > 1 ) {
		if( entryIndex >= 0 ) {
			c1 = -1;
			if( !hash.keys[c2 = entryIndex - 1] ) {
				for( cMask = entryMask >> 1, c2 = entryIndex & ( ~cMask ); cMask > 0; ) {
					if( !hash.keys[c2] ) {
						break;
					}
					c1 = c2;
					cMask >>= 1;
					c2 = c2 & ~( cMask ) | ( cMask << 1 );
				}
			}
			else c1 = c2;
		} else c1 = -1;
		if( ( c2 || c1<0 ) && entryIndex <= KEY_DATA_ENTRIES ) {
			if( !hash.keys[c3 = entryIndex + 1] ) {
				c2 = KEY_DATA_ENTRIES;
				for( cMask = entryMask>>1, c3 = entryIndex & ( ~cMask )|(cMask<<1); cMask > 0; ) {
					if( !hash.keys[c3] ) {
						break;
					}
					c2 = c3;
					cMask >>= 1;
					c3 = c3 & ( ~cMask );
				}
			}
			else c2 = c3;
		} else c2 = KEY_DATA_ENTRIES;;
		if( c1 >= 0 && ( ( c2 >= KEY_DATA_ENTRIES ) || ( entryIndex - c1 ) < ( c2 - entryIndex ) ) ) {
			moveOneEntry( hash, c1, entryIndex, 1 );
			hash.keys[c1] = null;
			cMask = 1 << getLevel( c1 );
			if( c1 & 1 )
				bootstrap( hash, c1, cMask );
			else
				updateEmptiness( hash, c1 & ~( cMask << 1 ) | ( cMask ), cMask << 1 );
		}
		else if( ( ( c2 ) < KEY_DATA_ENTRIES ) && hash.keys[c2] ) {
			moveOneEntry( hash, c2, entryIndex, 1 );
			hash.keys[c2] = null;
			cMask = 1 << getLevel( c2 );
			if( c2 & 1 )
				bootstrap( hash, c2, cMask );
			else
				updateEmptiness( hash, c2 & ~( cMask <<1) | ( cMask ), cMask<<1 );
		}else
			hash.keys[entryIndex] = 0;
	} else {
		// just a leaf node, clear emptiness and be done.
		updateEmptiness( hash, entryIndex, entryMask<<1 );
	}
}


function deleteFlowerHashEntry( hash, entryIndex, entryMask )
{
	if( entryMask > 1 ) {
		bootstrap( hash, entryIndex, entryMask );
	}
	else {
		hash.keys[entryIndex] = 0;
		updateEmptiness( hash, entryIndex &~(entryMask<<1)|entryMask, entryMask<<1 );
	}
	if( HASH_DEBUG_BLOCK_DUMP_INCLUDED) {
		//dumpBlock( hash );
		validateBlock( hash );
	}
}

function DeleteFlowerHashEntry( hash, key )
{
	const resultEx = {};
	const t = lookupFlowerHashEntry( hash.hash, key, resultEx );
	if( t ) {
		deleteFlowerHashEntry( hash.hash, resultEx.entryIndex, resultEx.entryMask );
	}
}


function convertFlowerHashBlock( hash ) {

	var new_dir_block;
	
	var counters = new Uint16Array(HASH_MASK+1);
	var maxc = 0;
	var imax = 0;
	var f_;
	var f;
	// read name block chain varo a single array

	for( f = 0; f < KEY_DATA_ENTRIES; f++ ) {
		name = hash.keys[f];
		if( name ) {
			let ch;
			let count = ( ++counters[ch = name.codePointAt(0) & HASH_MASK] );
			if( count > maxc ) {
				imax = ch;
				maxc = count;
			}
		}
	}
	//console.log( "--------------- Converting varo hash:%d", imax );
	// after finding most used first byte; get a new block, and povar
	// hash entry to that.

	maxc = 0;
	{
		const newHash = new hashBlock( hash );
		hash.nextBlock[imax] = newHash;

		newHash.parent = hash;
		var b = 0;
		for( f_ = 0; 1; f_++ ) {
			var fMask;
			fMask = 0;
			for( ; b < KEY_DATA_ENTRIES_BITS; b++ ) {
				if( f_ < ( fMask = ( ( 1 << ( b+1) ) - 1 ) ) )
					break;
			}
			if( !fMask ) 
				break;
			f = treeEnt( f_ - ( ( fMask ) >> 1 ), b, KEY_DATA_ENTRIES_BITS );
			if( f >= KEY_DATA_ENTRIES ) 
				continue;
			//console.log( "test at %d  . %d   %d  %d   %02x  ch:%02x", f_, f, b, f_ - ( ( fMask ) >> 1 ), fMask, hash.keys[f] );


			const name = hash.keys[f];
			if( name ) {
				var newlen = namebuffer[name - 1] - ( ( !hash.parent ) ? 0 : 1 );
				//console.log( "Found %d  %02x = %02x at %d(%d)", name, namebuffer[name], namebuffer[name]&HASH_MASK, f, f_ );
				if( newlen >= 0 ) {
					if( ( name.codePointAt(0) & HASH_MASK ) == imax ) {
						// these get promises created and resulted, but noone cares about them.
						// the new block is a leaf, and IS loaded.
						insertFlowerHashEntry( newHash, ( !hash.parent ) ? name : name.substr(1)  );
						ptr[0] = hash.entries[f].stored_data;
						// don't have to load records which get deleted by key value
						deleteFlowerHashEntry( hash, f, 1 << getLevel( f ) );
						maxc++;
						f_--;  // retry this record... with new value in it.
					}
				}
				else {
					console.log( "key has no data, must match in this block" );
				}
			}
		}
//		validateBlock( hash );
		return newHash;
	}
}









function dumpBlock_( hash, opt ) {
	let buf = '';
	let leader = '';
	var n;
	if( !opt ) {
		//console.log( "\x1b[H\x1b[3J" );
		//console.log( "\x1b[H" );
		while( hash.parent ) hash = hash.parent;
	}
	else {
		for( n = 0; n < opt; n++ )
			leader += '\t';
	}
	console.log( "HASH TABLE: " );

	if( 0 ) {
		// this is binary dump of number... just a pretty drawing really
		{
			let b;
			for( b = 0; b < KEY_DATA_ENTRIES_BITS; b++ ) {
				buf = ''
				for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
					buf+= (( n & ( 1 << ( KEY_DATA_ENTRIES_BITS - b - 1 ) ) ) ? '1' : '0');
				}
				console.log( "%s    :%s", leader, buf );
			}
		}

		buf = ''
		for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
			buf += '-';
		}
		
		console.log( "%s    :%s", leader, buf );
	}

	{
		buf = ''
		// output vertical number
		for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
			buf += ( n / 100 )|0;
		}
		
		console.log( "%s    :%s", leader, buf );

		buf = ''
		for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
			buf += ( ( n / 10 ) % 10 )|0 + '0';
		}
		
		console.log( "%s    :%s", leader, buf );

		buf = ''
		for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
			buf += ( ( n ) % 10 )|0;
		}
		
		console.log( "%s    :%s", leader, buf );
	}

	buf = ''
	for( n = 0; n < KEY_DATA_ENTRIES>>1; n++ ) {
		buf += ' ';
		if( hash.used.get( n ) ) buf += '1'; else buf += '0';
	}
	
	console.log( "%sFULL:%s", leader, buf );


	buf = ''
	for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
		if( hash.keys[n] ) buf += '1'; else buf += '0';
	}
	
	console.log( "%sUSED:%s", leader, buf );

	buf = ''
	for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
		if( hash.entries[n] ) buf += '1'; else buf += '0';
	}
	
	console.log( "%sENTS:%s", leader, buf );

	if(1)
	{
		// output empty/full tree in-levels
		let l;
		for( l = (KEY_DATA_ENTRIES_BITS-1); l >= 0; l-- ) {
			buf = ''
			for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
				if( ( n & ( 0x1FF >> l ) ) == ( 255 >> l ) ) {
					if( l < (KEY_DATA_ENTRIES_BITS) )
						if( hash.used.get( n >> 1 ) )
							buf += '*';
						else
							buf += '-';
					else
						if( hash.keys[n] )
							buf += '*';
						else
							buf += '-';
				}
				else
					buf += ' ';
			}
			
			console.log( "%s    :%s", leader, buf );
		}
	}

	if(0)
	{
		// output key data bytes tree in-levels
		let l;
		for( l = 0; l < 20; l++ ) {
			let d; d = 0;
			buf = ''
			for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
				if( hash.keys[n] 
					&& (l>>1)<= hash.keys[n].length  
					) {
					d = 1;
					let keyval = ( hash.keys[n].codePointAt(l>>1) );
					if( !(l & 1) )  
						buf += ( ( keyval & 0xF0 ) >> 4 ) .toString(16)
					else
						buf += ( keyval & 0xF ) .toString(16)
				} else
					buf += ' ';
			}
			
			if( d )
				console.log( "%s    :%s", leader, buf );
		}
	}

	{
		// output key data bytes tree in-levels
		let l;
		for( l = 0; l < 3; l++ ) {
			let d; d = 0;
			buf = ''
			for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
				if( hash.keys[n] 
					//&& (l)<= hash.keys[n].length  
					) {
					d = 1;
					if( l === 0 )
						buf += (( hash.keys[n].length )/100 % 10)|0
					else if( l === 1 )
						buf += (( hash.keys[n].length )/10 % 10)|0
					else if( l === 2 )
						buf += (( hash.keys[n].length ) % 10)|0
				} else
					buf += ' ';
			}
			
			if( d )
				console.log( "%s    :%s", leader, buf );
		}
		console.log( '-------------------------------------' );

		for( l = 0; l < 10; l++ ) {
			let d; d = 0;
			buf = ''
			for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
				if( hash.keys[n] 
					&& (l)< hash.keys[n].length  
					) {
					d = 1;
					buf += hash.keys[n][l]
				} else
					buf += ' ';
			}
			
			if( d )
				console.log( "%s    :%s", leader, buf );
		}
		console.log( '-------------------------------------' );

		for( l = 0; l < 3; l++ ) {
			let d; d = 0;
			buf = ''
			for( n = 0; n < KEY_DATA_ENTRIES; n++ ) {
				if( hash.keys[n] 
					//&& (l)<= Math.log(hash.entries[n] )
					) {
					d = 1;
					if( l === 0 )
						buf += (( hash.entries[n] )/100 % 10)|0
					else if( l === 1 )
						buf += (( hash.entries[n] )/10 % 10)|0
					else if( l === 2 )
						buf += (( hash.entries[n] ) % 10)|0
				} else
					buf += ' ';
			}
			
			if( d )
				console.log( "%s    :%s", leader, buf );
		}
	}

//	for( n = 0; n < ( HASH_MASK + 1 ); n++ ) {
//		if( hash.nextBlock[n] ) dumpBlock_( hash.nextBlock[n], opt+1 );
//	}

} // -------------- end of BlooMNHash

function dumpBlock( hash ) {
	dumpBlock_( hash, 1 );
}


}

let gets = [];
let getting = false;

BloomNHash.prototype.get = function( key ) {
	if( getting ) {
		_debug_lookup && console.log( "Getting is still set, delay." );
		const g = { t:this, key:key, res:null, rej:null };
		gets.push( g );
		return new Promise( (res,rej)=>((g.res=res),(g.rej=rej)) );
	}
	if( !this.root ) {
		console.log( "No objects in hash; return 'undefined'" );
		return Promise.resolve( undefined );
	}
	getting = true;
	const result = {};
	//console.trace( "Get:", this, key );
	const self = this;
	const doit = function (thing){// thisless
	//console.log( "Self Root:", this, thing, self, self.root );
		if( self.root instanceof Promise ) {
			_debug_lookup && console.log( "Root is still mapping?" );
			return self.root.then((root)=>{
				root.lookupFlowerHashEntry( key, result ).then( (obj)=>{
					_debug_lookup && console.trace( "Lookup finally resulted, and set getting = false...(and do gets)", gets, obj );
					getting = false;
					if( gets.length ) {
						const g = gets;
						gets = [];
						getone( g );//.then( ()=>obj );
					}
					return obj;
				} );    
			} );
		}
   
		function getone(gets_)
		{
			const g = gets_.shift();
			if( g ) {
				_debug_lookup && console.log( "doing an existing get(1):", g );
				// this promise result doesn't matter.. will resolve the waiting one...
				g.t.get( g.key ).then( (obj)=>{
						_debug_lookup && console.log( "doing a get something", g, obj );
						if( gets_.length ) getone( gets_ )
						else if( gets.length ) {
							const g = gets;
							gets = [];
							getone(g);//.then( ()=>obj);
						}
						g.res( obj );
						return obj;
					}).catch( g.rej );
			}
		}

		_debug_lookup && console.log( "Returning promised lookup:", key );
		return self.root.lookupFlowerHashEntry( key, result ).then( (obj)=>{
			_debug_lookup && console.log( "(2)Lookup finally resulted, and set getting = false...", obj );
			getting = false;
			if( gets.length ) {
				const g = gets;
				gets = [];
				getone(g)
				return obj;
			}
			return obj;
		} );

	} // end of doit()

	if( Object.getPrototypeOf( this.root ).constructor.name === "Promise"
        	|| this.root instanceof Promise ) {
		// reload this.root 
             //console.log( "Mapping thing...", this.hashBlock );
		return this.storage_.map( this, { depth:0 } ).then( doit );

	}
	return doit();
}

const inserts = [];
let inserting = false;
BloomNHash.prototype.set = function( key, val ) {
	if( this.root instanceof Promise ) {
		return blockStorage.map( this, {depth:0,} ).then( (realroot)=>{
			return this.set( key, val );
		} );
	}

	if( inserting ) {
		let i;
		inserts.push( i = {t:this, key:key,val:val,res:null,rej:null } );
		return new Promise( (res,rej)=>((i.res=res),(i.rej=rej)));
	}
	inserting = true;
	if( "number" === typeof key ) key = '' + key;
	const result = {};

	if( !this.root ) new this.hashBlock(null);

	_debug_root && console.log( "This.root SHOULD be set...", this.root );
	return this.root.insertFlowerHashEntry( key, result ).then( (res)=>{
		//console.log( "SET ENTRY:", result.entryIndex, val );
        	result.hash.entries[result.entryIndex] = val;
		if( this.storage_ ) {
			result.hash.store();
		}
		inserting = false;
		if( inserts.length ) {
			// begin a new insert.
			const i = inserts.shift();				
			i.t.set( i.key, i.val ).then( i.res ).catch( i.rej );
		}
		return val;
	} );
}

BloomNHash.prototype.delete = function( key ) {
	if( !this.root ) return;
	return this.root.DeleteFlowerHashEntry( this.root, key, val );
}


function encodeHash( stringifier ){
	return `{root:${stringifier.stringify(this.root)}}`;
}

BloomNHash.hook = function(storage ) {
	nextStorage = storage;
	storage.addEncoders( [ { tag:BloomNHash_StorageTag, p:BloomNHash, f:encodeHash } ] );
	storage.addDecoders( [ { tag:BloomNHash_StorageTag, p:BloomNHash, f:null } ] );
	bitReader.hook( storage );
}


BloomNHash.prototype.store = function( ) {
	//storage.addEncoders( [ { tag:".hb", p:this.hashBlock, f:null } ] );
	//storage.addDecoders( [ { tag:".hb", p:this.hashBlock, f:null } ] );
	//this.storage_ = storage;
	//bitReader.hook( storage );
	//console.log( "Putting the hash..." );
	if( this.storage_ )
		return this.storage_.put( this ); // establish this as an object
	else console.trace( "Storage is not enabled on this hash" );

}

export { BloomNHash };
