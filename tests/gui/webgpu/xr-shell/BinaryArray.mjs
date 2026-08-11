

const attrib_bytes =     [4,4,1,1,1,4,4,1,1,1]
const attrib_sizes =     [3,2,4,4,2,3,1,1,1,1]
const attrib_buftype = [Float32Array,Float32Array
	,Uint8Array,Uint8Array,Uint8Array
	,Float32Array,Float32Array,Uint8Array,Uint8Array, Uint8Array]


/* creates an exandable typed array.
   the type passed to the constructor
   should be the literal class/function constructor
   for the array type */

export class BinaryArray {
	buftype = Float32Array;
	buffer = new Float32Array( [] );
	indeces = 3;
	used = 0;
	available = 0;
	set length( v ) {
		this.used = v;
	}
	constructor( type, indeces ) {
		const index = attrib_buftype.indexOf( type );
		this.index = index;
		this.indeces = indeces;
		this.available = 100;
		this.buftype = type;
		this.buffer = new type( this.available * indeces );
	}
	expand() {
		this.available = ( this.available + 1 ) * 2;

		 //attribs.forEach( (att,index)=>{
		const newbuf =   new this.buftype( this.available * this.indeces );
		newbuf.set( this.buffer );
		this.buffer = newbuf;
		 //})
	};
	push( v ) {
		if( this.used >= this.available )
			this.expand();

		const u = this.used * this.indeces;
		if( this.indeces === 3 ) {
			this.buffer[u + 0 ] = v.x;
			this.buffer[u + 1 ] = v.y;
			this.buffer[u + 2 ] = v.z;
		}
		if( this.indeces === 4 ) {
			this.buffer[u + 0 ] = v.r;
			this.buffer[u + 1 ] = v.g;
			this.buffer[u + 2 ] = v.b;
			this.buffer[u + 3 ] = 1;
		}
		this.used++;
	}
}

