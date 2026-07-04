import {sack} from "sack.vfs"


export function getRoot( min ) {
	const disk = sack.Volume();
	let levels = (typeof(min )=== "number")?min:0;
	do {
		let s = '';
		for( let l = 0; l <= levels; l++ ) s += s?"/..":"..";
		if( !s ) s = '.';
		const d = disk.dir( s, "package.json" );
		if( d.length ) {
			return levels;
		}
		levels++;
	} while(levels < 6);
   return 0;
}
