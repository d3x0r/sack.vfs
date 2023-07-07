
class Event {
	cb = null;
	priority = 0;

	#enableArrayArgs = true;
	set enableCallbackArrays(value) {
		this.#enableArrayArgs = value;
	}
	get enableCallbackArrays() {
		return this.#enableArrayArgs;
	}

	constructor(cb) {
		this.cb = cb;
	}
}

export class Events {
	#events = {};
	static #log = false;
	static set log(value) {
		if( value ) // only enable; once enable no disable
			this.#log = value;
	}

	#usePriority = true;
	static set enablePriority(value) {
		this.#usePriority = value;
	}


	on( evt, d, extra ) {
		if( "function" === typeof d ) {
			const callback = new Event(d);
			if( Events.#log ) console.log( "Defining event handler for:", evt );
			if( evt in this.#events ) {
				const events = this.#events[evt];
				let findPriority = 0;
				if( this.#usePriority && "number" === typeof extra ) {
					findPriority = callback.priority = extra;
				}
				const index = events.findIndex( (cb)=>( cb.priority < findPriority ) )
				if( index > 0 ) events.splice( index-1, 0, callback ); // insert one before the one found
				else if( index === 0 ) events.unshift( callback ); // first in list
				else events.push( callback ); // lowest priority.
			}
			else this.#events[evt] = [callback];
			return callback;
		}else if( "undefined" !== typeof d ) {
			if( Events.#log ) console.log( "Emiting event handler for:", evt );
			if( evt in this.#events ) return this.#events[evt].reduce( (arr,cb,idx)=>{
	 			const isArray = cb.#enableArrayArgs && d instanceof Array;
				if( isArray ) arr.push( (cb.cb)( ...d, arr ) );
				else arr.push( (cb.cb)(d, arr) );
				return arr;
			}, [] );
		}else {
			if( Events.#log ) console.log( "requesting event handler:", evt );
			if( evt in this.#events ) return true;
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
