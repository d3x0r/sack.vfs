
import {sack} from "sack.vfs"


const iAm = import.meta.url.substring( 0, import.meta.url.lastIndexOf( '/' ) );
console.log( "I AM:", import.meta.url, iAm );


const  {setupImports,resolveImports} = await( import(iAm + "/importLoader.mjs" ));

const loadFrag = `
console.log( "Can't load back in?" );
import { _waitImports_ } from "${iAm}/importLoader.mjs";
console.log( "wait?", _waitImports_ );
//await wait;
`;

function processSource( src, imports ){
	const lines = src.replace( "\r\n", "\n" ).replace( "\r", "\n" ).split('\n' );
	const output = [];
	let sentFunction = false;
	let nImport = 0;
	const as = /\s*as\s*/;
	for( let line of lines ) {
		if( !line.match( "import" ) ) {
			output.push( line );
			continue;
		}
		//console.log( `line: [${line}]`);
		//const parse = line.match( /import\s*(\{[^\}]&})|( (\d+)(,\s*\d+)* )\s*from\s*([^\s]*)/ );
		const parsename = line.match( /import\s*([^\s]*)\s*from\s*([^\s]*)/ );
		//console.log( "parsename=", parsename );

		const parse3 = line.match( /import\s*(\*[^\s]*\s*as\s*[^\s]*)\s*from\s*([^\s;]*)/ );
		//console.log( "parse3?", parse3 );
		const parse2 = line.match( /import\s*({[^\}]*})\s*from\s*([^\s;]*)/ );
		if( parsename ) {
			imports.push( new NetImport( parsename[2].substring(1,parsename[2].length-1) ) );
			output.push( [ "const ",parsename[1]," = (_waitImports_)[", nImport++, "].default;"].join('') );		
		} else if( parse3 ) {
			let assign = parse3[1];

			//console.log( "Val is...", a, val );
			const parts = assign.split( as );
			//console.log( "split is:", parts );
			if( parts[0] === '*' ) {
				imports.push( new NetImport( parse3[2].substring(1,parse3[2].length-1) ) );
				output.push( [ "const ",parts[1]," = (_waitImports_)[", nImport++, "];"].join('') );		
			} else {
				console.log( "ignoring an import...", line );
			}

		} else if( parse2 ) {
			const parse = parse2;
			let assign = parse[1].substring(1,parse[1].length-1);
			const assigns = assign.split( ',' );
			//console.log( 'parse2?', parse, assigns );
			let a;
			for( a =0; a < assigns.length; a++ ) {
				const val = assigns[a];
				const as = /\s*as\s*/;
				if( val.match( as ) ) {
					//console.log( "Val is...", a, val );
					const parts = val.split( as );
					//console.log( "split is:", parts );
					assigns[a] = parts[0]+":"+parts[1];
					//console.log( "became:", assigns[a] );
				}else{
					console.log( "Unhandled destructure in : ", line );
				}
			}
			if( a === assigns.length ) {
				const from = parse[2].substring( 1, parse[2].length-1 );
				imports.push( new NetImport( from ) );
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
	//console.log( "args:", file, base, imp );
	const fileUrl = new URL( file, base );
	//console.log( "url:", imp.url.href );

	for( let impChk of netImports ) {
		if( imp === impChk ) continue; 
		//console.log( "name?", impChk.name, file , impChk.url.href, fileUrl.href );
		if( impChk.name === file || impChk.url.href === fileUrl.href ) {
		console.log( "Return wait?",  impChk );
			if( !impChk.module )  {
				return impChk.wait;
			}
			return impChk.module;
		}
	}

	console.trace( ' --------- did dnot find existing? (or is self)', file, base, fileUrl, netImports);
	const fileData = sack.HTTP.get( { hostname: fileUrl.hostname, port: fileUrl.port || 80, method : "get", path : fileUrl.pathname } );
	//console.log( "fileData:", fileData );
	if( fileData.statusCode === 200 ) {
		const script = fileData.content;
		const loader = setupImports( file );
		const imports = loader.imports;
		let allImports;
		//console.log( "Script WAS:", script );
		const newScript = processSource( script, imports );
		if( imports.length ) {
			allImports = imports.map( async impDo=>{ 
				impDo.url = new URL( impDo.name, fileUrl.href );
				return impDo.module = await netImport_( impDo.name, fileUrl.href, impDo );
			} );
			console.log( "Have imports to wait for...", file, allImports );
			allImports = await Promise.all( allImports );
			resolveImports( loader, allImports );
			console.log( "Had imports to wait for...", file, allImports );
		}
		//console.log( "Script:", newScript );
		console.log( "Do so import...." );
		const pendingImport = import(`data:text/javascript;base64,${Buffer.from(newScript).toString(`base64`)}`);
		console.log( "Import can't complete, right, because async?" );
		imp.module = await pendingImport;
		console.log( "And now I can do my own module:", imp.module );
		if( imp.done ) { imp.done( imp.module ); imp.done = null }
		console.log( "result?", imp );
   	//instance.demo() // Hello World!
	} else {
		throw new Error( "Failed to load script:" + file + " " + fileData );
	}
	return imp.module;
}

export async function netImport( file, base ) {
		// my imprt
	const fileUrl = new URL( file, base );

	for( let impChk of netImports ) {
		console.log( "is loaded name?", impChk.name, file , impChk.url.href, fileUrl.href,impChk.url.href=== fileUrl.href );
		if( (impChk.name === file) || (impChk.url.href === fileUrl.href) ) {
			console.log( "This IS true...", impChk.module, impChk );
			if( !impChk.module )  {
				console.log( "return wait" );
				return impChk.wait;
			}
			console.log( "return the module (in async)" );
			return impChk.module;
		}else console.log( "why is this false?" );
	}
	console.log( "External first load..................", netImports );
	const imp = new NetImport( file );
	netImports.push( imp );
	imp.url = fileUrl;
	imp.wait = new Promise( (res,rej)=>{imp.done = res} );
	//console.log( "so?", imp );
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

export {netImport as import};
