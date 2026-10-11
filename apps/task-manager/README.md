
# Task Launcher and Manager

hosts an http/websocket service for launcher interfacing.  Default port is `process.env.PORT || config.PORT || 8080`.  `config` the file 
read from the current directory to load a file 'config.tasks.jsox' which is in JSOX format(JSON+).

Task entries are described below. 


`node  --import=sack.vfs/import node_modules\sack.vfs\apps\task-manager\src\main.mjs`

## HTTP controls

The REST-ish endpoints are intended for automation.

| endpoint | query | response |
|---|---|---|
| `/start` | `task=<name>` | starts the task; restarts it if already running |
| `/stop` | `task=<name>` | stops the task |
| `/restart` | `task=<name>` | sets restart/start behavior for the task |
| `/running` | `task=<name>` | `true` or `false` |
| `/ready` | `task=<name>` | `true` only after the task's configured `readyPort` or `readyDelay` check succeeds |
| `/list` | optional `json=1` | task ids, running state, and names |
| `/log` | optional `task=<name>` or `id=<task id>`; optional `time=1`; optional `at=<line index>`; optional `length=<count>`; optional `json=1` | recent log lines as plain text, or JSON with `json=1` |

Task lookup tries exact name, case-insensitive exact name, unique prefix, then unique contains match.  Ambiguous close matches return `409 Ambiguous Task` with candidate task names, or `{ error, matches }` when `json=1`.

`/running` reports whether the child process exists; it does not imply that the service has begun accepting connections.  Use `/ready` for the configured readiness result.  JSON `/list` responses include both `running` and `ready`.

`/log` without a task returns the previous 50 retained lines from the interleaved master log.  `/log?task=Example&time=1` returns one task's log and prefixes each line with the captured log timestamp.  `at` uses the absolute line index returned in the `X-Task-Log-At` header to page older retained log lines; `length` controls how many lines are returned.  The response also includes `X-Task-Log-Length`.  `/log?task=Example&json=1` returns `{ at, length, truncated, log }`; `/log?json=1` returns master log entries as `{ taskId, taskName, time, error, line }`.

## Configuration

`config.jsox` is the configuation file for tasks.

```

{
	port: 8089,
	tasks: [
		{ name: "Log Test Program",
		  bin: "node",
		  work: "test/test_logging",
		  args: ["--import=sack.vfs/import", "generate_log.mjs" ],
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
| bin | string | this is the name of the program to run; it is built from this and default options specified in the configuration; linux searches the PATH environment.  Leave it out for a task that runs nothing - see [Readiness, probes and pre-run steps](#readiness-probes-and-pre-run-steps). |
| winbin | string | used if `process.platform !== linux`  |
| linbin | string | used if `process.platform === linux`   |
| restart | bool | defaults task to automatically restart when it fails |
| work | string | this is where the program's working directory is set when it is started |
| args | array of strings | These arguments are passed to the application started |
| firstArgIsArg | bool | Pass `args[0]` as the first real argument. Task-manager defaults this to true. |
| env | object of values | Names of the fields in this object are used to define environment variables for the task launched. |
|			newGroup|bool | (WIN32) Start task as a new group.  Signaling ctrl-c, ctrl-break relies on sending those signals to a group.  |
|			noKill | bool | don't kill the task on exit.  Child tasks that are started will normally be killed when the launcher exits. |
|			noWait | bool | don't wait on the task (when exiting?)  |
|			newConsole | bool | (WIN32) Starts task with a new console window.  |
|			usePty | bool | (WIN32) Starts task through a pseudo terminal instead of plain pipes.  |
|			ptySize | object | Size for a pseudo terminal task: `cols`, `rows`, and optional pixel `width`, `height`. Defaults to 80x30. |
|			useSignal |bool| (WIN32) A end task signal can be registered by children, this option enables using the signal to trigger an exit instead of ctrl-c, ctrl-break. |
|			useBreak |bool| (WIN32) Stop task with ctrl-break instead of ctrl-C effective signal.  |
|			moveTo | object | (WIN32) specifies where to move the main window of the task to when it launches,  |
|			style | number | (WIN32) Style to configure the main window of a task (remove border, make popup,.. )  |
|			noInheritStdio | bool | prevent standard IO handles from being inherited.  |
|			multiStart | bool | Once started, create a new, unstarted version of the same task. |
| dependsOn | [string,...] | An array of names of other tasks which this depends on.  Dependant tasks are started first.  Tasks that depend on a started task are also started, once every dependency is *ready* (below). |
| readyPort | number | The task is ready once something accepts a connection on this port.  Dependants wait for that rather than starting the moment this task launches. |
| readyHost | string | Host to probe for `readyPort` (default `localhost`). |
| readyDelay | number | With no `readyPort`, the task is ready this many ms after launch. |
| readyTimeout | number | How long to keep probing `readyPort` before giving up and declaring the task ready anyway, so a chain is never blocked outright (default 30000).  Ignored by a task with no `bin`. |
| readyRecheck | number | Task with no `bin` only: once the port answers, how often (ms) to check it is still there (default 5000). |
| readyMisses | number | Task with no `bin` only: this many failed re-checks in a row and the task goes down, taking dependants with it, then keeps probing for the port to return (default 3). |
| readyOnExit | bool | A pre-run step.  Not ready while it runs; a clean exit is what starts its dependants, and each later start of a dependant runs this step again first.  A non-zero exit starts nothing.  `restart` is ignored. |
| temporary | When the task ends, the definition for
the task is removed; the task is not saved to the running tasks config; useful for remote task invokations. |
| autoEndBatch | watches the stdin pipe for the ending message of a batch file to terminate y/n; This can happen if the task is sent a ctrl-c, this will finish the task termination |



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

### Readiness, probes and pre-run steps

`dependsOn` orders launches; *ready* is what spaces them.  A task is ready as
soon as it is running unless it says otherwise with `readyPort` (wait for
something to accept a connection there) or `readyDelay` (just wait).  Dependants
hold in the "Waiting" state until every dependency is ready, and a dependency
going down takes its dependants down with it.  The `on( taskName, "ready", cb )`
hook exported by `main.mjs` gives plugins the same signal.

**A task with no `bin` runs nothing; it is only its readiness check.**

- With a `readyPort` it is a *probe* on something this manager does not run: a
  remote service, a database on another machine.  It polls until the port
  answers (there is no timeout - the port answering is the whole task), then
  keeps checking every `readyRecheck` ms.  After `readyMisses` failures in a row
  it goes down the way a process exit does, dependants cascade, and it resumes
  probing on its own until the port is back.  Stopping it by hand holds it down
  like any other task.
- With no port it is a *placeholder*: ready as soon as it is started, which
  makes a single name several tasks can depend on.

```
  { name: "user database", readyHost: "app.d3x0r.org", readyPort: 8190, readyRecheck: 10000 },
  { name: "launcher", bin: "node", args: [ "launcher.mjs" ], dependsOn: [ "user database" ], restart: true },
```

**`readyOnExit` makes a task a pre-run step**, for the case where something has
to happen before a dependant comes up - typically killing the stray browser
processes that stop a fresh instance from starting.  It is never ready while it
runs.  A clean exit (code 0) is the ready edge that starts its dependants, and
ready drops again at once, so the next start of a dependant - from `restart`,
or by hand - runs the step again first.  A non-zero exit marks the step failed
and starts nothing.  Finishing is not dying, so its dependants are not cascaded
down, and `restart` on the step itself is ignored.  Nothing is written anywhere
to remember it ran; the step is simply re-run whenever it is needed.

```
  { name: "kill-edge", bin: "taskkill", args: [ "/F", "/IM", "msedge.exe" ], readyOnExit: true },
  { name: "edge", bin: "msedge.exe", args: [ "--kiosk", "http://localhost/" ], dependsOn: [ "kill-edge" ], restart: true },
```


###  Configuration global options

This configuration is flimsy at best, and is only meant at this time to launch node instances; so the
full path is used for compatibility between windows and linux.  On linux, the PATH variable is searched
for the program to run, while windows requires a full path (otherwise stdio redirection is not captured).

|name|Type|Description|
|---|---|----|
| port | number | port to host service on |
| hostname | string | name this service manager reports to its upstream; defaults to the machine's hostname |
| useUpstream | bool | Enables connecting to an upstream task server |
| upstreamServer | string | "Host:port" address to connect to (no `ws://` prefix) |
| disallowUpstreamTaskManagment | bool | the upstream may list and watch tasks here, but not start, stop, or edit them |
| maxMasterLogLines | number | maximum number of interleaved master log entries to retain |
| extraModules| array of {name,function} | Specifies additional modules to load before starting any tasks.  This are expected to be async functions and await resolution of each module in turn.|
| onStopAll| array of {name,function,options} | Specifies additional modules to load when Stop All is triggered.  This are expected to be async functions and await resolution of each module in turn. `module.function(options)` |
| tasks | array of Task configurations above | list of defined tasks |


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
A circular list of upstream servers might be constructed; this is untested, but should be fairly harmless... a system
which receives itself will end up showing itself as a tab of itself, but no deeper.
