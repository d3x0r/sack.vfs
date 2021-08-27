import {sack} from "sack.vfs"
const StoredObject = sack.ObjectStorage.StoredObject;
//import {StoredObject} from "../commonDb.mjs"

const SlabArray_StorageTag = "?sa"
const SlabArrayElement_StorageTag = "?ae"
const elementSize = 200;
let storage = null;

const storages = [];
function regStorage( storage ){
	if( !storages.find( s=>s===storage ) ) {
		storage.addEncoders( [ { tag:SlabArrayElement_StorageTag, p:SlabArrayElement, f: null }
			,  { tag:SlabArray_StorageTag, p:SlabArray, f: null }
			] );
		storage.addDecoders( [ { tag:SlabArrayElement_StorageTag, p:SlabArrayElement, f: null }
			,  { tag:SlabArray_StorageTag, p:SlabArray, f: null }
			] );
		storages.push( storage );
	}
}

class SlabArrayElement extends StoredObject {
	elements = [];
	depth = 0;
	parent = null;
	constructor(storage) {
		super(storage );
	}

	push(object, intoCb) {
		if( this.depth > 0 ) {
			// parent nodes - only contains other arrays - which may be delay loaded as promised content.
			const lastBlock = this.elements && this.elements[this.elements.length-1];
			if( lastBlock instanceof Promise ) {
				lastBlock.then( ()=>{
					//console.log( "Array element resolved this is now:", this );
					return this.push( object, intoCb );
				})
				this.storage.map(lastBlock);
				return  object; // it will get pushed...
			}

			// the last element has a free spot?
			//console.log( "this.elements is?", this.depth, this.elements.length, this.elements[this.elements.length-1].elements.length );
			if( !this.elements.length
				|| ( lastBlock.elements.length >= elementSize ) ) {
				if( this.elements.length ){
					if( lastBlock.push( object, intoCb ) ){
						return object;	
					}
					// else, that block is actually full, and still truly need a new block
				}
				const newSlab = new SlabArrayElement(this.storage);
				newSlab.depth = this.depth-1;
				newSlab.parent = this;
				this.elements.push( newSlab );
				this.store(); // parent 
				return newSlab.push( object, intoCb );  // push into real new slab.
			} else {
				return lastBlock.push( object, intoCb );
			}
		} else {
			if( this.elements.length >= elementSize  ) {
				if( !this.parent ) {
					const parent = new SlabArrayElement(this.storage);
					parent.depth = this.depth+1;
					parent.elements.push( this );
					this.parent = parent;
					//console.log( "New block data...", parent, "which should be the new root" );
					// new parent generation root.
					intoCb( parent );
					return parent.push( object, intoCb );
				}else {
					// this could infinitely recurse if there's bad counts.
					return null;
					//return this.parent.push( object, intoCb );
				}
			} else {
				this.elements.push( object );
				this.store();
				return object;
			}
		}
	}

	async forEach(cb) {
		let block = 0;
		let entry = 0;
		for( ; block < this.elements.length; block++ ){
			let elements = this.elements[block];		
			if( elements instanceof Promise ) {
				elements = await this.storage.map( elements );
			}
			if( this.depth > 0 ) {
				for( entry = 0; entry < this.elements.length; entry++ ){
					await this.elements[entry].forEach( cb );
				}
			}else {
				for( entry = 0; entry < this.elements.length; entry++ ){
					await cb( arr[entry], block * elementSize + entry );
				}
			}
		}
	}
}

class SlabArray extends StoredObject {
	elements = null;

	constructor() {
		super();
	}
	async get(index) {
		const block = Math.floor( index / elementSize );
		const entry = index % elementSize;
		if( block < this.elements.length ) {
			let elements = this.elements[block];

			if( elements instanceof Promise ) {
				elements = await this.storage.map( elements );
			}
			return elements.elements[entry];
		} 
		return undefined;
	}
	push(object) {
		const this_ = this;
		if( this.elements instanceof Promise ) {
			return this.storage.map( this.elements ).then( (elements)=>{
				return elements.push( object, newBlock );
			});
		} else {
			//console.log( "Do I not have elements at all?", this.elements );
			if( !this.elements ) {
				( this.elements = new SlabArrayElement( this.storage ) ).push( object, newBlock );
				this.store();
			}

			return this.elements.push( object, newBlock );
		}

		function newBlock(newRoot){
			if( !newRoot ){ console.trace( "Failure");
				throw new Error( "Please do not set NO root?");
			}
			//console.log( "new root?", newRoot );
			this_.elements = newRoot;
			this_.store();
		}
	}
	store( opts ) {
		return super.store( opts );
	}
	async forEach(cb) {
		if( !this.elements ) return;
		await this.elements.forEach( cb )
	}

	hook( storage ) {
		super.hook(storage);
		regStorage( storage );
	}

}

SlabArray.hook = function( storage ) {
	regStorage(storage);	
}


export {SlabArray} 
