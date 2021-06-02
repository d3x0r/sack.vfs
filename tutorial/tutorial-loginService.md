

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


## ...

An object storage file called 'data.os' is created in the root of sack.vfs.
The login server expects `node_modules/@d3x0r` modules to be installed; hence the above `npm install` command.


## Usage

``` js

import {popups} from "popups"
import {connection} from "/node_modules/@d3x0r/popups/example/webSocketClient.js"

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



```

