
function bench1() {
	const seed = xmur3( "test" );
	const seed2 = xmur3( new Date().toISOString() );
	console.log( "seeds:", seed(), seed2() );
	console.log( "seeds:", seed(), seed2() );
	console.log( "seeds:", seed(), seed2() );
	console.log( "seeds:", seed(), seed2() );
	const sfc = sfc32( seed(),seed(),seed(),seed() );
	const mul = mulberry32( seed(),seed(),seed(),seed() );
	const xor = xoshiro128ss( seed(),seed(),seed(),seed() );
	const jsf = jsf32( seed(),seed(),seed(),seed() );
        
	const SFC = SFC32( "test" );
	const MUL = MUL32( "test" );
	const XOR = XOR32( "test" );
	const JSF = JSF32( "test" );
        
        
        function Do(c) {
        	const start = Date.now();
        	let n = 0;
                let i = 0;
        	for( ; i < 10000000; i++ ) n += c();
                const end = Date.now();
                
                return { n:n, del:end-start, tries:i, tpms:i/(end-start) } ;
        }
        
        console.log( "SFC", Do( sfc ) );
        console.log( "MUL", Do( mul ) );
        console.log( "XOR", Do( xor ) );
        console.log( "JSF", Do( jsf ) );
        
        console.log( "SFC", Do( SFC ) );
        console.log( "MUL", Do( MUL ) );
        console.log( "XOR", Do( XOR ) );
        console.log( "JSF", Do( JSF ) );
        
}

bench1();

