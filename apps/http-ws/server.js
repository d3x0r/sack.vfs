const server = require( "./server.mjs" );

module.exports = {
	...server,
	open: server.openServer,
};
