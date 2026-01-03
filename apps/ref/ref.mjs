
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
