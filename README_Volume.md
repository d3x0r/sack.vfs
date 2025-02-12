


# Volume Interface 

  Arguments to Volume() constuctor `( mount name, filename, extra...)` .  If empty parameters `()`, the native filesystem is opened specifically.  If 
a mount is desired to be opened, it filename must be passed as null; `Volume( "mount", null )`.  If the first parameter is only a string, a VFS volume
by the name is opened and the volume is mounted anonymously; `Volume( "filename" )`. Similarly if the mount name is NULL, and a filename is passed as the second
parameter `Volume( null, "filename" )`, the volume is also mounted anonymously, so it can be refeenced when opening sqlite databases in it.  

`new` may be optionally applied to (most) any root object.  I may have gotten sloppy in some later works.  Everything behaves mostly as a factory by default.

Two addditional strings may be applied to encrypt the volume using those two key parts.

The following methods exist on the resulting object.

``` js
Volume() = {
    File(filename) - open a file (returns a new object)
            (filename) 
                the file to open in the volume.
    dir() - returns an array of filenames in the volume.
    exists(file/path name) - boolean if filename exists (as a file or a directory)
    read(fileName) - read a file from a volume; return an ArrayBuffer with a toString() that returns the buffer as utf8 as a string.
    readJSON(fileName, callback) - read a file from a volume; calls callback with each object decoded from the file interpreted as JSON (unimplemented)
    readJSON6(fileName, callback) - read a file from a volume; calls callback with each object decoded from the file interpreted as JSON6.
    readJSOX(fileName, callback) - read a file from a volume; calls callback with each object decoded from the file interpreted as JSOX.
    write(fileName,arrayBuffer/string) - writes a string or arraybuffer(or typed array) to a file. 
    Sqlite(database) - an interface to sqlite database in this volume.
    rm(file),delete(file),unlink(file) - delete a file.
    mv(file,new_filename),rename(file,new_filename) - rename a file. (mostly limited to same physical device)
    mkdir - utility function to make directories which might not exist before volume does; (volume auto creates directories now)
            parameters - (pathname)
                path to create- will created missing parent path parts too.
    rekey(version,a,b) - set key for a volume.
}

Volume  // function has these methods.
    readAsString( filename ) - read a local system file as a string; used to quick-read for require( "*.[jsox/json6]")
    mapFile( filename ) - return an ArrayBuffer that is the memory mapped file on the disk.
    cwd - getter that results with current working directory (may later be isolate-local).
    Thread : {
        post( uniqueId, Volume ) - post a volume to a thread by a unique identifier.
        accept( uniqueId, cb(volume) ) - registers a callback which is called with a volume thrown from some other thread.
    }


```



### File Interface opened within a Volume

 (result from vfs.Volume().File())

``` js
File instance methods (prototype methods)
File = {
    read(size[, position]) - read from the file; return an ArrayBuffer with a toString method to interpret it as utf8.  Optional parameter
                           position may set the position to read from.
    readLine( [position] ) - reads a line from a text file; optional parameter position may set the position to read from.
    write(arrayBuffer/string) - write data to the file; at the current position
    writeLine( line [, position] ) - output text to a file; a newline will be automatically appended to the line.
    seek(pos[,whence]) - set file position to write.  optional whence parameter can be vfs.File.SeekSet(0), vfs.File.SeekCurrent(1), or vfs.File.SeekEnd(2)
    trunc() - set file size to the curent file position.
    pos() - return the current position in the file.
    ... /*most other methods unimplemented*/
}
```

### File Methods

    - rm(file)
    - delete(file)
    - unlink(file) - delete in the native filesystem; file may have to be closed before delete can work.

### File Constants

    SeekSet - used in seek methods; value SEEK_SET(0)
    SeekCurrent - used in seek methods; value SEEK_CUR(1)
    SeekEnd - used in seek methods; value SEEK_END(2)


# Usage

``` js
var sack = require( 'sack.vfs' );
var volume = sack.Volume( "MountName", "fileName.vfs" );
var file = volume.File( "filename" );
var fileString = file.read();
var fileOut = volume.File( "filename.out" );
fileOut.write( fileString );

```

``` mjs
import {sack} from "sack.vfs";
const volume = sack.Volume( "MountName", "fileName.vfs" );
const file = volume.File( "filename" );
const fileArrayBuffer = file.read(); 
const fileString = fileArrayBuffer.toString(); // added method that is utf8 conversion.
const fileOut = volume.File( "filename.out" );
fileOut.write( fileString );

```


