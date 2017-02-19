# sack.vfs

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
Node addon for a custom virtual file system interface

# Usage

```
var vfs = require( 'sack.vfs' );
var volume = vfs.Volume( "MountName", "fileName.vfs" );
var file = volume.File( "filename" );
var fileString = file.read();
var fileOut = volume.File( "filename.out" );
fileOut.write( fileString );

```

## Objects

```
vfs = {
    Volume - a virtual disk partition that holds files.
    Sqlite - an interface to sqlite which can open databases in the volume.
}

Volume = {
    File - open a file (returns a new object)
    dir - returns an array of filenames in the volume.
}

File = {
    read - read from the file
    write - write data to he file
    seek - set file position to write
    ... /*most other methods unimplemented*/
}

Sqlite = {
    do - execute a sql command
    op - get an option from sqlite option database
    getOption - longer name of 'op'
    so - set an option in sqlite option database
    setOption - longer name of 'so'
    makeTable - evalute a sql create table clause, and create or update a table based on its definitions.
             Additional columns will be added, old columns are untouched, even if the type specified changes.
             Addtional indexes and constraints can be added...
             
    fo - find an option node under this one (returns null if node doesn't exist)   fo( "name" )
    go - get an option node      go( "name" )
    eo - enum option nodes from root of options  eo( callback ) ... eo( (node,name)=>{console.log( "got", name, node.value );} )
     
    var results = sqlite.do(sql);  // do a sql command, if command doesn't generate data result will be true instead of an array
    
}


OptionNode = {
    fo - find an option node under this one (returns null if node doesn't exist)   fo( "name" )
    go - get an option node      go( "name" )
    eo - enum option nodes from root of options  eo( callback ) ... eo( (node,name)=>{console.log( "got", name, node.value );} )
    value - set/get current value
}```

---

need to do a better job describing the above, parameter values for instance...
