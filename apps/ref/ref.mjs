// class that is a reference
//   constructor parameters
//     o - the object containing the field referenced
//     f - the name of the field in the object (or number if o is an array)
//     v - initial value to set the refernce to
//     s - setter callback, when the value is updated, this callback is called
//         with the value being set to the field.
//
//  It is possible to construct a reference that only contains the setter callback
//  If the object is not passed, a default containing the field name is created
//
//  Usage:
//    const local = { data : 3 };
//    const dataRef = new ref( local, data );
//
//   dataRef['*'] = 3;   // using setter
//   if( dataRef['*'] > 5 ) {   // using getter
//      /* do something */
//   }
//
//   const codeDataRef = new ref( null, null, null, (val)=>{
//         local.data = val;
//      } );


export class ref { 	
	#o = null;
	#f = null;
	#s = null;
	constructor( o, f, v, s ) { 
		this.#o = o || {[f]:undefined}; 
		this.#f = f; 
		if( v !== undefined ) this.#o[this.#f] = v;
		this.#s = s;  // set callback for temporary references (null obj)
	} 
	get '*'(){ return this.#o[this.#f]; } 
	set '*'(val){ this.#o[this.#f] = val; if( this.#s ) this.#s(val); }
}

