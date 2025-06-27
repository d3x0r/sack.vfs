
import {sack} from "sack.vfs"

const debug_ = false;

const iAm = import.meta.url.substring( 0, import.meta.url.lastIndexOf( '/' ) );

const  {setupImports,resolveImports} = await( import(iAm + "/importLoader.mjs" ));

const loadFrag = `import { _waitImports_ } from "${iAm}/importLoader.mjs";`;

function processSource( url, src, imports ){
	const lines = src.replace( "\r\n", "\n" ).replace( "\r", "\n" ).split('\n' );
	const output = [];
	let sentFunction = false;
	let nImport = 0;
	const as = /\s*as\s*/;
	output.push( '//# sourceURL='+url );
	for( let line of lines ) {
		if( !line.match( "import" ) ) {
			output.push( line );
			continue;
		}
		const parsename = line.match( /import\s*([^{\s]*)\s*from\s*([^\s]*)/ );
		const parse3 = (!parsename)?line.match( /import\s*(\*[^\s]*\s*as\s*[^\s]*)\s*from\s*([^\s;]*)/ ):null;
		const parse2 = (!parse3)?line.match( /import\s*({[^\}]*})\s*from\s*([^\s;]*)/ ):null;
		if( parsename ) {
			let end = parsename[2].length; if( parsename[2][end-1] == ';' ) end--; 
			const imp = new NetImport( parsename[2].substring(1,end-1) );
			//console.log( "qqpath..", parsename[2], imp.name );
			if( parsename[2][1] !== '/' && parsename[2][1] !== '.' ) {
				//console.log( "no path..", parsename[2] );
				output.push( line );						
				continue;
			}
			imports.push( imp );
			if( !sentFunction ) {
				output.push( loadFrag );
				sentFunction = true;
			}
			output.push( [ "const ",parsename[1]," = (_waitImports_)[", nImport++, "].default;"].join('') );		
		} else if( parse3 ) {
			let assign = parse3[1];
			let end = parse3[2].length; if( parse3[2][end-1] == ';' ) end--; 
			//console.log( "zzpath..", parse3[2] );
			if( parse3[2][1] !== '/' && parse3[2][1] !== '.' ) {
				//console.log( "no path..", parse3[2] );
				if( !sentFunction ) {
					output.push( loadFrag );
					sentFunction = true;
				}
				output.push( line );						
				continue;
			}
			const parts = assign.split( as );
			const imp = new NetImport( parse3[2].substring(1,end-1) )
			if( parts[0] === '*' ) {
				imports.push( imp );
				if( !sentFunction ) {
					output.push( loadFrag );
					sentFunction = true;
				}
				output.push( [ "const ",parts[1]," = (_waitImports_)[", nImport++, "];"].join('') );		
			} else {
				console.log( "ignoring an import...", line );
			}

		} else if( parse2 ) {
			const parse = parse2;
			let assign = parse[1].substring(1,parse[1].length-1);
			const assigns = assign.split( ',' );
			let a;
			for( a =0; a < assigns.length; a++ ) {
				const val = assigns[a];
				const as = /\s*as\s*/;
				if( val.match( as ) ) {
					const parts = val.split( as );
					assigns[a] = parts[0]+":"+parts[1];
				}else{
					//console.log( "Unhandled destructure in : ", line, assigns[a] );
				}
			}
			if( a === assigns.length ) {
				let end = parse[2].length; if( parse[2][end-1] == ';' ) end--; 
				if( parse[2][1] !== '/' && parse[2][1] !== '.' ) {
					//console.log( "no path..", parse[2] );
					output.push( line );						
					continue;
				}
				const from = parse[2].substring( 1, end-1 );
				//console.log( "path..", parse[2], from, sentFunction );
				const imp = new NetImport( from );
				imports.push( imp );
				if( !sentFunction ) {
					output.push( loadFrag );
					sentFunction = true;
				}
				output.push( [ "const {", assigns.join( ',' ), "} = (_waitImports_)[", nImport++, "];"].join('') );
			}
		} else {
			output.push( line );
		}
	}
	return output.join( '\n' );
}

const netImports = [];

async function netImport_( file, base, imp ) {
	const fileUrl = new URL( file, base );
	//console.log( "made name:", file, base, fileUrl.href );

	for( let impChk of netImports ) {
		if( imp === impChk ) continue; 
		debug_ && console.log( "Chcking for import:", impChk.name, imp.name );
		if( impChk.url.href === fileUrl.href ) {
			if( !impChk.module )  {
				debug_ && console.log( "return Wait", impChk.wait );
				return impChk.wait;
			}
			debug_ && console.log( "return module" );
			return impChk.module;
		}
	}
	
	const fileData = sack.HTTP.get( { hostname: fileUrl.hostname, port: fileUrl.port || 80, method : "get", path : fileUrl.pathname } );
	if( fileData.statusCode === 200 ) {
		const script = fileData.content;
		const loader = setupImports( file );
		const imports = loader.imports;
		let allImports;
		const newScript = processSource( fileUrl.href, script, imports );
		if( imports.length ) {
			debug_ && console.log( "imports:", imports.length, imports.map( imp=>imp.name ) );
			allImports = imports.map( async impDo=>{ 
				impDo.url = new URL( impDo.name, fileUrl.href );
				debug_ && console.log( "import map:", impDo.name );
				netImports.push( impDo );
				impDo.wait = netImport_( impDo.name, fileUrl.href, impDo );
				impDo.module = await impDo.wait;
				if( impDo.done ) impDo.done( impDo.module );
				debug_ && console.log( "resolve promisall wait:", impDo.module );
				return impDo.module;
			} );
			debug_ && console.log( file, "Wait for imports", base, allImports );
			allImports = await Promise.all( allImports );
			debug_ && console.log( file, "Waited for imports" );
			resolveImports( loader, allImports );
		}
		debug_ && console.log( "Run:", file, base, newScript );
		imp.module = await import(`data:text/javascript;base64,${Buffer.from(newScript).toString(`base64`)}`);
		debug_ && console.log( "Ran:", file, !!imp.done );
		if( imp.done ) { 
			debug_ && console.log( "mark wait event complete too." );
			imp.done( imp.module );
		}
	} else {
		console.log( fileData );
		throw new Error( "Failed to load script:" + file + " " + fileData );
	}
	return imp.module;
}

export async function netImport( file, base ) {
	const fileUrl = new URL( file, base );

	for( let impChk of netImports ) {
		if( (impChk.name === file) || (impChk.url.href === fileUrl.href) ) {
			if( !impChk.module )  {
				return impChk.wait;
			}
			return impChk.module;
		}else {
			//console.log( "why is this false?", file, base, impChk.name );
		}
	}
	debug_ && console.log( "import:", file )
	const imp = new NetImport( file );
	netImports.push( imp );
	imp.url = fileUrl;
	imp.wait = new Promise( (res,rej)=>{imp.done = res} );
	return netImport_( file, base, imp );
}

class NetImport {
	name = null; url=null; module=null;
	wait = null; // promise for module (circular?)
	done = null; // set as resolve function

	constructor( name ) {
		this.name = name;
	}
}
