
# System interface module

Interface to the SACK System Library.  This provides some views into internal information, and methods specific to the SACK system abstraction.

| Method Name | paramters | Description |
|----|----|----|
|dumpRegisteredNames  | ()  | Diagnostic. Dump internal registered names to stdout. | 
|createMemory|  (name, size) |  create a named memory region which can be opend by other thread/processes |
|openMemory| ( name) | Open an existing memory region.  Will return null if the region does not already exist. |
|allowSpawn|  ()  | Return the status of whether spawning processes is allowed or not. |
|disallowSpawn|  ()  | Disable spawning processes. |
|enableThreadFileSystem|  () | Enable thread-local filesystem on this thread.  No filesystems will bemounted after this call |
|reboot|  (mode)  | (Windows Only) Reboot or shutdown the current system.  Mode can be 'reboot' or 'shutdown' or empty which defaults to reboot. |
|dumpMemory | (verbose,filename) | Dump C allocated memory.  `verbose` and `filename` are both optional.  `verbose` enables dumping each block. `filename` dumps the log to the specified file. |
|enableExitSignal | (cb) | (Windows Only) Enable a callback which can be called when process exit event is triggered.  This disables the automatic exit, so this function should invoke process.exit() (The event cannot be triggered again). |
|hideCursor | (timeout) | (Windows Only) Enables hiding the mouse cursor if it is stationary for an amount of time. Timeout parameter is in milliseconds |

Some examples of using some of the system interfaces.

``` js
import {sack} from "sack.vfs"
sack.system.dumpRegisteredNames();
sack.system.reboot( "reboot" );
sack.system.dumpMemory(); 
```



# File Monitor - provides event callbacks when directories in the file system change.\

This provides an interface to receive notifications when files are created, modified or deleted.

``` js

var sack = require( "sack.vfs" );
var monitor = sack.FileMonitor( <pathname>, idleDelay );
monitor.addFilter( "*.jpg", /* imageChanged */ (info)=>{
	// info.path, info.size, info.date, info.directory, info.created, info.deleted
	// if !created and !deleted, is just a change.
	// the first time a file is seen it will be sent as 'created' even if it existed
	// previously (?).
} );

```


## TLS Interface

Exposes OpenSSL library functions to create key pairs, cerficates, and certificate chains.

### Interface

``` js
  TLS={
     genkey( length [,password]) - Generates a keypair, returns PEM string.  Requires length parameter, allows optional password to be specified.
     pubkey( {options} ) - gets public key of a keypair or certificate
     gencert( { options } ) - Generates a self signed certificate.
     genreq( {options} ) - Generates a certification request
     signcert( {options} ) - uses a certificate to sign a certificate request
     validate( {options} ) - validate a certificate against a certificate chain.
     expiration( certificate ) - gets the expiration of a certificate as a Date().     
  }
```

 | pubkey options | |
|---|---|
| cert | a certificate to get a key from, will not be used if 'key' option is specified.  either this or key MUST be specified, but not both.|
| key | a keypair to get the public key from.  Either this or cert MUST be specified, but not both.|
| password | password for keypair if required.|

 | gencert options | |
|---|---|
| country | specified country of origin; must be a 2 character country specifier. |
| state | specifies state or province of origin; |
| locality | specifies city in state specified.    |
| org | specified organization for certificate       |
| orgUnit | specifies organizational unit.             |
| name | specifies common name, this is typically a domain name with possible wildcard at start. |
| key | private/public key to use                                                                  |
| password | if required for key                                                                     |
| serial | serial number for this certificate (is a number type).                                      |
| expire | number of days before this certificate expires (is a number type).                            |
| pubkey | override storing the public key of this certificate. (used to test certificate chain validity check; do not use) |
| issuer | override the issuer/subject identifier of this certificate. (used to test certificate chain validity check; do not use) |

| genreq options |  |
|---|---|
| country | specified country of origin; must be a 2 character country specifier. |
| state | specifies state or province of origin;   |
| locality | specifies city in state specified.  |
| org | specified organization for certificate |
| orgUnit | specifies organizational unit. |
| name | specifies common name, this is typically a domain name with possible wildcard at start. |
| subject | add fields to Subject Alternative Name...  This is an object containing arrays |
| | <TABLE><TR><TD> DNS </TD><TD> a single domain name or an array of domain name strings to add </TD></TR> <TR><TD> IP </TD><TD> a single IP address string or an array of IO addresses to add; does support IPv6 addresses.</TD></TR></TABLE>  |
| key | private/public key to use  |
| password | if required for key |


``` js
// some examples of subject options
TLS.genreq( { subject : { IP: "127.0.0.1" } } );
TLS.genreq( { subject : { IP: ["127.0.0.1","192.168.1.1"] } } );
TLS.genreq( { subject : { DNS: "www.example.com" } } );
TLS.genreq( { subject : { DNS: ["www.example.com", "smtp.example.com"], IP:[ "10.0.0.1" ] } } );
```



| signcert options |  |
|---|---|
| signer | certificate to sign with |
| request | certificate request to sign |
| expire | number of days this certificate is valid for |
| key | private key of the signer |
| password | password of the signer's key. |
| serial | serial number for this certificate (is a number type). |
| issuer | used to override issuer identifier. (used to test certificate chain validity check; do not use) |
| subject | used to override subject identifier. (used to test certificate chain validity check; do not use) |
| pubkey | used to override the public key stored in the output certificate. (used to test certificate chain validity check; do not use) |

| validate options |  |
|---|---|
| cert | certificate to validate | 
| chain | concatenated certificate chain to use for validation.  Order does not matter.  Not required if cert is self signed. |

See Also [tlsTest.js](https://github.com/d3x0r/sack.vfs/blob/master/tests/tlsTest.js) for example usage.

## SRG Module

Salty Random Generator 

```
  var sack = require( 'sack.vfs' );
  var SRG = sack.SaltyRG( salt=&gt;salt.push( new Date() ) )// callback is passed an array to add salt to.
  var number = SRG.getBits(); // get a signed 32 bit number -  -2,147,483,648  to 2,147,483,647
```

  - SaltyRG( callback ), SaltyRG( initial_seed ), SaltyRG()<BR>
     callback is called to request more salt.  An array is passed which can have values pushed into it.<BR>
     initial_seed is a seed to use initially, then no callback is called.<BR>
     no parameters results with a generator with no initial seed, and no callback<BR>
     Results with a random_generator object.
     
| SaltyRNG Instance Methods | Args | Description |
|-----|-----|-----|
| seed |( toString ) : set the next seed to be used; any value passed is converted to a string.|
| reset|() : reset the generator to base state.|
| getBits|( [bitCount [, signed]] ) : if no parameters are passed, a 32 bit signed integer is returned.  If bitCount    is passed, it must be less than 32 bits.  If signed is not passed or is false, the resulting value is the specified       number of bits as an unsigned integer, otherwise the top bit is sign extended, and a signed integer results. |
| getBuffer|( bits ) : returns an ArrayBuffer that is the specified number of bits long, rounded up to the next complete byte count.|
| setVersion| () | Sets Sign/Verify generator to SHA3.  (needs additional options someday) |
| sign | ( [pad1 [, pad2],] string | get a signing identifier for string |
| setSigningThreads | ( n ) | Set the number of threads to sign with to n |
| verify | ( buffer, key ) | returns true/false if key is a signing key of buffer |
    

| SaltRNG Methods | Args | Description |
|----|-----|-----|
| sign | ( [pad1 [, pad2],] string | get a signing identifier for string |
| setSigningThreads | ( n ) | Set the number of threads to sign with to n |
| verify | ( buffer, key ) | returns true/false if key is a signing key of buffer |
| id | ( [source [, version]] ) | Version indicates which entropy generator to use internally |

### id function

The `id()` function takes an optional parameter that is an integer.

| id version | use generator |
|---|---|
| 0 | sha1 |
| 1 | sha2 |
| 2 | sha2_256 |
| 3 | sha3 |
| 4 | K12 |
|default| K12 |

``` js
  // some examples
  import {sack} from "sack.vfs";
  var SRG = sack.SaltyRG( salt=&gt;salt.push( new Date() ) )// callback is passed an array to add salt to.
  
  var signed_byte = SRG.getBits( 8, true );
  var unsigned_byte = SRG.getBits( 8 );
  var someValue = SRG.getBits( 13, true );  // get 13 bits of randomness, sign extended.
  var buffer = SRG.getBuffer( 1234 ); // get 155 bytes of random data (last byte only has 2 bits )

  // these return base64 encoded bytes.
 var randomId     = SRG.id();  // default generator
 var randomId_v4  = SRG.id(4);  // use version 4 hashing
 var regenId      = SRG.id("Hash This ID");  // default hash generator (NEEDS TESTING!)
 var regenId_v4   = SRG.id("Hash This ID", 4); // regenerate, using hash 4

```

## WebSocket Module

``` js
var sack = require( "sack.vfs" );
var client = new sack.WebSocket.Client( &lt;url&gt; [, &lt;protocol(s)&gt;] [,option object] );
client.on( &lt;event&gt;, &lt;callback&gt; );
client.onopen( callback );
client.onmessage( callback );
client.onerror( callback );
client.onclose( callback );

var server = new sack.WebSocket.Server( { &lt;options&gt; } );
server.on( &lt;event&gt;, &lt;callback&gt; );
server.onconnect( callback );
server.onaccept( callback );

```


## RegAccess - Windows registry system access....

Parses a simple path and assumes last name of path is key to read/write a value to.
HKCU
HKLM
and capital longhand forms of above are supported.

``` javascript
var vfs = require( "./vfs_module.js" )

function update( newVal ) {
        var oldval = vfs.registry.get( "HKCU/Software/Freedom Collective/ChangeMe" );
        console.log( "Old Value is:", oldval );
        if( oldval != newVal )
                 vfs.registry.set( "HKCU/Software/Freedom Collective/ChangeMe", newVal );
}
update( Date.now() );
```

The above script reads the value, reports what it was, if the old value is not the new value, then write the new value.
example was testing a constant like 526.  if typeof value is a number, value is set as a REG_DWORD.  Otherwise it's set as REG_SZ.


registry = {
     set( regPath, value );
     get( regPath );
}

``` js
var reg = vfs.registry.set( "HKCU/something", value );
var val = vfs.registry.get( "HKCU/something" );
```


# COM Ports
   (result from vfs.ComPort() )
 
``` js
ComObject = { 
     onRead( callback ) - sets a callback to be called with a uint8Array parameter when data arrives on the port.
     write( uint8Array ) - write buffer specfied to com port; only accepts uint8array.
     rts = true/false - set rts flag.
     close() - close the com port.
}

COM port settings are kept in the default option database under 
  /comports.ini/COM PORTS/&lt;comName&gt; = 57600,N,8,1,carrier,RTS,rTSflow
  /comports.ini/&lt;comName&gt;/port timeout = 100 

     the com port settings string is semi-standard DOS string... the first paramaeters, 
         baud, N/E/O/M/S (parity), 8/7 (data bits), 1/2 (stop bits); 
         then the next settings are toggles based on case...
            [C/c]arrier - enable/disable respect carrier detect to send (lower case is disable)
            [R/r]TS - enable/disable respect request to send (lower case is disable)
            [R/r]TSflow - enable/disable using RTS for flow control. (lower case is disable)

    port timeout is how long to wait for idle between sending com data.  
        (no data received for 100ms, post packet; this gives a slight delay between when the 
         data is received and actually dispatched)

```

# Config and Text Parsing Methods

   Configuration file processing; for processing formatted statements in text configurations.  Can
also be used to process streaming text input and responding with triggers based on configuration.
Things like a log analyzer can be created using this interface.

``` js
var sack = require( "sack.vfs" );
var processor = sack.Config();
processor.add( "some line with %i words", (n) => console.log( "found line param:", n ); );

//processor.go( "filename.txt" ); // reads configuration from a file...
processor.write( "Some sort of file streaming data\n" );  // the newline will be important, 
                                                          //otherwise it will be a single long line to match.

processor.write( "some line with 33 words\n" );  

// output:
// found line param: 33


// sample to read a configuration file.
var disk = sack.Volume();
processor.write( disk.read( "some file" ).toString() );
```


Configuration processor can also be used to build log scanners.  Since most of a line in a log is 
the same sort of information token matching and fitlering can trigger event handlers as each line
is discovered.  Recusive contexts can be used within config that will either disable the prior set 
of rules and begin a new set; for example, if a block header is discoverd that the following lines
would also relate to this, then it probably won't be any of the higher level rules for a while, so
a new sub-conconfiguration processor can be opened and rules added.  Whlie in this mode additional 
rules may also extend handlers; for example, processing a GUI configuration that describes a layout, 
a header 'Control....' might be discovered, and as part of the control there are lots of common properties ...
(CSS styles for instance), but then the type of the control might trigger adding an additional set of rules
for things specific to that type of control on the page (button click events), rules for a macro button, properties
for a text label like scroll, highlight, text line justification...



| Methods | arguments | description |
|----|-----|-----|
| add | (format, callback) | Add a matching expression to process match against the stream.  THe callback is passed the variables parts matched in the format statemnt in sequence of match. |
| go | (filename) | Reads a file and processes each line in the file |
| write | ( content) | Write some stream data to be processed; instead of coming from a file, data comes from this |
| on | (event,callback) | defines some event handlers on the stream.  
|   | "unhandled"  |  event is triggered on a line that does not match any format that has been added  |
|   | "done" |  file has finished processing (redundant, go() does not return until file is finished anyway   |
| begin | ( section name ) | begins a new configuration layer.  Rules can be added to this, and are applied until section is end()ed.  This allows defining things like custom configuration handler to handle sections of a configuration; an example might be the configuration defines controls in a layout, and some of the contorls are buttons that have different properties than say text fields. |
| end | () | ends the current sub-section processing.  Pops the context off the current processing stack |
| addFilter | ( data ) | Allows adding a transformation function on the data.  This can create custom ways to separate lines of text for instance (maybe it doesn't end with regular carriage returns).  Can also perform translation on the data and result with different data than it was passed. (Not yet implemented) |
| clearFilters | () | removes filters from the current handler (Not yet implemented) |


These methods are on the (module).Config object itself, not on an instance of the object.
|Static methods| arguments | Description |
| expand | (string) | Adds escape characters to the string to handle default filters.  (escape backslashes and hash marks for instane)
| strip | (string) | Remove extra characters from a string (unescape).  Usually don't have to do this, expand written will be stripped when read. |
| color | (color)  | format a parameter suitable for reading as a color formatted argument |
| encode | (TYpedArray) | format a block of binary data so it can be read in text; it's a variation of base64 encoding |
| decode | (stirng ) | result with a block of data that was previousy encoded |

#### Supported Formats
  This is a table of the format expressions allowed.  '%' character is the prefix of all of these.  if a literal percent is to be matched it should be prefixed with a backslash '\\%'

| Character | meaning |
|----|-----|
| m | multi-word.  Matches a sequency of words.  Ends at either the next word after %m specified or the end of line |
| w | single-word.  Matches a single word. |
| i | integer.  Matches a number.  Argument is passed as an integer |
| f | floating point number.  Matches a decimal/integer type number.  Argument is passed as a number. |
| b | boolean.  Matches (yes/no, true/false, on/off, 1/0, open/close) and is passed as a boolean argument |
| B | blob/binary.  String encountered matched binary encoded string; callback will be passed the binary buffer |
| c | color.  String matched a formatted color.  Color is passed to the callback (may require GUI support to be useful) |
| q | quotient.  Argument is passed as a fraction type (needs more support; fraction library interface not implemented) |
| u | url.  Matches a URL (host.address:port/with/path) 
| d | directory name.  allows gathering words with slashes as a path; must end with a slash (/ or \) |
| n | name of file.  Allows gathering words with slashes as a filename (same as directory?) |
| p | path (filepath).  Interpted path in sequence of words |
| a | a network address... host.name:port; collect a hostname/IP and optional port as an expression |
 

 ---

 ## Task

Task interface for launching and monitoring tasks.  Windows tasks are launched first by name, 
as processes, then as a shell execute (runs things like shortcut .lnk files), and then as 
a `cmd.exe /c ... ` command to run batch files.  Linux processes are attempted first to exec by the
name directly, and then try for each path set in PATH.

Pipes are connected to a task's stdin/stdout/stderr inputs if a output callback is specified.  The pipes
are left untouched otherwise.

| Task Static Methods | description |
| loadLibrary( libname ) | Load external shared library. ex: `sack.Task.loadLibrary( "xxx" );` |
| getDisplays() | (Windows) returns an array of information about displays. |

Having created a task instance with `sack.Task( {...} );` the following methods are available
to interact with the process.

 | Task methods | Description |
 |----|----|
 |end() | attempt to cause a task to exit.  It will first dispatch ctrl-c, ctrl-break, post a WM_QUIT message, and if the program does not end soon enough, terminates the process.  (closing pipes to task could also be implemented?)| 
 |terminate() | Terminates the task.  Terminates the process. |
 |write(buf) | Writes data to the task's stdin. |
 |send(buf) | Writes data to the task's stdin. |
 |exitCode | After/during the `end` callback, this may be queried to get the return code of the task |
 |moveWindow(object) | (Windows only) Move the task's primary window to the specifed location.  See Move options below. |
 |refreshWindow() | (Windows only) Refresh the internal handle for the window. (Set for moveWindow, and used later for moveWindow() or end())|
 |windowTitle() | (Windows only) Gets the current title of the window associated with the task. |

 Task Option object may contain these options to control how the task is spawned.

| Task options | Type | Description |
|----|----|-----|
| work  | string | specify the directory the task will be started in |
| bin | string | program to run |
| args | string or [ string [, string]...] | an argument or array of arguments to pass to the program |
| firstArgIsArg | bool | If false, first argument in `args` is program name and not an argument (POSIX exec); default is true, and the first argument in `args` is the first argument |
| env | object | key:value pairs to write to the environment before launching the process |
| binary | bool | if true, buffer to input callback will be an ArrayBuffer else it will be a string |
| input | callback(buffer) | A callback function which will receive output from the task.  This only receives, and, when specified, redirects the stdoutput stream. | 
| errorInput| callback(buffer) | A callback, which if specified, redirects stderr to this callback function; otherwise stderr is not captured. |
| end | callback() | This callback will be triggered when the task exits. |
| impersonate | bool | (Needs work;updated compatibility... this is for a service to launch a task as a user which is now impossible(?)) |
| hidden | bool | set windows UI flags such that the next process is created with a hidden window.  Default: false |
| firstArgIsArg | bool | Specified if the first argument in the array or string is the first argument or is the program name.  If it the first element is the program name, set to false.  If it is the first argument set true.  Default: true |
| newGroup | bool | create task as a new task group instead of a child of this group.  Default: false|
| newConsole | bool | create a new console for the new task; instead of ineriting the existing console, default false |
| suspend | bool | create task suspended.  Default: false |
| useBreak | bool | set task to use ctrl-break instead of ctrl-c; if it's a window generates WM_CLOSE regardless.  default: false |
| useSignal | bool | set task to use exit signal instead of ctrl-c or ctrl-break.  default: false |
| noKill | bool | allow task to continue running after the parent exits.  default: false (kills children at exit) |
| noWait | bool | Allow waiting for tasks that don't have an end() or input() callback specified.  default: true (don't wait if no callbacks) |
| detach | bool | (Windows) option to create a detached console process (like newConsole, but no Console is created).  default: false |
| moveTo | object | After the task is started, move its window to the specified location.  (See Move options below)

### Task Move Options

| Move options | Type | Description |
|----|----|-----|
| timeout | number | specifies milliseconds to wait for window to exist.  If no task window is found, status is `false`.  Default is 500.|
| x | number | specifies the x/left position of the window. Default is 0.|
| y | number | specifies the y/top position of the window. Default is 0.|
| width | number | specifies the width position of the window. Default is 1920.|
| height | number | specifies the height position of the window. Default is 1080.|
| display | number | specifies the display number to move the display to. 0 is the primary display, 1-N are displays by ID number.  If display option is specified, then `x`,`y`,`width`, and `height` options are ignored. Default is -1.|
| cb | function | callback function which receives `true`/`false` parameter indicating the result of the move operation.  `false` rsults if the display number is invalid, or if the timeout occurs before finding the window. |




``` js
var sack = require( "sack.vfs");

// don't redirect/capture input/output
var task1 = sack.Task( {bin:"echo", args:"hello, World"});

// send tasks's output to console.log...
var task2 = sack.Task( {bin:"echo", args:"hello, World", input:console.log });

sack.Task( { bin: "notepad.exe", args:"test.txt" } );
// default tasks exit when node does... or when garbage collected... 
// unless end and or input event handlers are attached...
setTimeout( ()=>{ }, 5000 );


```

