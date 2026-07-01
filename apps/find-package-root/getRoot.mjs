import {sack} from "sack.vfs"


export function getRoot( min ) {
	const disk = sack.Volume();
	let levels = (typeof min === "Number")?min:0;

	do {
		let s = '';
		for( let l = 0; l < levels; l++ ) s += s?"/..":"..";
		if( !s ) s = '.';
		const d = disk.dir( s, "package.json" );
      //console.log( "d?", s+ "/package.json", d );
		if( d.length ) {
			//console.log( "s:", s );
			return levels;
		}
		levels++;
	} while(levels < 6);
   return 0;
}
