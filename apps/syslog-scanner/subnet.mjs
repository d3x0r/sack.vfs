// Shared by the scanners: turn an address into the network it bans, and
// remember which networks were banned recently so a botnet spread across one
// subnet costs one ban command instead of one per address.

// `firewall = nft | ufw` in the conf picks both commands; an explicit
// `command=` / `unban command=` still overrides.
export const firewalls = {
	nft: { ban: "nft.sh", unban: "unban.sh" },     // own inet table + timed sets (any distro)
	ufw: { ban: "ufw.sh", unban: "ufw-unban.sh" }, // no timeouts; linear rules
};

// Adds the shared settings to a sack.Config handler; call settings.resolve()
// after .go() to fill in the firewall commands.
export function addSettings( configProcessor, lbs ) {
	let explicitBan = false, explicitUnban = false;
	lbs.firewall = lbs.firewall || "nft";
	lbs.banSeconds = lbs.banSeconds || 86400;
	lbs.v4prefix = lbs.v4prefix ?? 24;
	lbs.v6prefix = lbs.v6prefix ?? 64;
	configProcessor.add( "firewall=%m", ( c ) => lbs.firewall = c.trim() );
	configProcessor.add( "command=%m", ( c ) => { lbs.command = c.trim(); explicitBan = true; } );
	configProcessor.add( "unban command=%m", ( c ) => { lbs.unban = c.trim(); explicitUnban = true; } );
	configProcessor.add( "ban seconds=%m", ( c ) => lbs.banSeconds = Number( c ) || lbs.banSeconds );
	configProcessor.add( "v4 prefix=%m", ( c ) => lbs.v4prefix = clampBits( c, 32, lbs.v4prefix ) );
	configProcessor.add( "v6 prefix=%m", ( c ) => lbs.v6prefix = clampBits( c, 128, lbs.v6prefix ) );
	return {
		resolve() {
			const fw = firewalls[ lbs.firewall ];
			if( !fw ) console.log( "unknown firewall:", lbs.firewall, "(expected", Object.keys( firewalls ).join( " or " ) + ")" );
			if( !explicitBan && fw ) lbs.command = fw.ban;
			if( !explicitUnban && fw ) lbs.unban = fw.unban;
		},
	};
}

function clampBits( c, max, dflt ) {
	const n = Number( c );
	return Number.isInteger( n ) && n >= 0 && n <= max ? n : dflt;
}

// Returns { family: "ip" | "ip6", cidr: "a.b.c.0/24" } or null if `ip` isn't
// an address.  IPv4-mapped IPv6 (::ffff:1.2.3.4) counts as IPv4.
export function subnetOf( ip, v4bits = 24, v6bits = 64 ) {
	ip = String( ip ).trim();
	const mapped = /^::ffff:(\d+\.\d+\.\d+\.\d+)$/i.exec( ip );
	if( mapped ) ip = mapped[1];

	if( ip.includes( ":" ) ) {
		const words = expand6( ip );
		if( !words ) return null;
		return { family: "ip6", cidr: mask6( words, v6bits ).map( w => w.toString( 16 ) ).join( ":" ) + "/" + v6bits };
	}

	const parts = parse4( ip );
	if( !parts ) return null;
	const n = ( ( parts[0] << 24 ) | ( parts[1] << 16 ) | ( parts[2] << 8 ) | parts[3] ) >>> 0;
	const mask = v4bits ? ( 0xFFFFFFFF << ( 32 - v4bits ) ) >>> 0 : 0;
	const net = ( n & mask ) >>> 0;
	return { family: "ip", cidr: [ net >>> 24, ( net >>> 16 ) & 255, ( net >>> 8 ) & 255, net & 255 ].join( "." ) + "/" + v4bits };
}

function parse4( s ) {
	const parts = s.split( "." );
	if( parts.length !== 4 ) return null;
	const nums = parts.map( p => /^\d{1,3}$/.test( p ) ? Number( p ) : NaN );
	return nums.every( n => n >= 0 && n <= 255 ) ? nums : null;
}

// "2001:db8::1" -> [0x2001, 0xdb8, 0, 0, 0, 0, 0, 1]; null if malformed
function expand6( ip ) {
	ip = ip.split( "%" )[0]; // drop a zone id (fe80::1%eth0)
	const halves = ip.split( "::" );
	if( halves.length > 2 ) return null;
	const toWords = ( s ) => {
		const out = [];
		if( !s ) return out;
		for( const p of s.split( ":" ) ) {
			if( p.includes( "." ) ) { // embedded IPv4 tail
				const v4 = parse4( p );
				if( !v4 ) return null;
				out.push( ( v4[0] << 8 ) | v4[1], ( v4[2] << 8 ) | v4[3] );
			} else if( /^[0-9a-f]{1,4}$/i.test( p ) ) {
				out.push( parseInt( p, 16 ) );
			} else return null;
		}
		return out;
	};
	const head = toWords( halves[0] );
	const tail = toWords( halves[1] );
	if( !head || !tail ) return null;
	if( halves.length === 1 ) return head.length === 8 ? head : null;
	const fill = 8 - head.length - tail.length;
	if( fill < 1 ) return null;
	return [ ...head, ...new Array( fill ).fill( 0 ), ...tail ];
}

function mask6( words, bits ) {
	return words.map( ( w, i ) => {
		const keep = Math.max( 0, Math.min( 16, bits - i * 16 ) );
		return keep ? w & ( ( 0xFFFF << ( 16 - keep ) ) & 0xFFFF ) : 0;
	} );
}

// Remembers networks banned within the last `windowMs`.  Once a network is
// in the firewall, its other addresses stop reaching the logs, so lines that
// still arrive for it are in-flight stragglers and need no new command.
export function recentBans( windowMs ) {
	const seen = new Map(); // cidr -> time banned
	setInterval( () => {
		const cutoff = Date.now() - windowMs;
		for( const [ cidr, at ] of seen ) if( at < cutoff ) seen.delete( cidr );
	}, 3600_000 ).unref?.();
	return {
		// true if `cidr` was already banned in the window; otherwise records it
		check( cidr ) {
			const now = Date.now();
			const at = seen.get( cidr );
			if( at && now - at < windowMs ) return true;
			seen.set( cidr, now );
			return false;
		},
	};
}
