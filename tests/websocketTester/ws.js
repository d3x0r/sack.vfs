

var connectedTotal = 0;
var connections = [];
function doConnect() {
if ("WebSocket" in window) {
  var ws = new WebSocket("ws://"+location.host+"/appServer", "flashboard");
  
  ws.onopen = function() {
    // Web Socket is connected. You can send data by send() method.
    //ws.send("message to send"); 
		if( connectedTotal < 200 ) {
			console.log( "total:", connectedTotal++ );
			//for(var n = 0; n < 10; n++ )
			//doConnect();
		}
		connections.push( ws );
    ws.send( JSON.stringify( { MsgID: "flashboard" } ) );
  };
  ws.onmessage = function (evt) { 
  	var received_msg = evt.data; 
        //console.log( "received", received_msg );
	ws.close();
  };
  ws.onclose = function() { 
	var conn = connections.findIndex( c=>c === ws );
	if( conn >= 0 ) {
	  	//console.log( "Socket got closed...", connectedTotal-- );
		connections.splice( conn, 1 );
	}
	doConnect();
  	// websocket is closed. 
  };
} else {
  // the browser doesn't support WebSocket.
}

}

doConnect();
doConnect();
doConnect();
doConnect();
doConnect();
doConnect();
