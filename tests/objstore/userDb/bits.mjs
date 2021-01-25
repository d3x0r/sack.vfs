

function bitReader( bits ) {
	if( !(this instanceof bitReader) ) return new bitReader( bits );

	if( "number" === typeof bits ) {
		bits = new Uint8Array( ( (bits+7)/8)|0 );
	}

	function MASK_TOP_MASK(length) {
		return (0xFF) >>> (8 - (length))
	};

	function MY_MASK_MASK(n, length) {
		return (MASK_TOP_MASK(length) << ((n) & 0x7)) & 0xFF;
	}
	function MY_GET_MASK(v, n, mask_size) {
		return (v[(n) >> 3] & MY_MASK_MASK(n, mask_size)) >>> (((n)) & 0x7)
	}

	var bitReader_ = {
		entropy: bits,  // the variable that is 'read'
		available: 0,
		used: 0,
		//total_bits : 0, // this is more of a benchmark feature
		storage_ : null,

		reset() {
			//this.entropy = 
			this.available = 0;
			this.used = 0;
			this.total_bits = 0;
		},


		hook( storage ) {
			storage.addEncoders( [ { tag:"btr", p:null, f:this.encode.bind(this) } ] );
			storage.addDecoders( [ { tag:"btr", p:null, f:this.decode } ] );
			this.storage_ = storage;
		},
		encode( stringifier ){
			return `btr{e:${stringifier.stringify(this.entropy)},a:${this.available},u:${this.used}}`;
		},
		decode(a,b){
			console.log( "GOT:", a, b );
			const reader = bitReader( a.e );
			reader.available = a.a;
			reader.used = a.u;
			return reader;
		},
		get(N) {
			const bit = this.entropy[N>>3] & ( 1 << (N&7)) ;
			if( bit ) return true; 
			return false;
		},
		set(N) {
			this.entropy[N>>3] |= ( 1 << (N&7)) ;
		},
		getBit(N) {
			const bit = this.entropy[N>>3] & ( 1 << (N&7)) ;
			if( bit ) return true; 
			return false;
		},
		setBit(N) {
			this.entropy[N>>3] |= ( 1 << (N&7)) ;
		},
		clearBit(N) {
			this.entropy[N>>3] &= ~( 1 << (N&7)) ;
		},
		getBits(start, count, signed) {
			if( !count ) { count = 32; signed = true } 
			if (count > 32)
				throw "Use getBuffer for more than 32 bits.";
			var tmp = this.getBuffer(start, count);
			if( signed ) {
				var val = new Uint32Array(tmp)[0];
				if(  val & ( 1 << (count-1) ) ) {
					var negone = ~0;
					negone <<= (count-1);
					//console.log( "set arr_u = ", arr_u[0].toString(16 ) , negone.toString(16 ) );
					val |= negone;
				}
				return val;
			}
			else {
				var arr = new Uint32Array(tmp);
				return arr[0];
			}
		},
		getBuffer(start,bits) {
			let _bits = bits;
			let resultIndex = 0;
			let resultBits = 0;
			let resultBuffer = new ArrayBuffer(4 * ((bits + 31) >> 5));
			let result = new Uint8Array(resultBuffer);
			//this.total_bits += bits;  // benchmark feature
			this.used = start;
			{
				let tmp;
				let partial_tmp;
				let partial_bits = 0;
				let get_bits;

				do {
					if (bits > 8)
						get_bits = 8;
					else
						get_bits = bits;
					// if there were 1-7 bits of data in partial, then can only get 8-partial max.
					if( (8-partial_bits) < get_bits )
						get_bits = (8-partial_bits);
					// if get_bits == 8
					//    but bits_used is 1-7, then it would have to pull 2 bytes to get the 8 required
					//    so truncate get_bits to 1-7 bits
					let chunk = ( 8 - ( this.used & 7) );
					if( chunk < get_bits )
						get_bits = chunk;
					// if resultBits is 1-7 offset, then would have to store up to 2 bytes of value
					//    so have to truncate to just the up to 1 bytes that will fit.
					chunk = ( 8 - ( resultBits & 7) );
					if( chunk < get_bits )
						get_bits = chunk;

					//console.log( "Get bits:", get_bits, " after", this.used, "into", resultBits );
					// only greater... if equal just grab the bits.
					if (get_bits > (this.available - this.used)) {
						if (this.available - this.used) {
							partial_bits = this.available - this.used;
							// partial can never be greater than 8; request is never greater than 8
							//if (partial_bits > 8)
							//	partial_bits = 8;
							partial_tmp = MY_GET_MASK(this.entropy, this.used, partial_bits);
						}
						needBits();
						bits -= partial_bits;
					}
					else {
						tmp = MY_GET_MASK(this.entropy, this.used, get_bits);
						this.used += get_bits;
						if (partial_bits) {
							tmp = partial_tmp | (tmp << partial_bits);
							partial_bits = 0;
						}
						
						result[resultIndex] |= tmp << (resultBits&7);
						resultBits += get_bits;
						// because of input limits, total result bits can only be 8 or less.
						if( resultBits == 8 ) {
							resultIndex++;
							resultBits = 0;
						}
						bits -= get_bits;
					}
				} while (bits);
				//console.log( "output is ", result[0].toString(16), result[1].toString(16), result[2].toString(16), result[3].toString(16) )
				return resultBuffer;
			}
		}
	}

	Object.assign( this, bitReader_ );
}


function encode( a ){
	return this.encode(a);
}
function decode(field,val){
	if( field === "a" ) this.available = val;
	if( field === "u" ) this.used = val;
	return this;
}


bitReader.hook = function(storage ) {
	storage.addEncoders( [ { tag:"btr", p:bitReader, f:encode } ] );
	storage.addDecoders( [ { tag:"btr", p:bitReader, f:decode } ] );
}

//module.exports = exports = bitReader;
export {bitReader};
