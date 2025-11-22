const _debug = false;
const _debug_dangling = false;
const _debug_output = _debug || false;
const _debug_object_convert = _debug || false;
const _debug_ll = false; // remote receive message logging.
const _debug_map = false;
const _debug_replace = false;


import {ObjectStore} from "./object-storage-data.mjs"


export class SocketStore extend ObjectStore {

	remotes = [];
	requests = new Map();
	//newStorage.remotes = [newStorage];


	get(opts){
   		return new Promise( (res,rej)=>{
				const msg = {op:"get", id:unique++, opts:opts, res:res,rej:rej };
				ws.send( msg );
				requests.set( msg.id, msg );
			} )
	}
	put(obj, rid,res,rej){


		for( var remote of this.remotes ) {
			// track a put for each remote.
			const msg = {op:"put", id:unique++, data:obj, rid:rid, res:res,rej:rej };
			this.requests.set( msg.id, msg );
			remote.send( msg );
		}

	}

	
	handleMessage( msg ) {
            _debug_ll && console.log( "Storage Remote Handler:", msg );
            try {
		if( msg.op === "GET" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
                                        //console.log( "Found request?", req );
                                        _debug_get && console.log( "result:", msg );
			const data = (msg.data && ( msg.data instanceof ArrayBuffer
                                      || Object.getPrototypeOf( msg.data ).constructor.name === "ArrayBuffer"

                                     ))?decoder.decode( msg.data )
                                            : msg.data;
                        		//if(
                        		
                        		_debug_get && console.log( "Resolving request?", req, data, decoder.decode(msg.data) );
					if( req ) req.res( data );
                                        return true;
				}
			if( msg.op === "Get" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
                                        _debug_get && console.log( "Rejecting get reply..", msg.err );
					if( req ) req.rej( msg.err );
                                        return true;
				}
			if( msg.op === "PUT" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
					if( req && req.res ) {
                                            // sometimes it's write-only?
                                            _debug_put && console.log( "Something",req );
                                            req.res( msg.r );
                                        }
                                        return true;
				}
			if( msg.op === "Put" ) {
					const req = requests.get( msg.id );
                                        requests.delete(msg.id);
					if( req ) {
                                            _debug_put && console.log( "Req:", req );
                                            req.rej( msg.err );
                                        }
                                        return true;
				}
                }catch(err){
	                console.log( "something message error:", err );
                }
                return false;
	}


}

