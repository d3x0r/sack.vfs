/**
 * @file Events — a small event class.
 *
 * `on()` is three operations depending on its second argument: subscribe with a
 * function, dispatch with anything else, probe with nothing.  Each is declared
 * as a separate `@overload` so callers get the right return type instead of
 * `any` — an EventHandle, the array of handler results, or a boolean.
 *
 * Differs from Events2 in one way that matters: array spreading is opt-out
 * PER HANDLER here, through `EventHandle.enableArrayArgs`.  Events2 dropped
 * that in favour of wrapping the payload twice at the dispatch site.
 */

/**
 * A handler.
 *
 * Dispatch passes the payload followed by the array of results returned by the
 * handlers that already ran, so a handler can see what came before it.  An
 * array payload is spread unless this handle has `enableArrayArgs = false`.
 *
 * @callback EventHandler
 * @param {...any} args
 * @returns {any}
 */

/**
 * Per-class state, keyed by constructor.
 *
 * @typedef {object} EventTypeState
 * @property {Object<string,EventHandle[]>} static_events
 * @property {boolean} usePriority
 * @property {boolean} log
 */

/** @type {WeakMap<Function,EventTypeState>} */
const eventTypes = new WeakMap();

/**
 * @param {Function} type
 * @returns {EventTypeState}
 */
function getType( type ) {
	const t = eventTypes.get( type );
	if( t ) return t;
	const newType = { static_events:{}, usePriority:true, log: false };
	eventTypes.set( type, newType );
	return newType;
}

/**
 * A subscription, returned by `on()`.
 *
 * It carries the list it lives in, so `off(handle)` can splice it out directly
 * rather than searching by name and function.
 */
export class EventHandle {
	/** @type {EventHandler|null} */
	cb = null;
	/** The handler list this registration belongs to. @type {EventHandle[]|null} */
	list = null;
	/** Higher runs earlier; equal priorities keep registration order. @type {number} */
	priority = 0;

	#enableArrayArgs = true;

	/**
	 * Whether an array payload is spread across this handler's parameters.
	 *
	 * Set false to receive the array whole as a single argument — the per-handler
	 * escape hatch that Events2 does not have.
	 *
	 * @param {boolean} value
	 */
	set enableArrayArgs(value) {
		this.#enableArrayArgs = value;
	}
	/** @returns {boolean} */
	get enableArrayArgs() {
		return this.#enableArrayArgs;
	}

	/** @param {EventHandler} cb */
	constructor(cb) {
		this.cb = cb;
	}
}

export class Events {
	/** @type {Object<string,EventHandle[]>} */
	#events = {};

	/**
	 * Enable debug logging for this class. Once enabled it cannot be disabled.
	 * @param {boolean} value
	 */
	static set log(value) {
		if( !this ) throw new Error( "(log)Events should have the class type as this..."+(this)+(Events) );
		const type = getType( this );
		if( value ) // only enable; once enable no disable
			type.log = value;
	}

	/**
	 * Whether the argument after a handler is treated as a priority.
	 * @param {boolean} value
	 */
	static set usePriority(value) {
		if( !this ) throw new Error( "(SetPriority)Events should have the class type as this..."+(this)+(Events) );
		const type = getType( this );
		type.usePriority = value;
	}

	/**
	 * Subscribe to a static event.
	 * @overload
	 * @param {string} evt
	 * @param {EventHandler} d
	 * @param {number} [extra] priority; higher runs earlier, default 0
	 * @returns {EventHandle}
	 */
	/**
	 * Dispatch a static event.
	 * @overload
	 * @param {string} evt
	 * @param {any[]|Record<string,any>|string|number|boolean} d
	 * @returns {any[]|undefined} every handler's return value, in call order
	 */
	/**
	 * Ask whether anything is listening.
	 * @overload
	 * @param {string} evt
	 * @returns {boolean|undefined}
	 */
	/**
	 * @param {string} evt
	 * @param {any} [d]
	 * @param {number} [extra]
	 * @returns {any}
	 */
	static on( evt, d, extra ) {
		if( !this ) throw new Error( "(on)Events should have the class type as this..."+(this)+(Events) );
		const type = getType( this );
		return on( type.static_events, type.usePriority, type.log, evt, d, extra );
	}

	/**
	 * Remove a static subscription by its handle.
	 * @overload
	 * @param {EventHandle} evt
	 * @returns {void}
	 */
	/**
	 * Remove a static subscription by name and function.
	 * @overload
	 * @param {string} evt
	 * @param {EventHandler} d
	 * @returns {void}
	 */
	/**
	 * @param {string|EventHandle} evt
	 * @param {EventHandler} [d]
	 * @returns {void}
	 */
	static off( evt, d ) {
		if( evt instanceof EventHandle ) {
			const l = evt.list;
			for( let i = 0; i < l.length; i++ ) { if( l[i] === evt ) { l.splice(i,1); return; } }
			// throw handle already removed?
		}
		if( !this ) throw new Error( "(off)Events should have the class type as this..."+(this)+(Events) );
		const type = getType( this );
		return off( type.static_events, type.log, evt, d );
	}

	/**
	 * Whether the argument after a handler is treated as a priority.
	 * @param {boolean} value
	 */
	set usePriority(value) {
		const ThisEventClass = Object.getPrototypeOf( this ).constructor;
		const type = getType( ThisEventClass );
		type.usePriority = value;
	}

	constructor() {
		const ThisEventClass = Object.getPrototypeOf( this ).constructor;
		getType( ThisEventClass );
	}

	/**
	 * Subscribe.
	 * @overload
	 * @param {string} evt
	 * @param {EventHandler} d
	 * @param {number} [extra] priority; higher runs earlier, default 0
	 * @returns {EventHandle}
	 */
	/**
	 * Dispatch. An array payload is spread across each handler's parameters
	 * unless that handle has `enableArrayArgs = false`.
	 * @overload
	 * @param {string} evt
	 * @param {any[]|Record<string,any>|string|number|boolean} d
	 * @returns {any[]|undefined} every handler's return value, in call order
	 */
	/**
	 * Ask whether anything is listening.
	 * @overload
	 * @param {string} evt
	 * @returns {boolean|undefined}
	 */
	/**
	 * @param {string} evt
	 * @param {any} [d]
	 * @param {number} [extra]
	 * @returns {any}
	 */
	on( evt, d, extra ) {
		const type = getType( Object.getPrototypeOf( this ).constructor );
		return on( this.#events, type.usePriority, type.log, evt, d, extra );
	}

	/**
	 * Remove a subscription by its handle — the cheap path, since the handle
	 * already knows which list it is in.
	 * @overload
	 * @param {EventHandle} evt
	 * @returns {void}
	 */
	/**
	 * Remove a subscription by name and function.
	 * @overload
	 * @param {string} evt
	 * @param {EventHandler} d
	 * @returns {void}
	 */
	/**
	 * @param {string|EventHandle} evt
	 * @param {EventHandler} [d]
	 * @returns {void}
	 */
	off( evt, d ) {
		if( evt instanceof EventHandle ) {
			const l = evt.list;
			for( let i = 0; i < l.length; i++ ) { if( l[i] === evt ) { l.splice(i,1); return; } }
			// throw handle already removed?
		}
		const type = getType( Object.getPrototypeOf( this ).constructor );
		return off( this.#events, type.log, evt, d );
	}
}

/**
 * @param {Object<string,EventHandle[]>} events
 * @param {boolean} usePriority
 * @param {boolean} log
 * @param {string} evt
 * @param {any} [d]
 * @param {number} [extra]
 * @returns {EventHandle|any[]|boolean|undefined}
 */
function on( events, usePriority, log, evt, d, extra ) {
	if( "function" === typeof d ) {
		if( log ) console.log( "Defining event handler for:", evt );
		const callback = new EventHandle(d);
		const findPriority = ( usePriority && "number" === typeof extra )
			?(callback.priority = extra)
			:0;

		if( evt in events ) {
			const eventList = callback.list = events[evt];
			const index = eventList.findIndex( (cb)=>( cb.priority < findPriority ) )
			eventList.splice( index < 0 ? eventList.length : index, 0, callback );
		}
		else events[evt] = callback.list = [callback];
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

/**
 * @param {Object<string,EventHandle[]>} events
 * @param {boolean} log
 * @param {string|EventHandle} evt
 * @param {EventHandler} [d]
 * @returns {void}
 */
function off( events, log, evt, d ) {
	if( "function" === typeof d ) {
		const a = events[evt];
		for( let i = 0; i < a.length; i++ ) {
			if( a[i].cb === d ) {
				if( log ) console.log( "Removed event handler for:", evt );
				a.splice( i, 1 );
				break;
			}
		}
	} else {
		console.log( "Unsupported parameter type to 'off'" );
	}

}
