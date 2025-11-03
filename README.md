# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) 

[![buddy pipeline](https://app.buddy.works/d3ck0r-1/sack-vfs/pipelines/pipeline/492336/badge.svg?token=60065f4e65bb4060abba4ed78e886e269a135fdbc7f1ae03ccc0e79af3cffe76 "buddy pipeline")](https://app.buddy.works/d3ck0r-1/sack-vfs/pipelines/pipeline/492336)


Quick summary of features/subsystems.

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
    JSOX - A jsox parser. (JSON5/6 input compatible)
        parse(string) - result with a V8 object created from the json string.  
        stringify(obj [,stringifer [,formatter])
        begin( cb ) - begin parsing JSOX stream; callback is called as each value is completed.
        stringifier() - create a reusable stringifier which can be used for custom types
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


    log(string) - log a string.
    memDump() - log memory stats (track module memory leaks)
    mkdir() - make a directory in the current path; handles recursive directory creation.
    u8xor(s,k) - utility function to apply a string mask.
    b64xor(s,k) - utility function to just xor a value into a base64 string without expanding the values.
    id() - generate a unique ID (256 bits, 32 bytes, 44chars, trailing '=').
    Id() - generate a short unique ID (12 bytes, 16chars).
    loadComplete() - Indicate to SACK system that this is completed loading (task summoner support;linux;deprecated)

    /* if( isolated )... */
    setTimeout(fn,delay ) - same as JS function of same name (on sack object)  (returns a number ID)
    setInterval(fn,delay) - same as JS function of same name  (returns a number ID)
    clearTimeout( timeout_id )    
    clearInterval( timeout_id )
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
 - [UDP](https://github.com/d3x0r/sack.vfs/blob/master/README_UDP.md) - Low level system UDP sockets. 
 - [TCP](https://github.com/d3x0r/sack.vfs/blob/master/README_TCP.md) - Low level system TCP sockets.
 - [ICMP](https://github.com/d3x0r/sack.vfs/blob/master/README_UDP.md) - ICMP interface (name lookup and ping)
 - [TLS](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md) - certificate creation and validation
 - [Task](https://github.com/d3x0r/sack.vfs/blob/master/README_Task.md) - Process creation/control/information
 - [SSH](https://github.com/d3x0r/sack.vfs/blob/master/README_SSH.md) - SSH connection interface
 - [ComPort](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md) - COM port IO
 - [File Changes](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md) - Monitors files for changes
 - [Misc...](https://github.com/d3x0r/sack.vfs/blob/master/README_Misc.md) - Other

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
- 1.3.130(in progress)
- 1.3.129
   - Fix failure to build on *nix from updated COM port features.
- 1.3.128
   - Fix thread local initialization of file system interfaces.
- 1.3.127
   - fix potential internal error checking for directory existance.
- 1.3.126
   - Add method to expand a path name with symbolic character to full name.
- 1.3.125
   - Add get root filesystems call.
- 1.3.124
   - added slab-array and bloom-n-hash utilities
   - separated object-storage to a separate module to include (reduce required initial parsing)
   - Added monitor name to windows task getDisplay call.
   - Fixed network deadlock processing a request on socket that closes while in process.
   - Fixed potential network out-of-order read.
   - Added file creation and last written timestamps to result of vfs dir().
   - Added COM port ability to reset the logical port.
- 1.3.123
   - Apply actual fix for libressl clang windows build.
- 1.3.122
   - Add to windows WIFI interface to allow scripts to monitor, connect, and disconnect the wifi device given existing configurations.
   - Protect against null tick callback.
   - Improve windows system tray icon interface; allow changing the text of menu items after creating them, apply check mark to menu items, sort the items so the program name is on top and exit is last.
   - Add ability to set program name, which affects what the default system tray icon title is.
   - Improve windows keyboard/mouse interface to support multiple instances of the event handlers.
   - Improve COM port interface to get more information about the COM port.
   - Restore ability to open second listener for any address to listen on ::0 and 0.0.0.0.  Otherwise the listener will listen to only the specified address.
   - Updated libssh2, libressl.
   - Improve HTTPS hosting with multiple certificates.
- 1.3.121
   - Implements interface compatible with isolated-vm.
   - Adds some utility functions to support isolated-vm; setTimeout and setInterval for examples.
   - Fixes some small limits of set size in SACK threads; migrate small dynamic object THREAD_EVENT to a set managed object.
   - Fixes some client HTTP requesting.
- More in [CHANGELOG.md](CHANGELOG.md)
