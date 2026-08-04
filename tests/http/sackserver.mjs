import { sack } from "sack.vfs";
const PORT = Number(process.env.PORT)||8081;
let served = 0;
const s = new sack.WebSocket.Server( { port: PORT } );
s.onrequest = ( req, res ) => {
	served++;
	console.log( `served #${served} ${req.url}` );
	const body = `ok-${req.url}`;
	res.writeHead( 200, { "Content-Type":"text/plain" } );
	res.end( body );
};
console.log( `sack server on ${PORT}` );
