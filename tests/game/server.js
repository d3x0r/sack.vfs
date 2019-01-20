

var sack = require( "../.." );
var JSON = sack.JSON6;
var server = sack.WebSocket.Server( { port: 8080 } )

console.log( "serving on 8080", server );

var player_id = 0;
var players = [];

server.onrequest( function( req, res ) {

	var ip = ( req.headers && req.headers['x-forwarded-for'] ) ||
		 req.connection.remoteAddress ||
		 req.socket.remoteAddress ||
		 req.connection.socket.remoteAddress;
	//ws.clientAddress = ip;

	console.log( "Received request:", req );
	if( req.url.endsWith( ".html" ) || req.url == "/" ) {
		res.writeHead( 200 );
		res.end( "<HTML><BODY>Success.</BODY></HTML>" );
	} else {
		res.writeHead( 404 );
		res.end();
	}
} );

server.onaccept( function ( protocols, resource ) {
	console.log( "Connection received with : ", protocols, " path:", resource );
        if( process.argv[2] == "1" )
		this.reject();
        else
		this.accept();
		//this.accept( protocols );
} );

server.onconnect( function (ws) {
	//console.log( "Connect:", ws );
	var player = players.find( player=>player.unused );
	if( !player )
		player =  { ws : ws,
			id : player_id++,
			unused : false,
		};
		players.push( player );
	else {
		player.unused = false;
		player.ws = ws;
	}
	ws.onmessage( function( msg ) {
		var _msg = JSON.parse( msg );
		_msg.id = player.id;
		players.forEach( player=>{
			if( player.ws === ws || player.unused ) return;
			//console.log( "set player id:", player.id );
			setTimeout( ((_msg)=>()=>{
				player.ws.send(  _msg );
			})(JSON.stringify(_msg)), 400 );
		} );
        } );
	ws.onclose( function() {
		var player = players.find( player=>ws === player.ws );
		if( player ) 
			player.unused = true;
        	console.log( "Remote closed" );
        } );
} );
