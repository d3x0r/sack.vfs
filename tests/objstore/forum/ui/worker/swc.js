
import {JSOX} from "../../../../../node_modules/jsox/lib/jsox.mjs"
import {SaltyRNG} from "../../../../../node_modules/@d3x0r/srg/salty_random_generator.js"
const regenerator = SaltyRNG.Id;
const generator = SaltyRNG.Id;
const short_generator = generator;

const idGen = { generator:generator,regenerator:regenerator }

const workerInterface = {
	connect: connect,
	uiSocket:null,
	setUiLoader(protocol) {
		workerInterface.uiSocket = protocol;                    
		l.worker.postMessage( {op:"setUiLoader", socket:protocol.socket} );
	},
	expect(url) {
            	// notify worker that another page will be
            	// loaded soon, and that this client socket is also
            	// used for that.
		l.worker.postMessage( {op:"expect", url:url } );
	},
        initWorker: initWorker
}

const l = {
	requestSocket : null,
	reg : null,
	worker : null,
	connect:false,
	connects:[],
	opens:[],
	sockets :new Map(),

};

const config = {run:{ devkey:localStorage.getItem( "devkey" ),
		clientKey : localStorage.getItem( "clientKey" ),
		sessionKey : localStorage.getItem( "sessionKey" )
			} };


const localStorage_ = {
	setItem(item,val) {
		localStorage.setItem( item, val );
		config.run[item] = val;
	},
	getItem(item) {
		return localStorage.getItem( item );
	},
	removeItem(item) {
		config.run[item] = null;
		localStorage.removeItem(item);
	}
}

function initWorker() {

navigator.serviceWorker.register('/sw.mjs', { scope: '/' }).then(function(registration) {
      // Registration was successful
      //console.log('ServiceWorker registration successful with scope: ', registration.scope, registration);
	l.reg = registration;
	//console.log( "got registration", registration );

	function tick() {
            	if( !l.worker ) {
	  	      l.worker = l.reg.active;
			if( l.worker ) {
				l.worker.postMessage( {op:"Hello" } );
                	
				if( l.connect )
					l.worker.postMessage( {op:"connect" } );
			} else {
				setTimeout( tick, 100 );
			}
                }
	}
	tick();
    }, function(err) {
      // registration failed :(
      console.log('ServiceWorker registration failed: ', err);
    });

navigator.serviceWorker.ready.then( registration => {
	//console.log( "THIS IS READY?" );
	l.reg = registration;
        if( !l.worker ) {
		l.worker = l.reg.active;
		//console.log( "got registration ready", registration );
		if( l.worker )
			l.worker.postMessage( {op:"Hello" } );
		if( l.connect )
			l.worker.postMessage( {op:"connect" } );
        }
  });

navigator.serviceWorker.addEventListener( "message", handleMessage );

}

function makeSocket( sockid, from ) {

	const socket = {
		socket:sockid,
		setUiLoader() {
			workerInterface.setUiLoader( socket );
		},
		from:from,
		cb : null,
                events_ : [],
		newSocket(addr) {
			return new Promise( (res,rej)=>{
				l.opens.push( res );				
				l.worker.postMessage( addr );
			} );
		},
                on(event,cb ) {
			if( "function" !== typeof cb ) {
                        	if( event in this.events_ )
					this.events_[event](cb);
			} else {
				this.events_[event] = cb;
			}
		},
		send(a) {
			//console.log( "Send something?",a );
			l.worker.postMessage( {op:"send", id:socket.socket, msg:a } );
		},
		handleMessage(msg ) {
			console.log( "this message", msg ) ;
			if( msg.op === "addMethod" ) {
                                try {
					const f = new Function( "JSON", "config", "localStorage", "idGen", msg.code );
					f.call( socket, JSOX, config, localStorage_, idGen );
		                
					if( "setEventCallback" in socket )
						socket.setEventCallback( socket.on.bind( socket, "event" ) );
					
					socket.on( "connect", socket );
				} catch( err ) {
					console.log( "Function compilation error:", err,"\n", msg.code );
				}		
				
			} else if( msg.op === "status" ) {
				if( socket.cb )
				    socket.cb( msg.status );
				else
                                    console.log( "Socket doesn't have a event cb? Status:", msg.status );
			} else if( msg.op === "disconnect" ) {
	                        const socket = l.sockets.get( msg.id );
        	                l.sockets.delete( msg.id );
                	        socket.on("disconnect");
			} else {
				if( socket.fw_message )
					if( socket.fw_message( socket, msg ) ) return;
				if( msg.op === "get" ) {
					console.log( "No service handler for get... passing back to default handler." );
					if( workerInterface.uiSocket )
						workerInterface.uiSocket.send( msg );
					//l.worker.postMessage( msg );
					return;
				}

				//socket.cb( msg );
				console.log( "Recevied unknown network event:", msg );
			}
		}
	};

	return socket;
}


function handleMessage( event ) {
	const msg = event.data;
        console.log( "msg:", msg );
	if( msg.op === "a" ) {		
		const sock = l.sockets.get( msg.id );
		if( sock ) {
			if( msg.msg.op === "connected" ) {
				if( sock.cb )
					sock.cb( true, sock );
				else console.log( "Lost status callback for socket?" );
			} else {
				sock.handleMessage( msg.msg );
			}
		}
	} else {
		//console.log( "worker Event", msg );
		if( msg.op === "getItem" ) {
			event.source.postMessage( {op:"getItem", val:localStorage_.getItem( msg.key )} );
		} else if( msg.op === "setItem" ) {
			localStorage_.setItem( msg.key, msg.val );
		} else if( msg.op === "connecting" ) {
			if( l.opens.length ) {
				const sock = makeSocket( msg.id );
				l.sockets.set( msg.id, sock );
				return l.opens.shift()(sock);
			}
			const sockfrom = l.sockets.get( msg.from );
			const sock = makeSocket(msg.id,msg.from );
			if( sockfrom ) {
				sock.cb = sockfrom.cb
			} else
				sock.cb = l.connects.shift();
			l.sockets.set( msg.id, sock );
		} else if( msg.op === "get" ) {
			l.worker.postMessage( msg );
 	       } else if( msg.op === "disconnect" ) {
                    const sock = l.sockets.get(msg.id );
                    	sock.handleMessage( msg );
		} else {
			console.log( "Unhandled Message:", msg );
		}
	}
}

function connect( cb ) {
	l.connects.push(cb);
	if( !l.worker ) 
		l.connect = true;
	else
		l.worker.postMessage( {op:"connect"} ); 	
}


export {workerInterface }
