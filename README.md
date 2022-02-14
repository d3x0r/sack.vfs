# sack-gui

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)[![Build Status](https://travis-ci.org/d3x0r/sack.vfs.svg?branch=master)](https://travis-ci.org/d3x0r/sack.vfs)

Node addon for a lightweight platform independant gui.
Otherwise is includes all of *[sack.vfs](https://npmjs.org/package/sack.vfs)*

Vulkan API to be added eventually... 

## Requirements

CMake-js is required to build the GUI; to pull the full external sources.

* see requirements in *[sack.vfs](https://npmjs.org/package/sack.vfs)*

 - `git`; required to pull external project sources.

#### npm
	cmake-js
	// don't use these anymore... but rather internal macros
	// because I default all methods as readonly
	// all objects exported by require are constants.
        // nan (Native Abstractions for Node.js)

#### Centos/linux

 *  yum install gcc-c++ libuuid-devel unixodbc-devel
    * (uuid/uuid.h sql.h)
 *  apt-get install uuid-dev unixodbc-dev 
    * (uuid/uuid.h sql.h)
 *  pacman -S unixodbc util-linux
    * (sql.h uuid/uuid.h(probably already available, fs2util) )
 *  (?)emerge unixodbc

ODBC can be optioned out leaving only SQLite interface; or both may be optioend out; uuid is only required because of ODBC or Sqlite support inclusion.  Which can then be 0 dependancies.

#### Mac

  *  (ODBC might be optioned out; just uses sqlite typically)
  *  brew (brew install unixODBC), if ODBC is desired.

ODBC can be optioned out leaving only SQLite interface; or both may be optioend out; uuid is only required because of ODBC or Sqlite support inclusion.  
A third party UUID library is used (? or were there BSD builtins that got used, probably this).


#### Windows
	none




# Usage

```
var sack = require( 'sack-gui' );
var frame = sack.PSI.Frame( "control", 512, 512 );
frame.

```

## Tests

In GIT respository there's a tests folder.
Tests/sack_* are all gui type tests, and examples of simple gui applications.

## Objects
This is the object returned from require( 'sack.vfs' );

```
vfs = {
    /*... for core system interfaces see [sack.vfs](https://npmjs.org/packages/sack.vfs) */

   PSI : Panther's Slick Interface.  It's a simple gui.  It provides elementary dialog controls.
   Image : interface for handling images.
   Render : interface for showing images.
   InterShell : a higher level control managment engine for full screen applications.
}
```

[PSI Readme](README_PSI.md)

---

## Changelog
- 0.9.124
   - add systray support.
   (changelog lost?)
- 0.9.123 
   - Sync VFS. 
   - Fixed some GUI issues.
   - Improved documentation
- 0.9.122 - Release work in progress update; fixed link to other project.
- 0.9.121 - 
- 0.9.120 - Add listbox methods.  Make control color accessors a templated object instead of adding an object with method extensions.  
- 0.9.119 - Fix missing websocket client event accessors.  Add custom border support.  
- 0.9.118 - Update documentation and keywords. Fix building. 
- 0.9.117 - Fork from sack.vfs 0.9.117.  Initial publication to NPM.
