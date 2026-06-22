'use strict';
// require.js
// Node.js only: adds a import() hook for .jsox files, just like the native
// hook for .jsox files.  Also adds support for HTTP/HTTPS requested files.
//
// Usage:
// import {default as config} from "config.jsox";
import fs from "node:fs";
import url from "node:url";
import path from "node:path";
import util from "node:util";
const moduleName = "@d3x0r/sack-gui";
import {sack} from "@d3x0r/sack-gui";

const debug_ = false;
const forceModule = ( process.env.FORCE_IMPORT_MODULE ) || false;
const forceHttpModule = ( process.env.FORCE_IMPORT_HTTP_MODULE ) || false;

/**
 * @param {string} url
 * @param {Object} context (currently empty)
 * @param {Function} defaultGetFormat
 * @returns {Promise<{ format: string }>}
 */
export async function getFormat(url, context, defaultGetFormat) {
	const exten = path.extname( url );
	//if( exten === '' ) return { format:'module' }
	debug_ && console.log( "Format of:", url, context, defaultGetFormat );
	if( exten === ".jsox" || exten === ".json6" ){
	    return { format: 'module' };
	}
	return defaultGetFormat(url,context );
}

/**
 * @param {string} url
 * @param {{ format: string }} context
 * @param {Function} defaultGetSource
 * @returns {Promise<{ source: !(string | SharedArrayBuffer | Uint8Array) }>}
 */
export async function getSource(urlin, context, defaultGetSource) {
	const exten = path.extname( urlin );
	if( exten === ".jsox" || exten === '.json6' ){
	  	const { format } = context;
		const file = url.fileURLToPath(urlin);
		const result = fs.readFileSync(file).toString("utf8");
   	 return {
   	   source: result,
	    };
	}
  // Defer to Node.js for all other URLs.
  return defaultGetSource(urlin, context, defaultGetSource);
}

/**
 * @param {!(string | SharedArrayBuffer | Uint8Array)} source
 * @param {{
 *   format: string,
 *   url: string,
 * }} context
 * @param {Function} defaultTransformSource
 * @returns {Promise<{ source: !(string | SharedArrayBuffer | Uint8Array) }>}
 */
export async function transformSource(source, context, defaultTransformSource) {
	const exten = path.extname( context.url );
	if( exten === ".jsox" ){
    return {                                                                                       
      source: "const data = JSOX.parse( '" + escape(source) + "'); export default data;",
    };
	}
	if( exten === ".json6" ){
    return {                                                                                       
      source: "const data = JSON6.parse( '" + escape(source) + "'); export default data;",
    };
	}
  return defaultTransformSource(source, context, defaultTransformSource);
}

export async function load(urlin, context, defaultLoad) {
	const { format } = context;
	const exten = path.extname( urlin );
	debug_&&console.log( "LOAD:", urlin, exten, context );
	if( urlin.startsWith( "module://./" ) ) {
		// this handles forcing load of a module (even if extension is .js)
		// otherwise loading the file normally would work better.
		// This is for pretending that the load happens as if it happened over the HTTP connection
		// The rules for the HTTP loader would redirect /node_modules and /common to different locations than the resource path.
		// environment variables that control this: COMMON_PATH for /common; NODE_MODULE_PATH for "/node_modules"; and RESOURCE_PATH for everything else
		context.format="module";
		let pathOnly = urlin.substring( 10 );
		debug_ && console.log( "pathOnly:", pathOnly, urlin )
		if( pathOnly.startsWith( "/common" ) ) pathOnly = pathOnly.replace( "/common", process.env.COMMON_PATH ) ?? "../..";
		else if( pathOnly.startsWith( "/node_modules" ) ) pathOnly = pathOnly.replace( "/node_modules", process.env.NODE_MODULE_PATH ) ?? "../../node_modules";
		else pathOnly = ( process.env.RESOURCE_PATH??".") + pathOnly;
		urlin = url.pathToFileURL( pathOnly ).href;
		debug_ && console.log( "Resulting urlin:", urlin );
	} else if( urlin.startsWith( "http://" ) || urlin.startsWith( "https://" ) || urlin.startsWith( "HTTP://" ) || urlin.startsWith( "HTTPS://" ) ) {
		const url = new URL( urlin );
		const exten = path.extname( url.pathname );
		debug_&&console.log( "Real extension:", exten );
		url.has = "";
		url.search = "";
		return new Promise( (res,rej)=>{
			const request = { hostname:url.hostname, path:url.pathname, port:Number(url.port), onReply(result){

				if( result.statusCode === 200  ) {
					debug_ && console.log( "Got back success...", !forceModule, !forceHttpModule, ( !forceModule && !forceHttpModule && exten===".js" )?"commonjs":"module", result.content.substr(0, 40) )
					res( {
						format:( !sack.import.forceNextModule && !forceModule && !forceHttpModule && exten===".js" )?"commonjs":"module",
						source:result.content,
						shortCircuit:true,
					} );
				} else {
					rej(util.format( "request for (", urlin, ") failed:", result ) );
					//sack.log(  );
				}
			} };
			sack.HTTP.get( request );
		})
	}
	else if( exten === ".jsox" || exten === '.json6' || exten === ".json" ){
	  	const { format } = context;
		debug_&&console.log( "urlin is a string?", typeof urlin );
		const file = url.fileURLToPath(urlin);
		debug_&&console.log( "FILE?:", urlin, file )
		const result = fs.readFileSync(file).toString("utf8");
   	 

		if( exten === ".jsox" || exten === ".json" ){
			return {
			   format:"module",
			   source: "import {sack} from '"+moduleName+"'; const data = sack.JSOX.parse( '" + escape(result) + "'); export default data;",
			   shortCircuit:true,
			};
		}
		if( exten === ".json6" ){
		    return {
			   format:"module",
			   source: "import {sack} from '"+moduleName+"'; const data = sack.JSON6.parse( '" + escape(result) + "'); export default data;",
			   shortCircuit:true,
			};
		}
	} else if( sack.import.modules.find( (m)=>urlin.endsWith( m ) ) ) {
		//console.log( "Setting format to module here...");
		context.format="module";
	} else {
		debug_ && console.log( "forcing .js to be module?", process.env.FORCE_IMPORT_MODULE, forceModule, exten );
		if( forceModule && exten === ".js" ) context.format="module";
	}
	return defaultLoad(urlin, context, defaultLoad);

}

function escape(string) {
	let output = '';
	if( !string ) return string;
	for( let n = 0; n < string.length; n++ ) {
		const ch = string[n];
		if( ch === '\n' ){
			output += "\\n";
		} else if( ch === '\r' ){
			output += "\\r";
		} else {
			if( ( ch === '"' ) || ( ch === '\\' ) || ( ch === '`' )|| ( ch === '\'' )) {
				output += '\\';
			}
			output += ch;
		}
	}
	return output;
};

/**
 * @returns {string} Code to run before application startup
 */
// Preloads JSON6 as a global resource; which is then used in the transformed source above.
export function globalPreload() {
	// this changes in 17+; check node version from process? and export the right thing?
	return getGlobalPreloadCode();
}

export function getGlobalPreloadCode() {
  return `\
const { createRequire } = getBuiltin('module');
const requireJSOX = createRequire('${escape(url.fileURLToPath( import.meta.url ))}');
globalThis.SACK = requireJSOX( "${moduleName}" );
globalThis.JSOX = globalThis.SACK.JSOX;
globalThis.JSON6 = globalThis.SACK.JSON6;
`;

}

export function initialize( data ) {
	//console.log( "Got:", data );	
}

export function resolve( specifier, context, nextResolve ) {
	debug_ && console.log( "resolve continue?", specifier, context );
	if( specifier.startsWith( "http:" ) || specifier.startsWith( "https:" ) || specifier.startsWith( "module:" )  ) {
		return {
	      shortCircuit: true,
	      url: context.parentURL ?
	        new URL(specifier, context.parentURL).href :
	        new URL(specifier).href,
	    };	
		return {
			shortCircuit: true,
			url: parentURL ?
		                new URL(specifier, context.parentURL).href :
				new URL(specifier).href,
		};	
	} else if( specifier.endsWith( ".jsox" ) || specifier.endsWith( ".json" ) ){
		return {
			shortCircuit: true,
			url: context.parentURL ?
		                new URL(specifier, context.parentURL).href :
				new URL(specifier).href,
		};	
	} else if( specifier.endsWith( ".json6" ) ){
		return {
			shortCircuit: true,
			url: context.parentURL ?
		                new URL(specifier, context.parentURL).href :
				new URL(specifier).href,
		};	
		
	}
	//console.log( "Force is:", sack.import.forceNextModule, context.format );
	if( sack.import.modules.find( (m)=>specifier.endsWith( m ) ) ) {
		//console.log( "Setting format to module here...", specifier, context );
		context.format="module";
	} else if( forceModule && context.format !== "module") {
		//console.log( "Return forced module:", specifier, context );
		return nextResolve( specifier, Object.assign( {}, context, { format: "module" } ) );
		
	}
	// nextResolve( specifier, {modified options} );
	return nextResolve( specifier, context );
}


import module from "node:module";
const version = process.version.slice(1).split('.'  ).map( a=>Number(a));
if( version[0] >= 21 || ( version[0] >= 20 && version[1] >= 6 ) ) {
	// https://nodejs.org/api/module.html#resolvespecifier-context-nextresolve
	const fileURL = url.pathToFileURL("./", import.meta.url );
	debug_ && console.log( "FileURL?", fileURL );
	module.register( moduleName + "/import.mjs", fileURL.href );
}

export function forceNextModule( yesno ) { sack.import.forceNextModule = yesno }


/*
{
  parentURL: import.meta.url,
  data: { number: 1, port: port2 },
  transferList: [port2],
}
*/
// --import 'data:text/javascript,import { register } from "node:module"; import { pathToFileURL } from "node:url"; register("sack.vfs/import.mjs", pathToFileURL("./"));'

//--import "sack.vfs/import"
