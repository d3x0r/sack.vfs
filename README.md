# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)[![Build Status](https://travis-ci.com/d3x0r/sack.vfs.svg?branch=master)](https://travis-ci.com/d3x0r/sack.vfs)

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
  * Node, NPM.
  * `npm install -g node-gyp`
  * a working build environment
     * (win32) `npm install -g windows-build-tools` - if these are not initially installed, it will require a system restart, without any visible notice; but the install will not complete until restarted.
     * ninja, gcc 
  * (opt) `npm install -g cmake-js` and `cmake`.


### ODBC and uuid support. (can be disabled)

ODBC connectivity is provided as an alternative to the sqlite interface, providing connectivity
with all existing databases.  It is not required; Sqlite itself is not 'required' but special steps
have to be taken to disable it.  When enabled, the [SACK](https://github.com/d3x0r/sack) library uses
it to configure some runtime options.  Options may be controlled by an 'interface.conf' configuration file.

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


---

## Changelog
- 1.0.1013(in progress)
   - Split README.md into multiple files, so each section can be expanded.
   - add parse() command to JSOX parser instances.
   - add JSOX automatic encoding for `RegExp` as 'regex'.
   - handle badly formatted JSOX better; partial evaluations left states set that affected future valid expressions.
   - Fix setting 'undefined' results to fields.  (should be ignored)
   - fix onmessage, onopen, onclose, onerror websocket methods to be getter/setters, not function call based. (sorry; nearly warrants 1.1; though, it's also just broken so.).
   - Added some test services for a standlone checkout.
- 1.0.1012
   - Fix small leak closing com ports.
   - Fix order object fields are revived; assign object after it has been built; use internal context stack for references.
   - Fix failure to send final revive for an object containing an array.
- 1.0.1011
	- Websocket server connection instances didn't complete opening correctly (regression in 1.0.1010). 
- 1.0.1010
   - fix base64 typed array encoding (regression in 1009).
   - forgive double-posting a websocket socket.
   - lock first websocket send until header can be sent.
- 1.0.1009
   - Added node support for `--experimental-loader=node_modules/sack.vfs/import.mjs`.
   - Removed noisy debug(?)
- 1.0.1008
   - Object filesystem reprocessed journal; flush journal root block when cleaned.
   - Handle exceptions thrown by callbacks provided to JSOX parser better.
   - Fix a potential segfault while parsing invalid data.
   - update links to travis-ci.com instead of travis-ci.org.
- 1.0.1007
   - CRITICAL - fix reversion failure in base64 and interpreting JSOX unquoted typed-array values.
   - fix reversion at quote after string before ':' in JSOX.
- 1.0.1006
   - added remove() method for object storage directories.
   - getRemoteFragment() get code fragment for this API to connect a remote.
   - Added idle flush for object storage directory updates.
   - Removed noisy debug logging.
- 1.0.1005
   - fixed several issues reviving custom data types with references and replacement operations.
   - Added newer tests from [JSOX](https://github.com/d3x0r/JSOX).
- 1.0.1004
   - fixed keyboard input; object GC caused fault.
   - fixed JSOX parsing and stringification issues; Emit `string string` not `stringstring` if the tag and string are both unquoted; fix object revival issues.
- 1.0.1001-1.0.1003
   - first 1.0 version.
   - Fixes for JSOX; improved object storage; updated GUI interface code.
- 0.9.161
   - Added thread local storage
   - Added control flag to disallow per-thread spawn permission.
   - Improved object storage system; implemented rollback journal.
   - Shutdown stability issues; new worker-thread support shutdown invokes a library shutdown, and weak references can still fire their callbacks.
   - control-c shutdown is done in an external thread, which inadvertantly self triggers thread reference creation while locking.
   - JSOX object revival improvements in the C library side/C++ object creation side.
- 0.9.160
   - Improve worker_thread support (missed some other static constructors)
   - Added .log() to allow log output outside of any JS.
   - Fix event dispatches to callbacks that create promises that need to resolve.
- 0.9.159
   - Implement websocket thread interface to post accepted sockets to other threads.
- 0.9.158
   - add test for typed array revival; fix typed array revival (JSOX).
- 0.9.157 
   - revert UTC change.
- 0.9.156
   - missed getFullYear->getUTCFullYear update.
- 0.9.155
   - normalize JSOX class defintiion keys for later comparison to objects.
   - removed node 7 travis integration; `#  - "7" (doesn't have Utf8Value with isolate, 12 doesn't have Utf8Value without isolate())`
   - added FileMonitor interface to get event changes when files change on the disk.   
   - fixed some issues with JSOX parsing ( tagged class in tagged class in map )
   - fix time precision in JSOX stringification (add ms), and was encoding Localtime but claiming UTC.
   - fix error in salty random generator partial bit streams.
   - Implement worker_threads support better; remove static global persistent things.
   - Updated parsers so '\xa0' is the same as '\x20'.  Most parsers are for human-readable code; JSOX, TextParse, BurstExx.
- 0.9.154
   - Continued applying deprecation fixes; republished as 154
- 0.9.153
   - Republish 0.9.152 with ignored source
- 0.9.152
   - Update to Node 12 (Requires usage of context, and MaybeLocal types)
   - Fixed id regnerator to match JS version in more cases.
   - Use pthread_mutex for waits.
   - Improve static global usage so no external memory allocation gets used. (/tmp/.shared*)
   - Removed now unused signal handler registrations.
- 0.9.151
   - SKipped due to error in tagging.
- 0.9.150
   - Fix regression handline NUL inline in JSOX parser (re fix partial codepoints received across buffer bounds)
   - Fix releasing the buffer too soon on HTTP fallback from HTTPS.
- 0.9.149
   - Fixed getting events from listening sockets before being fully setup.
- 0.9.148
   - Updated documentation to cover lowerror event and disableSSL.
   - Added ping() method call on sockets accepted by a server and sockets connected to a server.
   - Fixed lost event during SSL handshake.
   - Fixed linux network write lock issue.
   - Fix static initializing SQL.
- 0.9.147 
   - fixed some issues with JSOX.
   - Fixed occasional event loss for SSL connections.
   - Improved ODBC Data marshalling ability.
   - Fix lost task output from task module tasks.
   - Added Text() type.  Which is the internal rich text segment tracking.  (for output to GUI console) 
- 0.9.146
   - SACK Update; add XSWS 
   - Fixed stall issue with http server and lots of requests in series from browser.
   - Fixed http server issue getting data while processing another header.
- 0.9.145 
   - Add 'mv', 'rename' methods to Volume() instance.  
   - Split locks on ssl read/write.  
   - Fix VFS directory truncation issue (early EODMARK injection). 
   - TLS Certificate add Certificate Policy
   - Add JSOX Parser.
   - Add Object Storage Module.
   - Added support for paramter binding in Sqlite.
   - Improve data marshalling for Sqlite to JS and vice versa.
   - Network improvement for send and close edge conditions.
   - Implement parameter binding for sqlite and ODBC (less tested).
   - Fixed spurious HTTP request failures when the connection closed with writes pending.
   - Improves SRG module; allow configuring SRG instance with a specific entropy generator.
   - Fixed task output stall.
- 0.9.144 - Fix websocket receiving packets with multiple frames.
- 0.9.143 - Improve task interface.  Simplify com data buffer; it's now only valid during receive callback. Improve websocket server handling http requests; add a event callback when socket closes, after server HTTP to distinguish between incomplete(TLS error) connections. Sync SACK updates: improve SQL parsing/table-index generation, library load path for current and name as passed, event for http close, some protection against dereferencing null parameters.
- 0.9.142 - Fix node-gyp for windows build.
- 0.9.141 - Add callback event to trigger background thread preload memory mapped files.
- 0.9.140 - Fix bad test on opening file in VFS.
- 0.9.139 - Added onerror callback for websocket server connections.  Add low level windows keyboard interface module.  Added memory mapped file to arraybuffer.  Fix https init failure with certain combinations of options.
- 0.9.138 - Add alpha methods for generating signing identifiers.  Fixes some lost network close notivications.
- 0.9.137 - fix pathchr insensitive path comparison; update tls interface for newer openssl; allow opening volumes by mount name only.
- 0.9.136 - added log option to sql connections.
- 0.9.135 - Fix unmounted volumes to instead mount hidden.
- 0.9.134 - disable delay-load build option for node-gyp build.
- 0.9.133 - Isolate in Utf8Value is actually a very new thing.
- 0,9.132 - Make a pass to cleanup deprecated Utf8Value() usage.
- 0.9.131 - Fixes for Node 10 (missing export TLS_1_2...; backfix for < 10)
- 0.9.130 - Fixes for Node 10.
- 0.9.129 - (cont) merge linux changes.
- 0.9.128 - (cont) lost some changes; reapply on windows.
- 0.9.127 - (cont) Also implement respecting keep-alive on connection.
- 0.9.126 - (cont)
- 0.9.125 - Updated VESL Parsing(WIP); Fixed latency on windows server socket close.
- 0.9.124 - Fix segfault in latest node; resolve ToInteger deprication warnings.  
- 0.9.123 - Fix missing sources in binding.gyp build rules.
- 0.9.122(unpublished) - Add VESL parsing(in progress). Add Task interface.  Fix create table emitter.
- 0.9.121 - Fix missing close after all data queued to be sent was sent.
- 0.9.120 - Fix crash caused by closing socket during http request dispatch.
- 0.9.119 - Network Scheduling error on windows.
- 0.9.118 - Fix lock issue with SSL read/write.  Fix windows wait on short event count.  Fix VES unlink so files disappear even though still open.
- 0.9.117 - lock on wrong side. 
- 0.9.116 - Fix a windows network lock issue. Handle ArrayBuffer output for http response.
- 0.9.115 - Fix stray unlock.
- 0.9.114 - Fix leave critical section in release mode.
- 0.9.113 - Added optional version parameter for VFS. Sync sack sources... A lot of reformats; updates for split network lock.  Added Sqlite.procedure to define deterministic function.
- 0.9.112 - decode of unicode character escape had bad calculation.
- 0.9.111 - promote to more appropriate version.  If anyone else joins; this should go to 1.0.  Improve TLS error reporting and SQL result set ability.  Improve table parsing.
- 0.1.99324 Test and Update sqlite user defined functions (function/aggregate); improved data type retention.
- 0.1.99323 Fix mac ( got travis integration working for mac).
- 0.1.99322 Some fixes building on mac.  Added error accessor to sqlite object.  Added user functions and aggregates to sqlite interface.
Fixed windows registr access. Added interface to configscript parser.  Added analog interface for clock object (GUI build).  Lots of changes 
SACK core code for user functions, json parse fixes, DeleteFromSet parameters, optimize poplink, added nolock dequelink, added callback for websocket
data receive completion, 
- 0.1.99321 Fix some edge json parsing cases.  Fixed some network issues.  Allow NUL characters in json parsing.
- 0.1.99320 Fix json escape of strings containing NUL characters.  Add HTTP(s) request methods.
Improve/modify PSI control registration/creation a little.  Expose font select dialog, color picker dialog, 
Fix property accessors in websocket module.
- 0.1.99319 Fix older gcc crash from bad optimization.
- 0.1.99318 Fix udp readStrings option.
- 0.1.99317 Added optional GUI interfaces. (build with `npm run build-gui` or `npm run build-gui-debug`)
- 0.1.99316 Fix rekey.
- 0.1.99315 Add rekey method to volumes.
- 0.1.99314 Fixed duplicating address sent with UDP messages.  
Fix reading files from /sys/class/*.  Fix reading directory.  
Fix websocket protocol option before options object.  
Fix websocket connection sequence.  
Allocate new connection object during accept so protocol lookup can attach data to that object. 
Fix handling SSL certificates on websocket connections.  
Simplify sending/receiving UDP subnet broadcasts by simply enabling broadcast. Other small fixes.
- 0.1.99313 remove some debug logging.
- 0.1.99312 fixes building on mac a little.
- 0.1.99311 Fixes building on msvs (exception flag)
- 0.1.99310 Fix rebinding on linux to subnet broadcast address.
- 0.1.99309 Fix clearing some persistant handles so objects can be garbage collected. 
Fix SSL server issues.  Implement onrequest handling for websocket connections.  Improve websocket close reason handling.
Fix TLS certification validation.  Fix passing additional root certificate for validation.
- 0.1.99308 Improve network event handling; Allow more connections dynamically; fix some locking issues with critical sections.
- 0.1.99307 Implement interface to websocket library. Implement interface to UDP sockets.
- 0.1.99306 Move json reviver parameter handling internal.  Implement volume JSON stream reader interface.  Fix options to create/reopen an existing file.
- 0.1.99305 Fix handling exceptions triggered from callbacks. Fix missing truncate in more instances;  Sync sack filesystem updates; fix unlink return value; add pos() method to File.
- 0.1.99304 fix truncate on simple writes into a volume.
- 0.1.99302, 0.1.99303 improvements for sqlite interface.
- 0.1.99301 add SaltyRandomGenerator interface.
- 0.1.99300 fix random generator overflows.
- 0.1.99299 set default sql auto checkpoint to off.
- 0.1.99298 add preferGlobal to package.json.
- 0.1.99297 replace Persistent strings with Eternal, which work on nodev4.
- 0.1.99296 fix deprecated v8::Delete method to use Maybe version.
- 0.1.99295 fix build error on older versions of node.
- 0.1.99294 fix stream paring empty array elements; and a error stream parsing a break between a field label just at the colon.
