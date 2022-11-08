
export class Events {
	#events = {};
	on( evt, d ) {
		if( "function" === typeof d ) {
			if( evt in this.#events ) this.#events[evt].push(d);
			else this.#events[evt] = [d];
		}else {
			if( evt in this.#events ) return this.#events[evt].map( cb=>cb(d) );
		}
	}
	off( evt, d ) {
		if( "function" === typeof d ) {
			const a = this.#events[evt];
			for( let i = 0; i < a.length; i++ ) {
				if( a[i] === d ) {
					a.splice( i, 1 );
					break;
				}
			}
		} else {
			console.log( "Unsupported parameter type to 'off'" );
		}
	}
}
