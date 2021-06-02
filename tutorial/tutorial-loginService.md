

# Login Service

This is a login service that combines client side display library [Popups](https://github.com/d3x0r/popups) and provides a websocket connection to a user database.
It creates accounts, verifies login information, sets up a unique tracking ID for that client.

Login supports 

- Guest Login : Provide only a display name for others to see, an account is created internally, and the webpage localStorage saves values to continue logging in to the same guest account.  TODO: Guest accounts should automatically delete after a period of inactivity.
- Login : ask for an account name and password to verify the user.  The user must already exist.
- Create : Provides a user display name, account name, and optional email that might be used to recover/reset password information.  Password is double authenticated, no specific requirements are imposed on a password.

## Setup

``` bash
git clone https://github.com/d3x0r/sack.vfs
cd sack.vfs
npm install .
npm run login-server
```
Opens a server by default on port 8089.

## Test 1 

Connect with a browser(chrome) to `http://localhost:8089/` [URL](http://localhost:8089/).  It will redirect to `node_modules/@d3x0r/popups/example/index-login.html#ws`.


## ...

An object storage file called 'data.os' is created in the root of sack.vfs.
The login server expects `node_modules/@d3x0r` modules to be installed; hence the above `npm install` command.


## Usage

``` js

import {popups} from "popups"
import {connection,openSocket} from "/node_modules/@d3x0r/popups/example/webSocketClient.js"

connection.loginForm = loginForm = {
     connect() {
	// login is ready
     },
     disconnect() {
	// login is not ready
     },
     login() {
	// login is success
     }


const loginForm = popups.makeLoginForm( ()=>{
	/* login success... do something */
} );


openSocket( "localhost:8089" ); // connect to server; default address is location.origin.

```


---

### Internals

| Source | Description |
|---|----|
| `tests/objstore/userDb/userDbServer.mjs`  | This is the script that gets run with `npm run login-server`; as `node tests/objstore/userDb/userDbServer.mjs`. |
| `tests/objstore/userDb/userDb.mjs` | provides database classes that interface between storage and application code |
| `tests/objstore/userDb/userDbMethods.js`  | This is the internals of an ` async function( JSON ) { /*this fragment goes here*/ } ` which is executed with the client websocket as the 'this'.  The socket is mutated with functions to interface with user database across socket. |

### Db Methods

The current method fragment requests a unique client ID from the login server; which is saved in `localStorage( 'clientId' );`

| Provided Method | args | Description |
|-----|----|-----|
| doLogin | (user,password) | send a login request for the provided username and password; function hashes the password before being sent. |
| doCreate | (display,user,password,email) | Send a create account for the provided information. Username is used to login, while display is what is shown to others.  Related services will get a unique userID and the display name when requested. |
| doGuest | (display) | Create a temporary account which only provides a display name, and fills in random identifiers for the rest. |


This is used by the `node_modules/@d3x0r/popups/` `example/webSocketClient.mjs` to handle messages incoming from the server.

| Internal Methods | Description |
|-----|-----|
| processMessage(ws,msg) | process an incoming message; the message is expected to be an object, and the websocket the request came in on. Routine can optionally return true to prevent default handling.  Default handling updates the popup form. |



### Popups

| Source | Description |
|---|----|
| `node_modules/@d3x0r/popups/popups.mjs` | This is the JS code library to create DOM elements, giving them a desktop application feel. |
| `node_modules/@d3x0r/popups/example/loginForm.html` | This is a HTML fragment that is loaded into the login form. |
| `node_modules/@d3x0r/popups/example/webSocketClient.js` | This provides the bindings betweenJS and the loginForm.HTML by, searching by ID within each form. |

