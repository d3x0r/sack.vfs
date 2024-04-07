
# Task Launcher and Manager

hosts an http/websocket service for launcher interfacing.  Default port is `process.env.PORT || config.PORT || 8080`.  `config` the file 
read from the current directory to load a file 'config.tasks.jsox' which is in JSOX format(JSON+).

Task entries are described below. 


`node  --experimental-loader=sack.vfs/import.mjs node_modules\sack.vfs\apps\task-manager\src\main.mjs`

## Configuration

`config.jsox` is the configuation file for tasks.

```

{
	port: 8089,
	winroot : "C:\\Program Files\\nodejs\\",
	winsuffix: ".exe",
	tasks: [
		{ name: "Log Test Program",
		  bin: "node",
		  work: "test/test_logging",
		  args: ["--experimental-loader=sack.vfs/import.mjs", "generate_log.mjs" ],
		  env: { MORE_ENV: "value" },
		},
	]
}

```
### Task configuration options

Tasks to run are defined with a few fields.

|name|Type|Description|
|---|---|----|
| name | string | this is the text name shown on the UI regarding this task. |
| bin | string | this is the name of the program to run; it is built from this and default options specified in the configuration; linux searches the PATH environment. |
| winbin | string | used if `process.platform !== linux`  |
| linbin | string | used if `process.platform === linux`   |
| restart | bool | defaults task to automatically restart when it fails |
| work | string | this is where the program's working directory is set when it is started |
| args | array of strings | These arguments are passed to the application started |
| env | object of values | Names of the fields in this object are used to define environment variables for the task launched. |
|			newGroup|bool | (WIN32) Start task as a new group.  Signaling ctrl-c, ctrl-break relies on sending those signals to a group.  |
|			noKill | bool | don't kill the task on exit.  Child tasks that are started will normally be killed when the launcher exits. |
|			noWait | bool | don't wait on the task (when exiting?)  |
|			newConsole | bool | (WIN32) Starts task with a new console window.  |
|			useSignal |bool| (WIN32) A end task signal can be registered by children, this option enables using the signal to trigger an exit instead of ctrl-c, ctrl-break. |
|			useBreak |bool| (WIN32) Stop task with ctrl-break instead of ctrl-C effective signal.  |
|			moveTo | object | (WIN32) specifies where to move the main window of the task to when it launches,  |
|			style | number | (WIN32) Style to configure the main window of a task (remove border, make popup,.. )  |
|			noInheritStdio | bool | prevent standard IO handles from being inherited.  |


Console applications on windows should be configured with new group = true.  If it is part of the same group as the launcher, then the launcher would end up sending itself
ctrl-c or ctrl-break when attempting to end a task.

```
		  moveTo: { display:2, timeout:1000 },
```

|field | type | description |
|---|---|---|
| display | number | the windows display number to move the window to full screen on that display. |
| monitor | number | the windows monitor number to move the window to full screen on that display.  Like display, but doesn't match 'Identify' on windows display settings; display does match. |
| timeout | number | how long to wait before giving up moving the window. |
| x | number | x position to put the window  |
| y | number | y position to put the window |
| width | number | how wide to make the window |
| height | number | how tall to make the window |

The absolute position settings are ignored if display or monitor is specified.  Display is more stable than Monitor.  Display overrides monitor and both override the absolute position `x,y, width, height`.


###  Configuration global options

This configuration is flimsy at best, and is only meant at this time to launch node instances; so the
full path is used for compatibility between windows and linux.  On linux, the PATH variable is searched
for the program to run, while windows requires a full path (otherwise stdio redirection is not captured).

|name|Type|Description|
|---|---|----|
| port | number | port to host service on |
| winroot | string | when run on windows, this string is prepended to the bin name provided |
| winsuffix | string | when run on windows, this string is appended to the bin name provided |
| useUpstream | bool | Enables connecting to an upstream task server |
| upstreamServer | string | "Host:port" address to connect to, with `ws://` (support wss?) |
|	extraModules| array of {name,function} | Specifies additional modules to load before starting any tasks.  This are expected to be async functions and await resolution of each module in turn.|


### Extra Module Object

Extra modules can be loaded that can perform additional work.  The source file and function can 
be any value, the defaults were built with `go` as the entry point.

| name | function | function |
|----|----|----|
|	      "./winKillChrome.mjs"|"go"|windows scan for chrome processes to kill.  Tries to target those that have `--user-data-dir` |
|	      "./waitForServer.mjs"|"go"|Connects to a URL and blocks until that request succeeds. |
|	      "./winRebootKey.mjs"|"go"| connects a low level key shortcut for ctrl-alt-R to reboot the system |
|	      "./winHideMouse.mjs"|"go" | moves the mouse off the screen after a short idle time.  shift-ctrl-m brings the mouse back to the center of the primary display.  Moving the mouse returns the mouse to where it was when it was hidden, and then does the mouse move |




## Upstream Servers

Task managers can be aggregated by specifying an upstream server; then connecting to that upstream 
server will indicate all statuses and tasks of all servers
that have specified that upstream server.  There is no limit of depth.

