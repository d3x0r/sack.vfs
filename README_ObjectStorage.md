
## Object Storage

(Added from 0.9.145)
Object storage interface is a hybrid combination of virtual file system and JSOX for object encoding.

The target usage is data storage; in an object relational format that supports lazy loading.

Accessors substitued for variable fields would have been ideal; but there's still some funtionality that JS
lacks to make that elegant...

### History

After making [JSOX](https://github.com/d3x0r/JSOX),
I forked my VFS from a while ago and changed the directory structure so it's a combined radix/remainder of name structure; so as a directory block fills with strings, it finds common ones, and updates a byte-character reference for all the files that start with that... 
I had wanted to do it somewhat more specifically to use 4096 hash which is 2 base64 characters \(except the ==\/\\0 \) at the end... but that ends up being like 16k block minimum; so I revised it to just use 256 entries (based on bytes of the filename).   It has some limitations, like \0 terminates a string; so index 0 will NEVER be used... and actually most control characters aren't in text filenames so hash entries 0-31 and (other than unicode) 127-250/ and definatly 250-255 are unused because those are never bytes in utf8 (using a string 'string' access for file content); but.  It works... generating random hashes `ba63,ab3j,b532` etc eventually overlaps in groups... anyway it's overall O(1) lookup; although that breaks into 10 hash hops (which is 10 pages of memory), plus 1 page for the name fragments.... ) and then each block has 254 'directory entires' (or 53 something 64 bit), which the directory is kept in-order always, so can do a binary search which is 8 string comparisons.... so a total of 10 cache loops, and 8 string comparisons worst case.  (This is in a single C file that compiles on any system with a C library or C++ if you prefer that.)  It can be compiled using emscripten which is about 120k minmum and about 300k total (gzips smaller).  The file system IO for emcc is IndexedDb (but I think I learned that here)....
So then I don't really want a 'file' access to the storage; although it has it, so it could be possible to update segments of blobs, I really intend it to store JS objects... which are stored as a whole and read as a whole (they are encoded to the disk in a container object and JSOX encoded once).  The Object Storage put and get use extended JSOX parser and stringifer that registers a handler for ~os as in `~os"abuasdfu31=="` (object storage identifier).  When it finds one of those, it knows it's a reference to another file in storage.  
If the object is loaded from storage with `get(id)`, and no other objects were loaded, the object would be returned with strings in the places of real objects... the can be manually revived with `obj.fixkey = objStore.get(obj.fixkey)`.  I added a `map(id)` which will also track the reviving objects, read all the files, and result with the completed object exactly as saved.... 
`put(object,bSign )` returns a new ID of the object, unless the object was previously stored or restored.  the optional argument for sigining can generate a nonce and result with a ID that is a PoW for the record and nonce.
records which have a nonce cannot be re-written.  other objects can be updated with their current state.
Oh - (one last note) Added sort to JSOX encoding so objects are always encoded in the same order ( as according to JS `if( stringA > stringB )`

### Example Usage (Setup)


``` js

const sack = require( "sack.vfs" );
const objectStorage = sack.ObjectStorage( 'filename.data' );

```
### Object Storage Method overview

```
sack.ObjectStorage.
    Thread : {
        post( uniqueId, Volume ) - post a volume to a thread by a unique identifier.
        accept( uniqueId, cb(volume) ) - registers a callback which is called with a volume thrown from some other thread.
    }
```

### Constructor arguments

| Object Storage constructor arguments | Description  |
|----|----|
| filename | Filename to use for object storage database.  If the filename includes '@' the part before the '@' is treated as a mount name; ie. 'vfsMount@object.store'. |
| options | an object passed with configuration options for the volume |


### Instance Methods

| Object Storage methods | Return | Arguments | Description |
|----------|------------|-------------|----|
| getRoot | promise | () | Promise resolves with root directory |
| put | unique ID | ( object \[, options\] ) | Stores or updates an object into storage.  If the object was not previously stored or loaded, a unique ID for the object is returned, otherwise the existing ID results.  The second parameter is an optional option object, see options below. |
| get | promise |  ( [string] or [Option object] ) | Returns a promise; success is passed the object loaded from storage.  Loads an object using the specified ID. |
| map | promise |  ( id ) | Same as get, but also loads any objects of specified ID. |
| timeline | TimelineCursorObject[Native] | getter | gets an accessor to browse the timeline of the object storage. |
| -- | | | |
| delete |  | (id/object) | remove object from storage... (danging references in other stored records?) |

### Timeline browsing

| Timeline Cursor | Return | Arguments | Description |
|----|-----|----|----|
| get | array | ( option object ) | get an entry from the timeline |

### Timeline Option Object

These option fields are supported when getting information from the timeline

| Timeline getter option | Description |
|----|-----|
| read | (true/false) whether records resulting from the search include the data associated with the record |
| from | (integer) seeks to the timeline index specified |
| from | (Date/JSOX.DateNS) seeks to the timeline index specified |
| limit | (integer) number of records to limit the result set to |

### Instanct get() options

These option fields are supported in the option object passed to a storage get, `storage.get( {option_object} )`.

| Object get Options names | type | Description |
|-----|-----|-----|
| id | string | use this id for the object storage target |
| extraDecoders | [ { tag:"tag", p:Type } ] | An array of optional object decoder(parse) types to use when getting objects. |

### Instance put() options

| Object put Options names | type | Description |
|-----|-----|-----|
| id | string | use this id for the object storage target |
| version | boolean | like always create, but save old data if there was data |
| sign | bool | whether this record should be digitally signed; makes record readonly |
| key | string | (todo) use to make record readable only with specified key. |
| extraEncoders | [ { tag:"tag", p:Type } ] | An array of optional object encoder(stringify) types to use when putting objects. |

### Other options for put() 

These are a work in progress, and are somewhat deprecated at the moment.

| Object put Options names | type | Description |
|-----|-----|-----|
| signed | string | this data should be signed with a permanent id; record becomes readonly. |
| readKey | string | data to prevent foreign read |
| sealant | string | data to encode object id |
| objectHash | string | something |

## file storage within object storage

Since the ID is specified for any `get()` or `put()` a string may be passed for the data, and the literal
value will be pushed at that ID. The browsing structure for this is only available as a timeline cursor; and
browsing the identifiers is just a long list.

A few simple objects can represent a file system that has directories that contain files and other directories.
Furthermore directories can have data content just like a file; and they are almost the same.  A directory has
an array of files (and directories) that are in that directory; usage would imply you sort of hold on to the point
you have your files in and just create files in that spot.

The following sections are about using a builtin file directory system, which is itself stored as objects in the
object storage system with long unique identifiers.  The only exception is the root directory which has the identifier
of '?'.  The unique identifiers are only semi-unique, in that they are base64 encoded long integer values, using `_` 
and `$` for the 62 and 63 characters; this is one of the standard base64 character sets for encoding.  This means the
identifiers are javascript literals that can be used as an identifier; other than they are suffixed with a `=` to 
indicate the end of encoding.

### file~~like~~ system directory interface
 
This are the methods for the object `storage.getRoot()` results with.  It is a directory object that is returned.
These are the methods of a `FileDirectory`.

|Object Storage Directory Methods | Return | arguments | Description |
|----|----|----|---|
| create | FileEntry | (filename) | creates a new file, ready to be written into. |
| open | FileEntry | ( filename ) | opens a file relative to the current root. |
| folder | FileDirectory | ( pathname ) | gets a path within the current path. |
| store | none | () | saves any changes. |
| has | Boolean | ( pathanem ) | returns true/false indicating whether this directory contains the specified pathname. |
| remove | Promise | ( filename ) | delete specified file from the file directory. |


### file~~like~~ system file interface

These are the methods of a `FileEntry`. Files and directo

|Object Storage File Methods | Return | arguments | Description |
|----|----|----|---|
| open | FileDirectory | ( pathane ) | opens a file within this file as a folder |
| getLength | Number | () | returns the current length of this file |
| read | Promise | ( [pos,] length) | Takes optional parameters (length), or (position, length); returns a promise that resolves with an ArrayBuffer |
| write | Promise | ( ArrayBuffer | String ) | Write this array buffer or string to the file.  Promise resolves with the id of the object written. |

### Filesystem example code.


#### Exmaple 1
``` js
storage.getRoot().then( root=>root.open("filename").catch( ()=>root.create( "filename" ) ).then( (file)=>file.read().then( data=>{
   console.log( "File Data:", data );
} ) ) )
```


#### Example 2

same as above, but using await.

``` js
const data = await (await (await storage.getRoot()).open("filename")).read()
```

#### Example 3

just example 1 reformatted in block code.

``` js
storage.getRoot().then( root=>
	root.open("filename")
		.catch( ()=>
			root.create( "filename" ).then( file=>{
				file.write( "Initial content?" ); 
			} )
		.then( (file)=>
			file.read()
				.then( data=>{
					console.log( "File Data:", data );
				} ) 
			) 
		)

```

### Revision System

A put option can be specified for files to version them, which keeps the history of their image in storage 
exactly where it was.  The same ID when used for `get()` will get the latest version of that record, without
additional options.  The same ID when used for `put()` will save the data specified, and update the timestamp, and
link to the previously data in the time lines; the file will then have multiple times, other than the 
creation and current times. 

When using `map()` the same ID used above will return the oldest version of the record.  References that are updated
with files that have a revision history, will have a time indicator appended to the identifier after a `.` character.
So, a record which references the newst version of a file will have an indicator addtional to the base identifier.

This might be a valid object for example: `{ref[AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=.1]}`.
