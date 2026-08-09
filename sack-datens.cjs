"use strict";

// A Date that also carries `ns` -- the sub-millisecond remainder in nanoseconds,
// 0 to 999999.  Kept here rather than in sack-jsox so that it has exactly one
// definition: the loader exposes it as sack.DateNS before anything else runs, the
// JSOX layer aliases it as sack.JSOX.DateNS, and the native side receives the
// constructor through setFromPrototypeMap.  Object storage revives file times
// through it too, which must not require the serializer to have initialized.

class DateNS extends Date {
	ns=0;
	constructor(a,b ) {
		// `a === undefined` rather than `!a`: 0 is a legitimate argument (the epoch),
		// and treating it as absent silently produced the current time instead.
		if( a === undefined ) super();
		else super(a);
		this.ns = b || 0;
	}
	toString() {
		return toISONS( this );
	}
	// `Z` is an offset of zero, not a statement about resolution, and ISO-8601 puts
	// no limit on fractional digits -- so this keeps Date's UTC contract while
	// carrying the full nanosecond fraction, trailing zeros trimmed.  Consumers that
	// only handle milliseconds (databases among them) floor the extra digits rather
	// than reject them.  JSON.stringify routes through here too, via Date's toJSON.
	toISOString() {
		const base = Date.prototype.toISOString.call( this ); // ...SS.mmmZ
		const frac = ( base.slice( -4, -1 ) + pad6( this.ns ) ).replace( /0+$/, '' );
		return base.slice( 0, -5 ) + ( frac ? '.' + frac : '' ) + 'Z';
	}
	// the same precision as a local wall clock plus its offset.  Note such strings
	// do not sort lexicographically the way Z-normalized ones do -- with an offset,
	// the fields before it are the local ones.
	toLocalISOString() {
		return toISONS( this );
	}
}

function pad6(num) {
	var norm = Math.floor(Math.abs(num));
	return (norm < 100000 ? '0' : '') + (norm < 10000 ? '0' : '') + (norm < 1000 ? '0' : '') + (norm < 100 ? '0' : '') + (norm < 10 ? '0' : '') + norm;
}

function toISO(this_) {
	if( this_.getTime()=== -62167219200000)
	{
		return "0000-01-01T00:00:00.000Z";
	}
	const yr = this_.getFullYear();
	const tzo = -this_.getTimezoneOffset(),
		dif = tzo >= 0 ? '+' : '-',
		pad = function(num) {
			var norm = Math.floor(Math.abs(num));
			return (norm < 10 ? '0' : '') + norm;
		},
		pad3 = function(num) {
			var norm = Math.floor(Math.abs(num));
			return (norm < 100 ? '0' : '') + (norm < 10 ? '0' : '') + norm;
		};
	return (( yr < 0 )? '-'+Math.abs(yr).toString().padStart( 6, '0' ):yr.toString().padStart( 4, "0" )) +
		'-' + pad(this_.getMonth() + 1) +
		'-' + pad(this_.getDate()) +
		'T' + pad(this_.getHours()) +
		':' + pad(this_.getMinutes()) +
		':' + pad(this_.getSeconds()) +
		'.' + pad3(this_.getMilliseconds()) +
		// the sign is already in `dif`; take the magnitude explicitly rather than
		// leaning on pad()'s abs, since a negative tzo makes `tzo % 60` negative.
		dif + pad(Math.abs(tzo) / 60) +
		':' + pad(Math.abs(tzo) % 60);
}

function toISONS(this_) {
	var tzo = -this_.getTimezoneOffset(),
		dif = tzo >= 0 ? '+' : '-',
		pad = function(num) {
			var norm = Math.floor(Math.abs(num));
			return (norm < 10 ? '0' : '') + norm;
		},
		pad3 = function(num) {
			var norm = Math.floor(Math.abs(num));
			return (norm < 100 ? '0' : '') + (norm < 10 ? '0' : '') + norm;
		},
		pad6 = function(num) {
			var norm = Math.floor(Math.abs(num));
			return (norm < 100000 ? '0' : '') + (norm < 10000 ? '0' : '') + (norm < 1000 ? '0' : '') + (norm < 100 ? '0' : '') + (norm < 10 ? '0' : '') + norm;
		};
	return this_.getFullYear() +
		'-' + pad(this_.getMonth() + 1) +
		'-' + pad(this_.getDate()) +
		'T' + pad(this_.getHours()) +
		':' + pad(this_.getMinutes()) +
		':' + pad(this_.getSeconds()) +
		'.' + pad3(this_.getMilliseconds()) + pad6(this_.ns) +
		// the sign is already in `dif`; take the magnitude explicitly rather than
		// leaning on pad()'s abs, since a negative tzo makes `tzo % 60` negative.
		dif + pad(Math.abs(tzo) / 60) +
		':' + pad(Math.abs(tzo) % 60);
}

module.exports = { DateNS, toISO, toISONS };
