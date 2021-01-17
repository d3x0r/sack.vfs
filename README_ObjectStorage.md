
## Object Storage

(Added from 0.9.145)
Object storage interface is a hybrid combination of virtual file system and JSOX for object encoding.


After making [JSOX](https://github.com/d3x0r/JSOX),
I forked my VFS from a while ago and changed the directory structure so it's a combined radix/remainder of name structure; so as a directory block fills with strings, it finds common ones, and updates a byte-character reference for all the files that start with that... 
I had wanted to do it somewhat more specifically to use 4096 hash which is 2 base64 characters \(except the ==\/\\0 \) at the end... but that ends up being like 16k block minimum; so I revised it to just use 256 entries (based on bytes of the filename).   It has some limitations, like \0 terminates a string; so index 0 will NEVER be used... and actually most control characters aren't in text filenames so hash entries 0-31 and (other than unicode) 127-250/ and definatly 250-255 are unused because those are never bytes in utf8 (using a string 'string' access for file content); but.  It works... generating random hashes `ba63,ab3j,b532` etc eventually overlaps in groups... anyway it's overall O(1) lookup; although that breaks into 10 hash hops (which is 10 pages of memory), plus 1 page for the name fragments.... ) and then each block has 254 'directory entires' (or 53 something 64 bit), which the directory is kept in-order always, so can do a binary search which is 8 string comparisons.... so a total of 10 cache loops, and 8 string comparisons worst case.  (This is in a single C file that compiles on any system with a C library or C++ if you prefer that.)  It can be compiled using emscripten which is about 120k minmum and about 300k total (gzips smaller).  The file system IO for emcc is IndexedDb (but I think I learned that here)....
So then I don't really want a 'file' access to the storage; although it has it, so it could be possible to update segments of blobs, I really intend it to store JS objects... which are stored as a whole and read as a whole (they are encoded to the disk in a container object and JSOX encoded once).  The Object Storage put and get use extended JSOX parser and stringifer that registers a handler for ~os as in `~os"abuasdfu31=="` (object storage identifier).  When it finds one of those, it knows it's a reference to another file in storage.  
If the object is loaded from storage with `get(id)`, and no other objects were loaded, the object would be returned with strings in the places of real objects... the can be manually revived with `obj.fixkey = objStore.get(obj.fixkey)`.  I added a `map(id)` which will also track the reviving objects, read all the files, and result with the completed object exactly as saved.... 
`put(object,bSign )` returns a new ID of the object, unless the object was previously stored or restored.  the optional argument for sigining can generate a nonce and result with a ID that is a PoW for the record and nonce.
records which have a nonce cannot be re-written.  other objects can be updated with their current state.
Oh - (one last note) Added sort to JSOX encoding so objects are always encoded in the same order ( as according to JS `if( stringA > stringB )`


``` js

const sack = require( "sack.vfs" );
const objectStorage = sack.ObjectStorage( 'filename.data' );

```

```
sack.ObjectStorage.
    Thread : {
        post( uniqueId, Volume ) - post a volume to a thread by a unique identifier.
        accept( uniqueId, cb(volume) ) - registers a callback which is called with a volume thrown from some other thread.
    }
```


| Object Storage constructor arguments | Description  |
|----|----|
| filename | Filename to use for object storage database.  If the filename includes '@' the part before the '@' is treated as a mount name; ie. 'vfsMount@object.store'.


| Object methods | Return | Arguments | Description |
|----------|------------|-------------|----|
| getRoot | promise | () | Promise resolves with root directory |
| put | unique ID | ( object \[, options\] ) | Stores or updates an object into storage.  If the object was not previously stored or loaded, a unique ID for the object is returned, otherwise the existing ID results.  The second parameter is an optional option object, see options below. |
| get | promise |  ( [string] or [Option object] ) | Returns a promise; success is passed the object loaded from storage.  Loads an object using the specified ID. |
| map | promise |  ( id ) | Same as get, but also loads any objects of specified ID. |
| -- | | | |
| delete |  | (id/object) | remove object from storage... (danging references in other stored records?) |


| Object get Options names | type | Description |
|-----|-----|-----|
| id | string | use this id for the object storage target |
| extraDecoders | [ { tag:"tag", p:Type } ] | An array of optional object decoder(parse) types to use when getting objects. |


| Object put Options names | type | Description |
|-----|-----|-----|
| id | string | use this id for the object storage target |
| extraEncoders | [ { tag:"tag", p:Type } ] | An array of optional object encoder(stringify) types to use when putting objects. |



 
|Object Storage Directory Methods | Return | arguments | Description |
|----|----|----|---|
| create | FileEntry | (filename) | creates a new file, ready to be written into. |
| open | FileEntry | ( filename ) | opens a file relative to the current root. |
| folder | FileDirectory | ( pathname ) | gets a path within the current path. |
| store | none | () | saves any changes. |
| has | Boolean | ( pathanem ) | returns true/false indicating whether this directory contains the specified pathname. |
| remove | Promise | ( filename ) | delete specified file from the file directory. |


|Object Storage File Methods | Return | arguments | Description |
|----|----|----|---|
| open | FileDirectory | ( pathane ) | opens a file within this file as a folder |
| getLength | Number | () | returns the current length of this file |
| read | Promise | ( [pos,] length) | Takes optional parameters (length), or (position, length); returns a promise that resolves with an ArrayBuffer |
| write | Promise | ( ArrayBuffer | String ) | Write this array buffer or string to the file.  Promise resolves with the id of the object written. |

```
storage.getRoot().then( root=>root.open("filename").catch( ()=>root.create( "filename" ) ).then( (file)=>file.read().then( data=>{
   console.log( "File Data:", data );
} ) ) )

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
