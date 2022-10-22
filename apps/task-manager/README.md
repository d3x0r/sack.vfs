
# Task Launcher and Manager

hosts an http/websocket service for launcher interfacing.  Default port is `process.env.PORT || 8080`.

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

###  Configuration global options

This configuration is flimsy at best, and is only meant at this time to launch node instances; so the
full path is used for compatibility between windows and linux.  On linux, the PATH variable is searched
for the program to run, while windows requires a full path (otherwise stdio redirection is not captured).

|name|Type|Description|
|---|---|----|
| port | number | port to host service on |
| winroot | string | when run on windows, this string is prepended to the bin name provided |
| winsuffix | string | when run on windows, this string is appended to the bin name provided |

