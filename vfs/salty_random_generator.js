"use strict";

var CryptoJS = require( 'crypto-js' );

exports.SaltyRNG = function( f ) {
  var RNG = {
     getSalt : f,
     compute : CryptoJS.SHA3,
     saltbuf : [],
     entropy : 0,
     available : 0,
     used : 0,
     reset() {
       this.entropy = this.compute( "test" );
       //console.log( "data : " + this.entropy );
       this.available = 0;//this.entropy.words.length * 32;
       this.used = 0;
     },
     getBits( count ) {
       if( count > 32 )
          throw "Use getBuffer for more than 32 bits.";
        var tmp = this.getBuffer( count );
        return tmp[0];
     },
     needBits( ) {
       this.saltbuf.length = 0;
       if( typeof( this.getSalt) === 'function')
         this.getSalt( this.saltbuf );
   console.log( this.saltbuf )
       this.entropy = this.compute( this.entropy + this.saltbuf );
       //console.log( "data : " + this.entropy );
       this.available = this.entropy.words.length * 32;
       this.used = 0;
     },
     getBuffer ( bits ) {
       function MASK_TOP_MASK(length) {
          //console.log( "mask is " + ((0xFFFFFFFF) >>> (32-(length))).toString(16) );
          return (0xFFFFFFFF) >>> (32-(length)) };

       function MY_MASK_MASK(n, length) { return MASK_TOP_MASK(length) << ((n)&0x1f) }
       function MY_GET_MASK(v,n,mask_size)  {
         //console.log( "big start is " + n + " size is " + mask_size )
               return (v[(n)>>5] & MY_MASK_MASK(n,mask_size) ) >>> (((n))&0x1f)
       }
      let resultIndex = 0;
      let resultBuffer = new ArrayBuffer( 4 * ( ( bits + 31 ) >> 5 ) );
      let result = new Uint32Array( resultBuffer );
      {
       	let tmp;
       	let partial_tmp;
       	let partial_bits = 0;
       	let get_bits;

       	do
       	{
       		if( bits > 32 )
       			get_bits = 32;
       		else
       			get_bits = bits;

       		// only greater... if equal just grab the bits.
       		if( get_bits > ( this.available - this.used ) )
       		{
       			if( this.available - this.used )
       			{
       				partial_bits = this.available - this.used;
       				if( partial_bits > 32 )
       					partial_bits = 32;
     					partial_tmp = MY_GET_MASK( this.entropy.words, this.used, partial_bits );
       			}
       			this.needBits();
       			bits -= partial_bits;
       		}
       		else
       		{
            //console.log( "get is " + get_bits )
     				tmp = MY_GET_MASK( this.entropy.words, this.used, get_bits );
       			this.used += get_bits;
       			if( partial_bits )
       			{
              //console.log( "partial " + partial_bits )
       				tmp = partial_tmp | ( tmp << partial_bits );
       				partial_bits = 0;
       			}
       			result[resultIndex++] = tmp;
       			bits -= get_bits;
       		}
       	} while( bits );
        return resultBuffer;
      }
    }
  }
  RNG.reset();
  return RNG;
}
