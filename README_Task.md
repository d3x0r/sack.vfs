# Task

Task interface for launching and monitoring tasks.  Windows tasks are launched first by name, 
as processes, then as a shell execute (runs things like shortcut .lnk files), and then as 
a `cmd.exe /c ... ` command to run batch files.  Linux processes are attempted first to exec by the
name directly, and then try for each path set in PATH.

Pipes are connected to a task's stdin/stdout/stderr inputs if a output callback is specified.  The standard IO
pipes are left untouched otherwise.  If task options define overridden `input()` and `errorInput()` handlers, then
inheritance on the the default standard IO handles is disabled.  If there are no input handlers, and it's a new console, 
and noClose the standard IO handle inheritance is also disabled.  There is also an option to just prevent standard IO 
handle inheritance.  (Many handles created by SACK have inheritance disabled by default).

| Task Static Methods | description |
| loadLibrary( libname ) | Load external shared library. ex: `sack.Task.loadLibrary( "xxx" );` |
| processId() | returns the current processes ID |
| parentId() | returns the parent processes ID |
| getProcessList( partial name) | gets a list of processes that the program name matches the optional `partial name` parameter; if no parameter or null is specified, then all processes are returned|
| getDisplays() | (Windows) returns an array of information about displays. |
| getStyles( processId ) | (Windows) returns an object with `window`, `windowEx` and `class` members that are the style values|
| setStyles( processId, window, windowEx, class ) | (Windows) Sets the style of a window associated with the process ID, -1 does not set any bits |
| getTitle( processId ) | (Windows) returns the title of the window associated with a process.
| getPosition( processId ) | (Windows) returns an object with the current window x,y, width, height |
| setPosition( processId, { x,y,width,height} ) | (Windows) Set the position of a window associated with the process ID. |
| modulePath | where sack-[vfs/gui].node loaded from |
| dataPath | where shared data is for the module |
| commonDataPath| global common data? like ProgramData on home, ~/.freedom Collective on linus |
| programDataPath| Program specific datapath? |
| programPath | The path of the program that is running |
| programName | the name (minus.exe) of the program that is running |
| kill(id) | attempt to kill a task by process ID (sigterm or TerminateProcess) |
| stop(id) | attempt to stop a task by process ID (sigint or WM_QUIT/Ctrl-C/EndProcess) |
| onEnd(id, cb(exitCode) ) | monitor a process ID to exit, when it does exit, send exit code to callback `cb`; if (exitCode === null) process with ID was not found; has already exited, callback happens before function returns.|


``` js
import {sack} from "sack.vfs"

console.log( "Program:", sack.Task.programName );
console.log( "Program Path:", sack.Task.programPath );
console.log( "Program Data:", sack.Task.programDataPath );
console.log( "Common(global) Data:", sack.Task.commonDataPath );
console.log( "Module Path:", sack.Task.modulePath );
console.log( "Data Path:", sack.Task.dataPath );

/***** OUTPUT *****
Program: node
Program Path: C:\Users\d3x0r\AppData\Local\nvs\default
Program Data: C:\ProgramData\Freedom Collective\node
Common(global) Data: C:\ProgramData\Freedom Collective
Module Path: M:\javascript\vfs\native\build\RelWithDebInfo
Data Path: C:\Users\d3x0r\AppData\Local\nvs
******************/

```


Having created a task instance with `sack.Task( {...} );` the following methods are available
to interact with the process.

 | Task methods | Description |
 |----|----|
 |end() | attempt to cause a task to exit.  It will first dispatch ctrl-c, ctrl-break, post a WM_QUIT message, and if the program does not end soon enough, terminates the process.  (closing pipes to task could also be implemented?)| 
 |terminate() | Terminates the task.  Terminates the process. |
 |write(buf) | Writes data to the task's stdin. |
 |isRunning() | Tests if the process is till running. |
 |send(buf) | Writes data to the task's stdin. |
 |exitCode | After/during the `end` callback, this may be queried to get the return code of the task |
 |moveWindow(object) | (Windows only) Move the task's primary window to the specifed location.  See Move options below. |
 |refreshWindow() | (Windows only) Refresh the internal handle for the window. (Set for moveWindow, and used later for moveWindow() or end())|
 |windowTitle() | (Windows only) Gets the current title of the window associated with the task. |
 |getPosition() | (Windows only) Gets the current position of the window associated with a task. |
 |getStyles() | (Windows only) Gets the current style of a window associated with a task. |
 |setStyles(window,windowEx,class) | (Windows only) Sets the specified style bits of window associated with a task.|

 Task Option object may contain these options to control how the task is spawned.

| Task options | Type | Description |
|----|----|-----|
| work  | string | specify the directory the task will be started in |
| bin | string | program to run |
| args | string or [ string [, string]...] | an argument or array of arguments to pass to the program |
| firstArgIsArg | bool | If false, first argument in `args` is program name and not an argument (POSIX exec); default is true, and the first argument in `args` is the first argument |
| firstArgIsArg | bool | Specified if the first argument in the array or string is the first argument or is the program name.  If it the first element is the program name, set to false.  If it is the first argument set true.  Default: true |
| env | object | key:value pairs to write to the environment before launching the process |
| binary | bool | if true, buffer to input callback will be an ArrayBuffer else it will be a string |
| input | callback(buffer) | A callback function which will receive output from the task.  This only receives, and, when specified, redirects the stdoutput stream. | 
| errorInput| callback(buffer) | A callback, which if specified, redirects stderr to this callback function; otherwise stderr is not captured. |
| end | callback() | This callback will be triggered when the task exits. |
| impersonate | bool | (Needs work;updated compatibility... this is for a service to launch a task as a user which is now impossible(?)) |
| hidden | bool | set windows UI flags such that the next process is created with a hidden window.  Default: false |
| newGroup | bool | (Windows)create task as a new task group instead of a child of this group.  Default: false|
| newConsole | bool | (Windows)create a new console for the new task; instead of ineriting the existing console, default false |
| suspend | bool | create task suspended.  Default: false |
| useBreak | bool | set task to use ctrl-break instead of ctrl-c; if it's a window generates WM_CLOSE regardless.  default: false |
| useSignal | bool | set task to use exit signal instead of ctrl-c or ctrl-break.  default: false |
| noKill | bool | allow task to continue running after the parent exits.  default: false (kills children at exit) |
| noWait | bool | Allow waiting for tasks that don't have an end() or input() callback specified.  default: true (don't wait if no callbacks) |
| detach | bool | (Windows) option to create a detached console process (like newConsole, but no Console is created).  default: false |
| moveTo | object | (Windows)After the task is started, move its window to the specified location.  (See Move options below)
| noInheritStdio | bool | prevents task from inheriting stdio pipes |
| style | object | (Windows)After the task is started, set window style bits, then, if specified, move the window |
| usePty | bool | enable using pty on linux; no additional task interface(yet) |
| admin | bool | (Windows)Run as administrator |

### Task Move Options

Move options are applied after styles are set.

| Move options | Type | Description |
|----|----|-----|
| timeout | number | specifies milliseconds to wait for window to exist.  If no task window is found, status is `false`.  Default is 500.|
| x | number | specifies the x/left position of the window. Default is 0.|
| y | number | specifies the y/top position of the window. Default is 0.|
| width | number | specifies the width position of the window. Default is 1920.|
| height | number | specifies the height position of the window. Default is 1080.|
| display | number | specifies the display number to move the display to. 0 is the primary display, 1-N are displays by ID number.  If display option is specified, then `x`,`y`,`width`, and `height` options are ignored. Default is -1.|
| monitor | number | monitor number(?) think this ends up being the same as display.|
| connector | number | specifies the display connector to move the display to. This relates to physical connector (and may need more info)|
| cb | function | callback function which receives `true`/`false` parameter indicating the result of the move operation.  `false` results if the display number is invalid, or if the timeout occurs before finding the window. |


### Task Style Object Options

Styles defined for the window of a task are applied first, and then any move options specified (otherwise the border of
the window may disappear, and the client area of the window would result smaller than the position information specified).

| Move options | Type | Description |
|----|----|-----|
| timeout | number | specifies milliseconds to wait for window to exist.  If no task window is found, status is `false`.  Default is 500.|
| window | number | Specifies bits to set in window style.  If -1 or undefined, option is ignored.|
| windowEx | number | Specifies bits to set in window Ex style.  If -1 or undefined, option is ignored.|
| class | number | Specifies bits to set in window's class style.  If -1 or undefined, option is ignored.|
| cb | function | callback function which receives 0-7 parameter indicating the result of the move operation.  `0` results if the display number is invalid, or if the timeout occurs before finding the window. |


### Style Values

There is a 'style' object on Task (`sack.Task.style`) which contains `window`, `windowEx` and `class` objects which
have the names of the bits defined by windows for window and class styles.  These values may be used to create the 
integer values associated with a task; for example `sack.Task.style.window.WS_POPUP|sack.Task.style.window.WS_BORDER`.


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


