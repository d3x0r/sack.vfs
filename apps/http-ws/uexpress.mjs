

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
			const parts = req.url.split("?");
			const url = unescape(parts[0]);
			const filepath = url;//path.dirname(url)+((path.dirname(url)&&path.basename(url))?"/":"")+path.basename(url);
                        
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
				if( map.expr.test( filepath ) ) {
					//console.log( "expr?", map, map.expr );
					let runNext = false;
					ranOne = true;
					handled = map.cb( req, res, ()=>(runNext = true) );
					if( !runNext ) break;
				}
			}

			if( cb = mappings.get( filepath ) ) {
				//console.log( "got cb?" );
				ranOne = true;
                handled = cb( req, res, ()=>{} );
			}
			
			return handled;
		}
    }
}

