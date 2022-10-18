
class Events {
	#events = {};
	on( evt, d ) {
		if( "function" === typeof d ) {
			if( evt in this.#events ) this.#events[evt].push(d);
			else this.#events[evt] = [d];
		}else {
			if( evt in this.#events ) for( let cb of this.#events[evt] ) { const r = cb(d); if(r) return r; }
		}
	}
	off( evt, d ) {
	}
}

module.Events = Events;