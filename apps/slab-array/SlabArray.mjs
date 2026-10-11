import {StoredObject} from "../object-storage/object-storage-object.mjs"

const SlabArray_StorageTag = "?sa";
const SlabArrayElement_StorageTag = "?ae";
export const SLAB_PAGE_SIZE = 200;

const storages = [];
function regStorage(storage) {
	if( storages.includes(storage) ) return;
	storage.addEncoders( [
		{ tag:SlabArrayElement_StorageTag, p:SlabArrayElement, f:null },
		{ tag:SlabArray_StorageTag, p:SlabArray, f:null },
	] );
	storage.addDecoders( [
		{ tag:SlabArrayElement_StorageTag, p:SlabArrayElement, f:null },
		{ tag:SlabArray_StorageTag, p:SlabArray, f:null },
	] );
	storages.push( storage );
}

class SlabArrayElement extends StoredObject {
	elements = [];
	depth = 0;
	parent = null;

	constructor(storage) {
		super(storage);
	}

	get capacity() {
		return SLAB_PAGE_SIZE ** ( this.depth + 1 );
	}

	async child(index) {
		let child = this.elements[index];
		if( child instanceof Promise ) {
			child = await this.storage.map( child );
			this.elements[index] = child;
		}
		return child;
	}

	async push(object) {
		if( this.depth === 0 ) {
			if( this.elements.length >= SLAB_PAGE_SIZE ) return false;
			this.elements.push( object );
			await this.store();
			return true;
		}

		let child = this.elements.length
			? await this.child( this.elements.length - 1 )
			: null;
		if( !child ) {
			child = new SlabArrayElement( this.storage );
			child.depth = this.depth - 1;
			child.parent = this;
			this.elements.push( child );
		}

		if( !await child.push( object ) ) {
			if( this.elements.length >= SLAB_PAGE_SIZE ) return false;
			child = new SlabArrayElement( this.storage );
			child.depth = this.depth - 1;
			child.parent = this;
			this.elements.push( child );
			if( !await child.push( object ) )
				throw new Error( "Failed to append to a new SlabArray block." );
		}

		await this.store();
		return true;
	}

	async get(index) {
		if( this.depth === 0 ) {
			let value = this.elements[index];
			if( value instanceof Promise ) {
				value = await this.storage.map(value);
				this.elements[index] = value;
			}
			return value;
		}
		const childCapacity = SLAB_PAGE_SIZE ** this.depth;
		const childIndex = Math.floor( index / childCapacity );
		if( childIndex >= this.elements.length ) return undefined;
		const child = await this.child( childIndex );
		return child.get( index % childCapacity );
	}

	async forEach(cb, offset = 0) {
		if( this.depth === 0 ) {
			for( let index = 0; index < this.elements.length; index++ )
				await cb( await this.get(index), offset + index );
			return;
		}
		const childCapacity = SLAB_PAGE_SIZE ** this.depth;
		for( let index = 0; index < this.elements.length; index++ ) {
			const child = await this.child( index );
			await child.forEach( cb, offset + index * childCapacity );
		}
	}
}

class SlabArray extends StoredObject {
	elements = null;
	count = 0;

	constructor(storage) {
		super(storage);
	}

	get length() {
		return this.count;
	}

	async get(index) {
		index = Number(index);
		if( !Number.isInteger(index) ) return undefined;
		if( index < 0 ) index = this.count + index;
		if( index < 0 || index >= this.count || !this.elements ) return undefined;
		if( this.elements instanceof Promise )
			this.elements = await this.storage.map( this.elements );
		return this.elements.get( index );
	}

	async push(object) {
		if( this.elements instanceof Promise )
			this.elements = await this.storage.map( this.elements );
		if( !this.elements )
			this.elements = new SlabArrayElement( this.storage );

		if( this.count >= this.elements.capacity ) {
			const oldRoot = this.elements;
			const newRoot = new SlabArrayElement( this.storage );
			newRoot.depth = oldRoot.depth + 1;
			newRoot.elements.push( oldRoot );
			oldRoot.parent = newRoot;
			this.elements = newRoot;
		}

		if( !await this.elements.push( object ) )
			throw new Error( "SlabArray root did not have capacity after expansion." );
		this.count++;
		await this.store();
		return object;
	}

	store(opts) {
		return super.store(opts);
	}

	async forEach(cb) {
		if( !this.elements ) return;
		if( this.elements instanceof Promise )
			this.elements = await this.storage.map( this.elements );
		await this.elements.forEach( cb );
	}

	async slice(offset = 0, limit = 50, options = {}) {
		offset = Math.max( 0, Number(offset) || 0 );
		limit = Math.max( 0, Number(limit) || 0 );
		const result = [];
		if( options.reverse ) {
			for( let index = this.count - 1 - offset;
				index >= 0 && result.length < limit; index-- )
				result.push( await this.get(index) );
		} else {
			const end = Math.min( this.count, offset + limit );
			for( let index = offset; index < end; index++ )
				result.push( await this.get(index) );
		}
		return result;
	}

	hook(storage) {
		super.hook(storage);
		regStorage(storage);
	}
}

SlabArray.hook = function(storage) {
	regStorage(storage);
};

export {SlabArray};
