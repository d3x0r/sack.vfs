
if( process.argv.length < 3 ) {
	console.log( "need to specify a name or regexp to match" );
        process.exit();
}
import {sack} from "sack.vfs";

const disk = sack.Volume();
const dir = disk.dir();
const args = process.argv.slice(2);
const regargs = args.map( a=>new RegExp(a) );

// codepage 437 to unicode.
/*
const mapLines = [charMap.slice(0,16),charMap.slice(16,32),charMap.slice(32,48),charMap.slice(48,64),charMap.slice(64,80),charMap.slice(80,96),charMap.slice(96,112),charMap.slice(112,128)];
const chars =  [ 
  'ÇüéâäàåçêëèïîìÄÅ',
  'ÉæÆôöòûùÿÖÜ¢£¥₧ƒ',
  'áíóúñÑªº¿㈐¬½¼¡«»',
  '░▒▓│┤╡╡╖╕╣║╗╝╜╛┐',
  '└┴┬├─┼╞╟╚╔╩╦╠═╬╧',
  '╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀',
  'αßΓπΣσµτΦΘΩδ∞φε∩',
  '≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ '
];


const aa = mapLines.map( l=>l.map( n=>String.fromCodePoint( n )).join('') );
console.log( "Map:", charMap.length, aa );
}

*/
const charMap = [
	0xc7, 0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0xe5, 0xe7, 0xea, 0xeb, 0xe8, 0xef, 0xee, 0xec, 0xc4, 0xc5,
        0xc9, 0xe6, 0xc6, 0xf4, 0xf6, 0xf2, 0xfb, 0xf9, 0xff, 0xd6, 0xdc, 0xa2, 0xa3, 0xa5, 0x20a7, 0x192,
        0xe1, 0xed, 0xf3, 0xfa, 0xf1, 0xd1, 0xaa, 0xba, 0xbf, 0x3210, 0xac, 0xbd, 0xbc, 0xa1, 0xab, 0xbb,
        0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2561, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255B, 0x2510,
        0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
        0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
        0x3b1, 0xdf, 0x393, 0x3c0, 0x3a3, 0x3c3, 0xb5, 0x3c4, 0x3a6, 0x398, 0x3a9, 0x3b4, 0x221e, 0x3c6, 0x3b5, 0x2229,
        0x2261, 0xb1, 0x2265, 0x2264, 0x2320, 0x2321, 0xf7, 0x2248, 0xb0, 0x2219, 0xb7, 0x221a, 0x207f, 0xb2, 0x25a0, 0xa0
]

const uMap = charMap.map( tou8 );

//console.log( 'dir:', process.argv, args, regargs );

for( let file of dir ) {
	let a;
        ///console.log( "test:", file.name );
	for( a of args )
		if( file.name === a )
                {
                	fixFile( file.name );
                        break;
                }else a = null;
        if( !a )
		for( a of regargs ) {
	                //console.log( "arg:", a, file.name );
			if( a.test( file.name ) )
                	{
                		fixFile( file.name );
	                        break;
        	        }
                }
}

function tou8(n) {
        if( n > 0x7F ) {
            if( n > 0xffff ) {
                return [ ( n >> 18 ) | 0xF0,
	                ( ( n >> 12 ) & 0x3f ) | 0x80,
        	        ( ( n >> 6 ) & 0x3f ) | 0x80,
                	( ( n ) & 0x3f ) | 0x80]
            }
            else {
                if( n > 0x7ff ) {
                    return [ ( n >> 12 ) | 0xE0,
        	             ( ( n >> 6 ) & 0x3f ) | 0x80,
                	     ( ( n ) & 0x3f ) | 0x80 ];
                } else {
                    return [ ( n >> 6 ) | 0xC0,
	                     ( ( n ) & 0x3f ) | 0x80]
                }
            }
        } else {
            return [n];
        }
}

function fixFile(f){
	//console.log( "testing file:", f );
	const file = new Uint8Array( disk.read( f ) );
        let badAt = [];
        let isBad = 0;
        //console.log( "File is a buffer?", file.byteLength, file );
        for(let i = 0; i < file.length; i++ ) { 
        	if( file[i] & 0x80 ) {
			//console.log( "checkat:", file[i].toString(16),i );
                	if( (file[i] & 0xE0 )=== 0xc0 ) {
                        	if( (file[i+1] & 0xC0 ) !== 0x80 ) {
                                	badAt.push(i);
                                	isBad+=uMap[file[i]-127].length-1;
                                }else i+=1;
                        }
                	else if( (file[i] & 0xF0) === 0xE0 ) {
                        	if( ( (file[i+1] & 0xC0) !== 0x80 ) || ( (file[i+2] & 0xC0) !== 0x80 ) ) {
                                	badAt.push(i);
                                	isBad+=uMap[file[i]-127].length-1;
                                }else i+=2;
                        }
                	else if( (file[i] & 0xF0) === 0xF0 ) {
                        	if( ( (file[i+1] & 0xC0) !== 0x80 ) || ( (file[i+2] & 0xC0 )!== 0x80 ) || ( (file[i+3] & 0xC0) !== 0x80 ) ) {
                                	badAt.push(i);
                                	isBad+=uMap[file[i]-127].length-1;
                                } else i+=3;
                        }
                }
        }
        if( isBad ) {
                const outFile = new Uint8Array( file.length+isBad );
		//const newFile = [];
		let o = 0;
                let bad = 0;
                for(let i = 0; i < file.length; i++ ) {
                	if( bad<badAt.length ) {
	                	const nextBad = badAt[bad++];
				//console.log( "next:", i, nextBad, bad );
        	                while( i < nextBad ) {
					//console.log( "copy char?", i, nextBad, file[i].toString(16) );
                	        	outFile[o++] = file[i++];
                        	}
				//console.log( "outputting real..." );
	                	if( file[i] & 0x80 ) {
        	                	const chars = uMap[file[i]-127];
					//console.log( "output for:", file[i].toString(16), chars );
                	                for( let c of chars )
                        	        	outFile[o++] = c;
	                        } else {
        	                	console.log( "(ERROR)Should have been skipped? or last segment?", o, i, bad, nextBad, badAt );
                	        	outFile[o++] = file[i];
                        	}
                        } else
              	        	outFile[o++] = file[i];
                }
		console.log( "output:", o, outFile );
        	disk.write( f+".new", outFile );                
        }else
		console.log( f, "is ok." );
}
                                                    