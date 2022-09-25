

import {sack} from "sack.vfs";
import path from "path";
import {openServer} from "./server.mjs";

export function uExpress() {
	/* router really */
	function makeRouter() {
		return {
			pre_mappings : new Map(),
			pre_req_maps : [],
			mappings : new Map(),
			req_maps : []
		};
        	
	}

	const defaultMap = makeRouter( );
	
	const pre_mappings = defaultMap.pre_mappings;
	const pre_req_maps = defaultMap.pre_req_maps;
	const mappings = defaultMap.mappings;
	const req_maps = defaultMap.req_maps;
        
	return {
        	all(a,b ) {
                	if( "string" === typeof a )
	                	pre_mappings.set( a, b ); // b(req,res,next /*next()*/ )
                        else
	                        pre_req_maps.push( { expr:a, cb:b } );
                },
        	get( a, b ) {
                	if( "string" === typeof a )
        	        	mappings.set( a, b );
                        else
	                        req_maps.push( { expr:a, cb:b } );
                },
        	post( a, b ) {
                	if( "string" === typeof a ) 
	                	mappings.set( a, b );
                        else
	                        req_maps.push( { expr:a, cb:b } );
                },
                handle( req, res) {
			//console.log( "Look for request:", req, res );
			const parts = req.url.split("?");
			const url = unescape(parts[0]);
			const filepath = path.dirname(url)+path.basename(url)+path.extname(url);
			const name = path.basename(url);
			const type = path.extname(url);
                        
			//console.log( "Think parts is:", filepath, name, type, parts[1] );
			let cb;
			let ranOne = false;
			let handled = false;
			if( cb = pre_mappings.get( filepath ) ) {
				let runNext = false;
				ranOne = true;
				handled = cb( req, res, ()=>{ runNext = true; } );
				if( !runNext ) 
					return handled;
			}
			for( let map of req_maps ) {
				if( map.expr.match( filepath ) ) {
					let runNext = false;
					ranOne = true;
					map.cb( req, res, ()=>(runNext = true) );
					if( !runNext ) break;
				}
			}

			//console.log( "mappings:", mappings );
                	if( cb = mappings.get( filepath ) ) {
				//console.log( "got cb?" );
				ranOne = true;
                       		handled = cb( req, res, ()=>{} );
			}
			
			return handled;
		}
        }
}

