

# Background Service Worker

- sw.js : is a service worker that handles fetch and websocket connections.  It can use connections to perform the request.
- swc.js : is the loader side of the script, which provides the ability to create a websocket, and specify that websocket for loading resources.

Resources are potentially loaded from cache across a server.

This also allows server side content to be built and shipped to the client on request, without the overhead of negotiating a new connection.

If not found, the request will be forwarded to conventional lookup methods.
