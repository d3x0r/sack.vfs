# sack-gui

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)[![Build Status](https://travis-ci.org/d3x0r/sack.vfs.svg?branch=master)](https://travis-ci.org/d3x0r/sack.vfs)

Node addon for a lightweight platform independant gui.
Otherwise is the same as [sack.vfs](https://npmjs.org/packages/sack.vfs)

Vulkan API to be added eventually... 

## Requirements
#### npm
	// don't use these anymore... but rather internal macros
	// because I default all methods as readonly
	// all objects exported by require are constants.
        // nan (Native Abstractions for Node.js)

#### Centos/linux
        yum install gcc-c++ libuuid-devel unixodbc-devel
        
#### Mac
        (ODBC might be optioned out; just uses sqlite typically)
	brew (brew install unixODBC)

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



### Config Methods

   Configuration file processing; for processing formatted statements in text configurations.  Can
also be used to process streaming text input and responding with triggers based on configuration.
Things like a log analyzer can be created using this interface.

```
var sack = require( "sack.vfs" );
var processor = sack.Config();
processor.add( "some line with %i words", (n) => console.log( "found line param:", n ); );

//processor.go( "filename.txt" ); // reads configuration from a file...
processor.write( "Some sort of file streaming data" );
processor.write( "some line with 33 words" );


```

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

# GUI Interface objects

Building GUI extensions use 'npm run build-gui' or 'npm run build-gui-debug'  Which will download SACK from Github and build against the full library instead of a partial amalgamation.

```
var sack = require( 'sack.vfs' );
```

| Interface | Description |
|-----|------|
| Image | Handles loading, creating, and simple image manipulation methods. |
| Image.Color | namespace to create color objects to interact with display.  Internally, depending on the driver, format changes from ARGB to ABRG potentially. |
| Render | A way to open a window which can display images |
| Intershell | Interactive shell, higher level application thing that allows dynamic page layout configurations |
| Frame | Entry point for GUI controls |
| Registration | Namespace for defining new control types which can be created on Frames. |

## Image Methods

```
var imageFromFile = sack.Image( "image file.jpg" ); // handles bmp, gif, png, jpeg formats.
var imageFromBuffer = sack.Image( UInt8Array or TypedArray or ArrayBuffer ); // attempts to decompress image data contained in passed buffer.
var imageRaw = sack.Image( 100, 100 ); // creates a new image of specified width, height
var imageSubImage = sack.Image( 100, 100, otherImage, 10, 10 ); // creates new image of width, height starting at x, y in otherImage.

```

Image constructor can be passed either a string which is treated as a filename and the specified image is loaded.  It can be passed a binary buffer
which is treated as the contents of a file and is attempted to be decoded.  Specifying a width and height alone will create a new blank image.
A sub image can be created in another existing image by specifying the image and x, y position to start at.  This last operation can be done 
by using the Image method on an existing image also.


| Image Methods | arguments | description |
|-----|-----|----|
| Image | (x, y, width, height ) | Creates a image within the current image.  Operations on this image are clipped within the region specified |
| reset | () | Clear an image to transparent black (0,0,0,0) |
| fill | ( x, y, width, height, color ) | fill a region of an image with a simple color.  |
| fillOver | ( x, y, width, height, color ) | fill a region of an image with a simple color; if color is semi-transparent applies color over existing image data.  |
| line | ( x1, y1, x2, y2, color ) | Draw a straight line in specified color.|
| lineOver | ( x1, y1, x2, y2, color ) | Draw a straight line in specified color.  If color is semi-transparent, applies color alpha over existing image data.|
| plot | ( x, y, color ) | set a pixel on the output image to specified color. |
| plotOver | (x, y, color ) | set a pixel on the output image to specified color.   If color is semi-transparent, applies color alpha over existing image data.|
| drawImage | ( image [, x, y [, xAt, yAt [,width, height]]] ) or ( image [, x, y, width, height, xAt, yAt , source_width, source_height ) | Sets the specified image into the 'this' image.  Draw at x, y (0,0 if not specified), starting at position (xAt, yAt) in the source IMage (0,0 if not specified), for a width and height specified (otherwise full image width, height.  If all parameters are specified, scales the portion of the source image to the target image.  )
| drawImageOver | (same as drawImage) | Same as drawImage, except applies alpha in source image to overlay the new image over the existin content in image. |
| imageSurface | () | Gets a uint8arry which is the underlaying image buffer.  PIxel format is typically [r,g,b,a].  If a lot of custom pixels are to be updated, this is far more efficient to update than using plot or plotOver methods.| 
| png | read-only accessor | Gets the image data as an ArrayBuffer, converted using PNG compression.  The quality of compression can be controlled using the jpgQuality accessor. |
| jpg | read-only accessor | Gets the image data as an ArrayBuffer, converted using JPEG compression.  The quality of compression can be controlled using the jpgQuality accessor. |
| jpegQuality | integer between 0-100, default is 78 | This controls the quality factor of jpeg compression |
| width | readonly accessor | Gets image width |
| height | readonly accessor | Gets image height |

### Color methods

```
var colorFromInteger = sack.Image.Color( 0xFF995522 ); //creates a color from specifed integer 0xAARRGGBB
var colorFromObject = sack.Image.Color( { a: 255, r:128, g: 64, b: 100 } ); // creates color from specified parts.  Clamped to 0-255.
var colorFromObjectFloat = sack.Image.Color( { a: 0.9, r:0.5, g: 0.25, b: 0.37 } ); // creates color from specified parts.  Clamped to 0-255.  The value 1, unfortunatly is treated as integer 1 for the above 0-255 value.
var colorFromString = sack.Image.Color( "Some string" ); // Unimplemented.
var colorFromDialog = sack.Image.Color.dialog( callback );  // callback is passed a color object representing the picked color
```

There are some constant colors already builtin in Image.colors.[color name]

| Color accessors | description |
|-----|-----|
| r | get/set red channel |
| g | get/set green channel |
| b | get/set blue channel |
| a | get/set alpha channel |

| Color Methods | arguments | description |
|---|---|---|
|  toString| () | gets color in a string format |


### Font methods

```
var font = sack.Image.Font.dialog( callback );  // callback is passed a font object represenging the font selected 
var font = sack.Image.Font( &lt;font name&gt;, width, height, render flags (0-3 for mono, 4bit, 8bit render) ); 
```

When creating a font by name, it can either be the name of a font, or a filename that contains a font.


| Font Method | Parameters | Description | 
|----|-----|----|
| measure  | (string)  | /* unfinished; should result with some { width : xx, height: yy } type object */


##Render Methods

Render constructor takes several optional parameters. 

( title [, x, y [, w, h [, parent Renderer]]] ) 

TItle is a text string set as the window's caption/title string.  X and Y control where the window is opened (-1, -1 uses a system default).
W and H control the width and height of the window (-1, -1 uses a system default).  parent renderer is a renderer that this one is opened over.
This should guarantee stacking order, and is semi-modal, that events will not be sent to the parent renderer while the child is open.

| Renderer Methods | Arguments | description |
|----|----|----|
| getImage | () | returns a sack.Image which is the surface of this renderer |
| setDraw | ( callback ) | callback function is passed a sack.Image which can be drawn on.  Once this function returns, the content is updated to the display. |
| setMouse | (callback) | callback function is passed an event object containing { x, y, b } which is the mouse event x, y position within the surface of this renderer, and the button events.  Button values are defined in  sack.button.[left/right/middle/scroll_down/scroll_up]. |
| setKey | (callback) | callback function is passed an encoded integer value of the key.  (needs more work to improve interfacing) |
| show | () | show the renderer.  This must be called before the window will be displayed. |
| hide | () | hide the renderer.  Does not close the renderer, but stops showing it on the display. |
| reveal | () | reveal the renderer.   Meant to be used after hide() to restore it to the screen.  (might also be show()? ). |
| redraw | () | trigger an event to the renderer which will invoke the draw callback specified. |
| update | () or (x, y, width, height ) | Specify the position of the display which is meant to be updated.  Can be used within the draw callback function for partial updates.  If no arguments are specified, full display is updated.|
| close | () | Close the renderer.  Releases all resources for the renderer.  Ends event loop for display, so application can exit. |
| on | ( event Name, callback ) | Set events by name, "draw", "mouse", "key" setting the specified event to the function specified. |

## Frame Methods

These are higher level interface that creates a frame with a caption using Renderer and Image methods underneath.  First, create a frame...

```
var frame = sack.Frame( "Title", x, y, width, height );
```
Frame constructor requires a title.  If x, y are not specified, (0, 0) is used, and (1024,768) is used for width and height.

| Frame Methods | arguments | description |
|----|----|----|
| Frame | ( title, x, y, width, height ) | create a new frame over the existing frame.  This is a modal operation... that the existing frame will not get events until this frame is closed. |
| Control | ( controlType, x, y, width, height ) | Create a control within the frame, the controlType is a string specifying the type of the control to be created.  Control types are available in an array as `sack.control.types`. |
| show | () | show the Frame.  Frame is hidden allowing creating controls efficiently before showing the completed dialog. |
| hide | () | hide the frame.  Does not close the frame, just removes it from being displayed. |
| reveal | () | restore a frame.  (may be same as show()?) |
| close | () | close a frame. |

| Control Methods | arguments | description |
|----|----|----|
| Control | (controlTypes, x, y, width, height ) | creates a control within this current control. |
| createControl | (controlTypes, x, y, width, height ) | creates a control within this current control. |
| createFrame | creates a new frame over the current control/frame.  Showing this will be modal. |
| show | () | show a control |
| hide | () | make a control hidden. |
| reveal | () | reveal a hidden control. |
| redraw | () | trigger an update to a control. |
| close | () | (missing) |
|  |  |  |
| (other) | ... | Depending on the control created, various addtional methods may be added.  (A button will get a way to set click event, listbox will have methods to add list items, etc) |
| size | accessor | gets an object {width:#, height:#} which has the current width and height of the control.  Passing a similar object will set the width and height of a control. |
| position | accessor | gets an object {x:#, y:#} which has the current x and y position of the control.  Passing a similar object will set the position of a control. |
| layout | accessor |  gets an object {x:#, y:#, width:#, height:#} which has the current x and y position of the control and also size of the control.  Passing a similar object will set the position of a control and also size of the control. |
| text | accessor | sets/gets the caption/text string of a control.  All controls have this, but not all controls show this. |

### Control Registration

New controls can be created by specfiying a string name for the control type, and setting up event callback functions.

```
var newControl = sack.Registration( { name : "custom Control", draw(Image) { /* handle draw */ }, mouse( event ) { /* handle mouse event } } );
```

There are a couple dozen events actually available, but only a handful have been implemented.   These are all specified in the option
object passed to the control Registration constructor.

| Registration Object Fields | description |
|----|----|
| name | string used to create a control of this type |
| width | default width of the control when created through frame editor (not yet exposed) |
| height | default height of the control when created through frame editor (not yet exposed) |
| default_border | the default border attributes of the control.  Border attribute values are in `sack.control.border` |
| create | a function called when the control is created.  Return true/false to allow the control to be created or not. |
| draw | a function called when a control needs to draw.   sack.Image type is passed to the function. |
| mouse | a function called when a mouse event happens.  A mouse event object with {x, y, b} is passed.  coordinates are relative to the surface of the control (within the border of the control) |
| key | a function called when a key event happens.  A encoded key event is passed.  (needs better interfacing) |
| destroy | A function called when the control has been destroyed/closed |
| | |
| Others | as required these will be implemented... focus, touch, property page handlers, moved, sized, load, save....  |

## Intershell Interface

Mostly unimplemented, more of a place holder than functional.


---

## Changelog
- 0.9.118 - 
- 0.9.117 - Fork from sack.vfs 0.9.117.  Initial publication to NPM.
