
class Events {
	#events = {};
	static #log = false;
	static set log(value) {
		if( value )
			this.#log = value;
	}
	on( evt, d ) {
		if( "function" === typeof d ) {
			if( Events.#log ) console.log( "Defining event handler for:", evt );
			if( evt in this.#events ) this.#events[evt].push(d);
			else this.#events[evt] = [d];
		}else {
			if( Events.#log ) console.log( "Emiting event handler for:", evt );
			if( evt in this.#events ) return this.#events[evt].map( cb=>cb(d) );
		}
	}
	off( evt, d ) {
		if( "function" === typeof d ) {
			const a = this.#events[evt];
			for( let i = 0; i < a.length; i++ ) {
				if( a[i] === d ) {
					if( Events.#log ) console.log( "Removed event handler for:", evt );
					a.splice( i, 1 );
					break;
				}
			}
		} else {
			console.log( "Unsupported parameter type to 'off'" );
		}
	}
}

var exports = exports || {};
exports.Events = Events;