
/**
 * @typedef {Object<string, unknown> & {
 *   url: string | null,
 *   method?: string,
 *   path?: string,
 *   query?: URLSearchParams,
 *   headers?: Object<string, string>,
 *   connection?: { headers?: Object<string, string>, [key: string]: unknown }
 * }} UExpressRequest
 */

/**
 * @typedef {Object<string, unknown> & {
 *   headersSent?: boolean,
 *   writeHead: (statusCode: number, headers?: Object<string, string>) => void,
 *   end: (body?: unknown) => void
 * }} UExpressResponse
 */

/**
 * @callback UExpressNext
 * @returns {void}
 */

/**
 * @callback UExpressHandler
 * @param {UExpressRequest} req
 * @param {UExpressResponse} res
 * @param {UExpressNext} next
 * @returns {unknown | Promise<unknown>}
 */

/**
 * @typedef {string | RegExp} UExpressRoute
 */

/**
 * @typedef {{
 *   use: ((handler: UExpressHandler) => void) & ((route: UExpressRoute, handler: UExpressHandler) => void),
 *   all: (route: UExpressRoute, handler: UExpressHandler) => void,
 *   get: (route: UExpressRoute, handler: UExpressHandler) => void,
 *   post: (route: UExpressRoute, handler: UExpressHandler) => void,
 *   handle: (req: UExpressRequest, res: UExpressResponse) => boolean
 * }} UExpressApp
 */

/**
 * Create a small Express-like router for the sack.vfs HTTP/WebSocket server.
 * @returns {UExpressApp}
 */
export function uExpress() {
	/**
	 * @param {string} uri
	 * @returns {string}
	 */
	function decodeRequestURI( uri ) {
		try {
			return decodeURI( uri );
		} catch( err ) {
			return uri;
		}
	}

	/* router really */
	/**
	 * @returns {{
	 *   use_mappings: Array<{ path: string, cb: UExpressHandler }>,
	 *   use_req_maps: Array<{ expr: RegExp, cb: UExpressHandler }>,
	 *   mappings: Record<string, Map<string, UExpressHandler>>,
	 *   req_maps: Record<string, Array<{ expr: RegExp, cb: UExpressHandler }>>
	 * }}
	 */
	function makeRouter() {
		return {
			use_mappings : [],
			use_req_maps : [],
			mappings : {
				ALL : new Map(),
				GET : new Map(),
				POST : new Map()
			},
			req_maps : {
				ALL : [],
				GET : [],
				POST : []
			}
		};
        	
	}

	const defaultMap = makeRouter( );
	
	const use_mappings = defaultMap.use_mappings;
	const use_req_maps = defaultMap.use_req_maps;
	const mappings = defaultMap.mappings;
	const req_maps = defaultMap.req_maps;

	/**
	 * @param {UExpressRequest} req
	 * @returns {string}
	 */
	function getMethod( req ) {
		return ( req.method || "GET" ).toUpperCase();
	}

	/**
	 * @param {string} method
	 * @param {UExpressRoute} a
	 * @param {UExpressHandler} b
	 * @returns {void}
	 */
	function addMapping( method, a, b ) {
		if( "string" === typeof a )
			mappings[method].set( a, b );
		else
			req_maps[method].push( { expr:a, cb:b } );
	}

	/**
	 * @param {UExpressRoute | UExpressHandler} a
	 * @param {UExpressHandler} [b]
	 * @returns {void}
	 */
	function addUseMapping( a, b ) {
		if( "function" === typeof a ) {
			b = a;
			a = "/";
		}
		if( !b )
			throw new TypeError( "uExpress.use requires a handler" );
		if( "string" === typeof a )
			use_mappings.push( { path:a, cb:b } );
		else
			use_req_maps.push( { expr:a, cb:b } );
	}

	/**
	 * @param {RegExp} expr
	 * @param {string} filepath
	 * @returns {boolean}
	 */
	function testExpr( expr, filepath ) {
		expr.lastIndex = 0;
		return expr.test( filepath );
	}

	/**
	 * @param {string} filepath
	 * @param {string} path
	 * @returns {boolean}
	 */
	function pathStartsWith( filepath, path ) {
		return path === "/" || filepath === path || filepath.startsWith( path + "/" );
	}

	/**
	 * @param {UExpressHandler} cb
	 * @param {UExpressRequest} req
	 * @param {UExpressResponse} res
	 * @returns {boolean}
	 */
	function runHandler( cb, req, res ) {
		let runNext = false;
		try {
			const handled = cb( req, res, ()=>{ runNext = true; } );
			if( handled && "function" === typeof handled.then )
				handled.catch( (err)=>{
					console.log( "Handler exception:", err.message, err.stack );
					if( res && !res.headersSent ) {
						res.writeHead( 500 );
						res.end( "Internal Server Error" );
					}
				} );
			return !runNext;
		} catch( err ) {
			console.log( "Handler exception:", err.message, err.stack );
			if( res && !res.headersSent ) {
				res.writeHead( 500 );
				res.end( "Internal Server Error" );
			}
			return true;
		}
	}

	/**
	 * @param {Array<{ path: string, cb: UExpressHandler }>} map
	 * @param {string} filepath
	 * @param {UExpressRequest} req
	 * @param {UExpressResponse} res
	 * @param {boolean} prefix
	 * @returns {boolean}
	 */
	function runMap( map, filepath, req, res, prefix ) {
		for( let mapping of map ) {
			if( prefix ? pathStartsWith( filepath, mapping.path ) : mapping.path === filepath ) {
				if( runHandler( mapping.cb, req, res ) )
					return true;
			}
		}
		return false;
	}

	/**
	 * @param {Array<{ expr: RegExp, cb: UExpressHandler }>} map
	 * @param {string} filepath
	 * @param {UExpressRequest} req
	 * @param {UExpressResponse} res
	 * @returns {boolean}
	 */
	function runExprMap( map, filepath, req, res ) {
		for( let mapping of map ) {
			if( testExpr( mapping.expr, filepath ) ) {
				if( runHandler( mapping.cb, req, res ) )
					return true;
			}
		}
		return false;
	}
        
	return {
		use(a,b ) {
			addUseMapping( a, b );
		},
		all(a,b ) {
			addMapping( "ALL", a, b );
		},
		get( a, b ) {
			addMapping( "GET", a, b );
		},
		post( a, b ) {
			addMapping( "POST", a, b );
		},
		handle( req, res) {
			if( typeof req.url === "undefined" ) {
				console.log( "Bad request:", req );
				return false;
			}
			if( req.url === null ){
				console.log( "Bad request:", req );
				return false;
			}
			const parts = req.url.split("?");
			const url = decodeRequestURI(parts[0]);
			const filepath = url;//path.dirname(url)+((path.dirname(url)&&path.basename(url))?"/":"")+path.basename(url);
			req.path = filepath;
			req.query = new URLSearchParams( parts.slice( 1 ).join( "?" ) );
                        
			let cb;
			const method = getMethod( req );
			const methodMappings = mappings[method] || new Map();
			const methodReqMaps = req_maps[method] || [];
			if( runMap( use_mappings, filepath, req, res, true ) )
				return true;
			if( runExprMap( use_req_maps, filepath, req, res ) )
				return true;
			if( runExprMap( req_maps.ALL, filepath, req, res ) )
				return true;
			if( runExprMap( methodReqMaps, filepath, req, res ) )
				return true;

			if( cb = mappings.ALL.get( filepath ) ) {
				if( runHandler( cb, req, res ) )
					return true;
			}
			if( cb = methodMappings.get( filepath ) ) {
				//console.log( "got cb?" );
				if( runHandler( cb, req, res ) )
					return true;
			}
			
			return false;
		}
    }
}

export default uExpress;
