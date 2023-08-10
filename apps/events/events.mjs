
const eventTypes = new WeakMap();

function getType( type ) {
	const t = eventTypes.get( type );
	if( t ) return t;
	const newType = { static_events:{}, usePriority:true, log: false };
	eventTypes.set( type, newType );
	return newType;
}

class Event {
	cb = null;
	priority = 0;

	#enableArrayArgs = true;
	set enableArrayArgs(value) {
		this.#enableArrayArgs = value;
	}
	get enableArrayArgs() {
		return this.#enableArrayArgs;
	}

	constructor(cb) {
		this.cb = cb;
	}
}

export class Events {
	#events = {};
	static set log(value) {
		const type = getType( this );
		if( value ) // only enable; once enable no disable
			type.log = value;
	}

	static set usePriority(value) {
		const type = getType( this );
		type.usePriority = value;
	}

	static on( evt, d, extra ) {
		const type = getType( this );
		return on( type.static_events, type.usePriority, type.log, evt, d, extra );		
	}
	static off( evt, d ) {
		const type = getType( this );
		return off( type.static_events, type.log, evt, d );
	}

	set usePriority(value) {
		const ThisEventClass = Object.getPrototypeOf( this ).constructor;
		const type = getType( ThisEventClass );
		type.usePriority = value;
	}

	constructor() {
		const ThisEventClass = Object.getPrototypeOf( this ).constructor;
		getType( ThisEventClass );
	}

	on( evt, d, extra ) {
		const type = getType( Object.getPrototypeOf( this ).constructor );
		return on( this.#events, type.usePriority, type.log, evt, d, extra );
	}
	off( evt, d ) {
		const type = getType( Object.getPrototypeOf( this ).constructor );
		return off( this.#events, type.log, evt, d );
	}
}

function on( events, usePriority, log, evt, d, extra ) {
	if( "function" === typeof d ) {
		if( log ) console.log( "Defining event handler for:", evt );
		const callback = new Event(d);
		if( evt in events ) {
			const eventList = events[evt];
			let findPriority = 0;
			if( usePriority && "number" === typeof extra ) {
				findPriority = callback.priority = extra;
			}
			const index = eventList.findIndex( (cb)=>( cb.priority < findPriority ) )
			if( index > 0 ) eventList.splice( index-1, 0, callback ); // insert one before the one found
			else if( index === 0 ) eventList.unshift( callback ); // first in list
			else eventList.push( callback ); // lowest priority.
		}
		else events[evt] = [callback];
		return callback;
	}else if( "undefined" !== typeof d ) {
		if( log ) console.log( "Emiting event handler for:", evt );
		if( evt in events ) return events[evt].reduce( (arr,cb,idx)=>{
 			const isArray = cb.enableArrayArgs && d instanceof Array;
			if( isArray ) arr.push( (cb.cb)( ...d, arr ) );
			else arr.push( (cb.cb)(d, arr) );
			return arr;
		}, [] );
	}else {
		if( evt in events ) return true;
	}
}

function off( events, log, evt, d ) {
	if( "function" === typeof d ) {
		const a = events[evt];
		for( let i = 0; i < a.length; i++ ) {
			if( a[i] === d ) {
				if( log ) console.log( "Removed event handler for:", evt );
				a.splice( i, 1 );
				break;
			}
		}
	} else {
		console.log( "Unsupported parameter type to 'off'" );
	}

}