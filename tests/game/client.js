
var sack = require( "../.." );
var JSON = sack.JSON6;
//console.log( "got sack:", sack );

var f = sack.PSI.Frame( "test", -1, -1, 600, 600 );

console.log( "sack.Image.Color", Object.getPrototypeOf(sack.Image.colors ));
var background = sack.Image.colors.blue;
console.log( "back:", background );

var customControl = sack.PSI.Registration( { 
	name: "image control",
	width: 256,
	height : 256,
	border : sack.PSI.Frame.Border.bump,
	create() {
		return true;  // can return false/0 to disallow control creation.
	},
	draw( image ) {
		image.fill( background );
		//image.fill( sack.Image.Color.black );
		//console.log( "draw..." );
		players.forEach( (player)=>{
		        image.fill( player.x, player.y, 5, 5, player.color );
		} )

	        image.fill( player.x, player.y, 5, 5, player.color );
		return true;
	},
	mouse( event ) {
		//console.log( "Mouse Event:", x_del, y_del, event.x, event.y, event.b );
	        if( ws ) {
			player.x = event.x;
			player.y = event.y;
			board.redraw();
        		ws.send( `{x:${event.x},y:${event.y}}` );
	        }
		if( event.b & sack.PSI.button.scroll_up ) { 
			scale *= 1.1;
			this.redraw();
		} else if( event.b & sack.PSI.button.scroll_down ) { 
			scale *= 0.9;
			this.redraw();
		} else if( event.b & sack.PSI.button.left ) {
			if( !( _b & sack.PSI.button.left ) ) {
				// first down;
				x_click = event.x;
				y_click = event.y;
			} else { 
				//x_del += ( event.x - _x );
				//y_del += ( event.y - _y );
				//this.redraw();
			}
			_x = event.x;
			_y = event.y;
			_b = event.b;
		} else {
			_b = event.b;
		}
		return true;
	}
} );

var board = f.Control( "image control", 0, 40, 500, 500 );

f.show();


var players = [  ];
var player = { x : 50, y : 50, color : sack.Image.colors.white };
var x, y;
//var background = sack.Image( "the rror.jpg" );
//console.log( "background=", r );

/*

var r = sack.Renderer( "test", 10, 10, 500, 500 );
console.log( "renderer=", r );

r.setDraw( ( image )=>{	
	console.log( "Needs to be drawn..." );
	players.forEach( (player)=>{
	        image.fill( player.x, player.y, 5, 5, player.color );
	} )
} );


r.setMouse( ( event )=> {
	console.log( "mouse event : ", event.x, event.y, event.b );
        if( ws ) {
        	ws.send( `{x:${event.x},y:${event.y}}` );
        }
} );

r.setKey( ( key )=> {
	console.log( "key event : ", key.toString(16) );
} );


r.show();

*/

var server = `ws://${process.argv[2]?process.argv[2]:"127.0.0.1:8080"}/`;
console.log( "connect to ", server );

function open() {
	var client = sack.WebSocket.Client( server, ["game"], { perMessageDeflate: false } );
	client.on( "open", function ()  {
		//console.log( "Connected: ", this.connection );
		//console.log( "ws: ", this );
		this.on( "message", function( msg ) {
			msg = JSON.parse( msg );
			if( "id" in msg ) {
				var player = players[msg.id];
				if( !player ) 
					players[msg.id] = { x : msg.x, y : msg.y, color : sack.Image.colors.red };
				else {
					player.x = msg.x; player.y = msg.y;
				}
			}
			board.redraw();
                	//this.close();
        	} );

		this.on( "close", function( msg ) {
        		console.log( "opened connection closed" );
        	        //setTimeout( ()=> {console.log( "waited" )}, 3000 )
	        } );
		//client.send( "Connected!" );
		//client.send( msg );
	} );

	client.on( "close", function( msg ) {
      		console.log( "unopened connection closed" );
	} );
	return {
        	send(buf) {
                	client.send( buf );
                },
        }
} 

var ws = open();

