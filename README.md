# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) 

[![buddy pipeline](https://app.buddy.works/d3ck0r-1/sack-vfs/pipelines/pipeline/492336/badge.svg?token=60065f4e65bb4060abba4ed78e886e269a135fdbc7f1ae03ccc0e79af3cffe76 "buddy pipeline")](https://app.buddy.works/d3ck0r-1/sack-vfs/pipelines/pipeline/492336)

(Deprecating; no free support - one shot 10k credits then 0) [![Build Status](https://travis-ci.com/d3x0r/sack.vfs.svg?branch=master)](https://app.travis-ci.com/github/d3x0r/sack.vfs)


- WebSocket/HTTP/HTTPS network library.  UDP sockets (`http`,`UDP` replacment).
- JSON/[JSON6](https://github.com/d3x0r/JSON6) (stream)parser,
- [JSOX](https://github.com/d3x0r/JSOX) (streaming) parser,
- Node addon for a custom virtual and physical file system interface (fs replacement).
- USB keyboard raw input (windows)
- Graph Database and Object Storage.
- Primitive Natural Language tokenizer, and attributed text type.
- OpenSSL/LibreSSL Certificate Generation (https, wss).
- Salty Random Generator - procedural random content generator.
- Simple Play a Sound. (windows, but can port to openAl and cross platform)
- Process control; launch and control tasks launched.
- Sqlite/ODBC database driver; minimal SQL interface, `open` and `do`.
- Option/Configuration Database support using sqlite/odbc interface; stores in a heirarchy of options per program and global.
- Monitor filesystem for changes to files (caching server update).
- Configuration file parser/log analyzer based on word token matches.
- opt-out per-worker access to tasks and filesystem access.


## Requirements

### All Systems
  * Node.
  * `npm install -g node-gyp`
  * a working build environment
     * (win32) `npm install -g windows-build-tools` - if these are not initially installed, it will require a system restart, without any visible notice; but the install will not complete until restarted.
     * ninja, gcc 
  * (opt) `npm install -g cmake-js` and `cmake`.


### ODBC and uuid support. (can be disabled)

ODBC connectivity is provided as an alternative to the sqlite interface, providing connectivity with all existing databases.  It is not required; Sqlite itself is not 'required' but special steps have to be taken to disable it.  When enabled, the [SACK](https://github.com/d3x0r/sack) library uses it to configure some runtime options.  Options may be controlled by an 'interface.conf' configuration file.

#### Various Linux

 *  yum install gcc-c++ libuuid-devel unixodbc-devel
    * (uuid/uuid.h sql.h)
 *  apt-get install uuid-dev unixodbc-dev 
    * (uuid/uuid.h sql.h)
 *  pacman -S unixodbc util-linux
    * (sql.h uuid/uuid.h(probably already available, fs2util) )
 *  (?)emerge unixodbc

#### Mac

  *  (ODBC might be optioned out; just uses sqlite typically)
  *  brew (brew install unixODBC)

#### Windows

  * None additional


# Usage

``` js
const sack = require( 'sack.vfs' );

```

``` mjs
import {SACK} from "sack.vfs";


```

## Overview of SACK Namespace Object


```
{
    ComPort(comport) - access to com ports.
     - A jsox (JavaScript Object eXchange) parser. (JSON5/6 input compatible)
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSOX stream; callback is called as each value is completed.
        stringifier() - create a reusable stringifier which can be used for custom types
        stringify(object,replacer,pretty) - stringify an object; same API as JSON.
    JSOX - A jsox parser. (JSON5 input compatible)
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSOX stream; callback is called as each value is completed.
        stringify(obj [,stringifer [,formatter])
    JSON6 - A json6 parser. (JSON5 input compatible)
        parse(string [,reviver]) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSON6 stream; callback is called as each value is completed.
        stringify(obj [,stringifer [,formatter])
    JSON - A json parser.
        parse(string) - result with a V8 object created from the json string.  
        begin( cb ) - begin parsing JSON stream; callback is called as each value is completed.
    Sqlite(dbName) - an interface to sqlite which can open databases in the volume.
            parameters - ( databaseName )
                databaseName maybe a simple ODBC name; if the name ends with .db, it is assumed to be 
                a sqlite database.  If ODBC is not available, it may be assumed that the name will 
                just be a sqlite database name. Extra syntax - if the name starts with a $, then the 
                next word after $ and before the next @ or $ is used to select a sqlite vfs name.
                ( https://sqlite.org/vfs.html )  
                if the name is 'sack' then after @ and before the next $ is specified a mounted filesystem name.
        Sqlite has methods available on it to access native program options.
        Sqlite.op( opName, defaultValue ) - read/write default option database option.
        Sqlite.so( opName, newValue ) - write new value to default option database.
    ObjectStorage( fileName [, version] ) - open a JS object storage database. 
    Volume([mountName,]fileName[,version]) - a virtual disk partition that holds files.
          mountName - the mount name used to mount to volume as a filesystem; it may be referenced 
                later in the string passed to Sqlite.  It may be `null` if it is anonymous mount.
          if no parameters are passed, a Volume object representing the native filesystem is returned.
    Volume.mkdir(pathname) - utility function to make directories which might not exist before volume does; 
            (Volume() auto creates directories now if specified path to filename does not exist)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
    Volume.readAsString( filename ) - memory map map a file from native filesystem as a utf8 string.
    Volume.mapFile( filename, loaded_callback ) - memory map map a file from native filesystem as an arrayBuffer.
            if loaded callback is specified, a thread is created that touches every page in the file, when it's done
            the callback is called with no parameters.
    File - some native filsystem utility methods(?)
    FileMonitor - receive event notifications when files change
    SaltyRNG(feed salt callback) - creates a random number generator
    TLS - namespace for utilities to generate certificates/keys
        genkey( length [,password]) - Generates a keypair
        pubkey( {options} ) - gets public key of a keypair or certificate
        gencert( { options } ) - Generates a self signed certificate.
        genreq( {options} ) - Generates a certification request
        signcert( {options} ) - uses a certificate to sign a certificate request
        validate( {options} ) - validate a certificate against a certificate chain.
        expiration( certificate ) - gets the expiration of a certificate as a Date().     
    WebSocket - Websocket interface
        Client( ... ) - create a websocket client
        Server( ... ) - create a websocket server        
        Thread - Helper functions to transport accepted clients to other threads.
            post( accepted_socket, unique_destination_string ) - posts a socket to another thread
            accept( unique_destination_string, ( newSocket )=>{} ) - receives posted accepted sockets
    HTTP - HTTP Request Method
        get( {options} ) - synchronous http request
    HTTPS - HTTPS Request Method
        get( {options} ) - synchronous https request
    Network - Raw network utilities
        Address( address [,port] ) - holder for network addresses.
        UDP( ... ) - UDP Socket to send/received datagrams.
    Config - Process configuration files; also streaming text parsing
        (methods)[#Config_Methods]
    ComPort - (see below)
    
    log(string) - log a string.
    memDump() - log memory stats (track module memory leaks)
    mkdir() - make a directory in the current path; handles recursive directory creation.
    u8xor(s,k) - utility function to apply a string mask.
    b64xor(s,k) - utility function to just xor a value into a base64 string without expanding the values.
    id() - generate a unique ID (256 bits, 32 bytes, 44chars, trailing '=').
    Id() - generate a short unique ID (12 bytes, 16chars).
    loadComplete() - Indicate to SACK system that this is completed loading (task summoner support;linux;deprecated)

    System - Namespace for SACK system interface routines (The above methods should be moved into this namespace)
	createMemory(name,byte size) - creates a named memory region; memory regions by name are shared on the system.
	openMemory(name) - opens an existing names region; returns an ArrayBuffer which can be mapped to a typed array by application.
	enableThreadFileSystem() - enables mounting file systems specifically for this thread
        allowSpawn() - returns task allowed state
        disallowSpawn() - disble task spawns for this thread
	dumpRegisteredNames() - dumps internal procedure/interface registry
        reboot() - on windows, trigger a reboot.
    Task(options) - an interface for createing and monitoring tasks.
        Task constructor takes an option object.
        end() - cause a task to exit..
        write() - send something to a task.
        send() - send something to a task.
        terminate() - terminate created task.
    // windows only
    registry - an interface to windows registry options
    	set( path, value ) - set a new value in the registry
        get( path )  - get a value from the registry.
    hid - raw keyboard interface, allows identification of different physical keyboard devices.
    
}	
```

## Interfaces


 - [Volume](https://github.com/d3x0r/sack.vfs/blob/master/README_Volume.md#volume-interface)
 - [SQLite](https://github.com/d3x0r/sack.vfs/blob/master/README_Sqlite.md#sqlite-interface)
 - [Object Storage](https://github.com/d3x0r/sack.vfs/blob/master/README_ObjectStorage.md#object-storage)
 - [JSON/JSON6/JSOX](https://github.com/d3x0r/sack.vfs/blob/master/README_JSOX.md#jsox-and-json-json6----json-and-json6-compatible-processor)
 - [Websocket Server](https://github.com/d3x0r/sack.vfs/blob/master/README_WSS.md#websocket-server-interface)
    - [HTTP Server](https://github.com/d3x0r/sack.vfs/blob/master/README_WSS.md#http-fallback)
 - [Websocket Client](https://github.com/d3x0r/sack.vfs/blob/master/README_WSC.md#http-request-interface--httphttps-)
 - [HTTP Requests](https://github.com/d3x0r/sack.vfs/blob/master/README_HTTP.md)
 - [UDP](https://github.com/d3x0r/sack.vfs/blob/master/README_UDP.md##udp-socket-object-networkudp) - Low level system UDP sockets; TCP could be available, but HTTP and Websocket suffice.
 - [TLS](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md)
 - [ComPort](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md)
 - [File Changes](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md)
 - [Misc...](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md)

### GUI Interfaces

  These are available when building the GUI version.  
  
 - [PSI](https://github.com/d3x0r/sack.vfs/blob/master/README_PSI.md); Image, Image.Color, Render, Intershell, Frame, Registration.



## Quick App Helpers

 - [Http/Websocket](apps/http-ws) - simple script to handle basic resource requests, and provide a websocket endpoint interface.  User supplies `accept()` and `connect()` callbacks.  Also includes basic express-like interface addon.

 - PRNG - Small seedable RNGs.
 - Events - Small event subscription/dispatch class
 - dbUtil - database utilities to read schema from database; builds JS objects representing table structures which can be queried for structure.
 - summoner - (experimental/example) this launches and tracks other tasks.  
 - task-manager - Runs tasks, provides a simple UI which can get the tasks' logs and control run state of tasks.

 
## Node JS Loader Support

Node loader hooks for [`.json6`](https://github.com/d3x0r/json6) and [`.jsox`](https://github.com/d3x0r/JSOX) file types.
This loader does an import of `sack.vfs` and sets `globalThis.SACK` with the result; and then also assigns 
`globalThis.JSOX` and `globalThis.JSON6`.  

The loader hooks are loaded with the option

```
    --experimental-loader=node_modules/sack.vfs/import.mjs
```

Previously support for `.json6` and `.jsox` were only provided for `require()`.

Loading either JS version of [JSON6](https://github.com/d3x0r/json6) or [JSOX](https://github.com/d3x0r/jsox) should be done after this, allowing them to replace the globalThis versions.
The sack.vfs version is still available via `SACK.JSOX` or `SACK.JSON6`.

Performance-wise the JS versions have advantages if the information to parse is sourced in JS, while the SACK versions
can operate directly on the array buffers loaded from SACK databases, Volumes and files.  Which limits the copy one side; otherwise
there's a conversion to string from binary and a copy of that string from JS to C potentially.

Loader also supports fetching files over `HTTP://` and `HTTPS://` (or `http://` and `https://`).  This performs a request, and then sends the result as the loaded module.

As a further feature, since web files often just use `.js` as the extension for modules, and everything loaded from a `type="module"` script is loaded as a module,
the loader supports loading from `module://./` (the '.' is important, since really the host is always this computer, and becomes part of the requests when a module loads another module).
The `module://./` prefix is substituted, and uses environment variables `RESOURCE_PATH` for where to load default files, if the file path starts with `/common` then `COMMON_PATH` environment
variable is prepended to the file, and `/node_modules` file prefix uses `NODE_MODULE_PATH` environment variable to load modules; native modules will probably not load this way, but typical 
javascript modules can be loaded this way.

There is a race condition, if `http(s)://` is used from the local server, then the loading might stall waiting for the server to become available; this is the reason
that `module://` support was added.  TODO: Fix stall, workaround, use `module://` instead of `http(s)://` for local files loaded from a self contained server.


---

## Changelog
- 1.1.822(in progress)
    - JSOX decode fixes.  Handle `'':` as not an error.  which also means `{:` is not an error and it's a zero length string.
    - Enable System Tray interface on windows (linux doesn't have a standard API; and tool doesnt' exist)
    - Fix rare JSOX parsing ref follow where part of path is used and then forks to previous data.
    - Add client SSH module; using libssh2 for support.
    - Add ability to open remote listen channels.
    - Added hook between a channel and the SACK Websocket/HTTP server to a remote connection.
    - fixed some issues with async SQL queries done in a quick batch.
    - improved name lookup from address.
    - Added uv_unref() to async SQL event; allows application to close when code is done.
    - Added ability to resolve names; the websocket is initialized with options on whether to get MAC and or resolve IP to name.
    - Added local and remote MAC addresses to websocket connection for clients and accepted server sockets.
    - Applied some minor fixes to build under termux on android.
    - Added isRunning() method to tasks to return if a task is still running.
    - Added `retries` and `timeout` parameters to HTTP request option object.
    - Added `ready()` method to client-protocol utility class.
    - Updated `import.mjs` import registration for `JSOX`, `JSON6` for Node 20+.
    - Improved async `DB.run()` method for SQL databases; removed dead code from old `do()` only method.
    - Apply auto inherit of stdio, stdout, stderr handles from node under linux.
    - Added some tests for `Task()` objects; fixed some minor memory leaks.
    - Modified Task `send()` method to send direct buffer, without format support; previously behaved like printf format.
    - Handle task write/read during close better; may generate an EPIPE, which can be caught with `process.on( "EPIPE", ...)` in JS script.
- 1.1.821
    - Linux build fix; don't use windows admin option.
- 1.1.820
    - Fix ability to launch task as admin on windows.
    - Added some tests for windows tasks (testing admin).
    - Added some windows shell utility interfaces; set windows shell, add logout, enable/disable task manager.
    - Added some app utility scripts for syslog scanner.
    - Fix environment variable for windows '~/' path to USERPROFILE instead of HOMEPATH
- 1.1.819
    - published final parser fixes.
- 1.1.818.1-4
    - minor fixes for common language parser multi-rule matching
- 1.1.818
    - Fix fatal error with non-async SQL command that generated an error.
    - Fix race condition crash with new SQL open callback; the db object was deallocated before uv_close event happened.
    - Improved import.mjs experimental loader support; added `module://./` support for it.
    - Fixed SQL result error when no records returned for non-promised query (returned undefined instead of empty array).
    - Fix failure to generate callback opening Sqlite databases.
    - Fixed configuration parser; added syslog scanner to generate bans by IP.
- More in [CHANGELOG.md](CHANGELOG.md)