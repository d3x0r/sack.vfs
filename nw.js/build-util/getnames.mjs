import {sack} from "sack.vfs"

//const x = sack.registry.get( "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows",  );
//console.log( "x:", x );
const disk = sack.Volume();

function f1() {
	const def_output = ["LIBRARY NW.DLL", "EXPORTS"];

	const buf1 = [];
	new sack.Task( {bin:"dumpbin.exe", args:["/exports", "nw.dll"], input(buf){ buf1.push(buf) }, end() {
		const output = buf1.join('').split('\r\n' );
			//console.log( "lines:", output );
		let n;
		for( n = 0; n < output.length; n++ ) {
			if( output[n].startsWith( "    ordinal" ) ) break;
		}
		if( n < output.length )  {
        	const words = output.slice( n+2 ).map( line=>line.split(/[ ]+/) );
			words.forEach( line=> {
				if( line.length > 4 )
				def_output.push( line[4] );
			} );
	        //console.log( "stuff:", words );
			disk.write( "nw.def", def_output.join('\n' ) );
				new sack.Task( {bin:"lib.exe", args:["/def:nw.def", "/out:nw.lib", "/machine:x64"], end() {} } );
		}
	} } );
}

function f2() {
	const def_output = ["LIBRARY NODE.DLL", "EXPORTS"];
	const buf1 = [];
	new sack.Task( {bin:"dumpbin.exe", args:["/exports", "node.dll"], input(buf){ buf1.push(buf) }, end() {
		const output = buf1.join('').split('\r\n' );
			//console.log( "lines:", output );
		let n;
		for( n = 0; n < output.length; n++ ) {
			if( output[n].startsWith( "    ordinal" ) ) break;
		}
		if( n < output.length )  {
        	const words = output.slice( n+2 ).map( line=>line.split(/[ ]+/) );
			words.forEach( line=> {
				if( line.length > 4 )
				def_output.push( line[4] );
			} );
	        //console.log( "stuff:", words );
			disk.write( "node.def", def_output.join('\n' ) );

				new sack.Task( {bin:"lib.exe", args:["/def:node.def", "/out:node.lib", "/machine:x64"], end() {} } );

		}
	} } );
}

f1();
f2();