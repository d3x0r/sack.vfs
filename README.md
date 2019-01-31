# sack-gui

[![Join the chat at https://gitter.im/sack-vfs/Lobby](https://badges.gitter.im/sack-vfs/Lobby.svg)](https://gitter.im/sack-vfs/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)[![Build Status](https://travis-ci.org/d3x0r/sack.vfs.svg?branch=master)](https://travis-ci.org/d3x0r/sack.vfs)

Node addon for a lightweight platform independant gui.
Otherwise is the same as *[sack.vfs](https://npmjs.org/package/sack.vfs)*

Vulkan API to be added eventually... 

## Requirements

CMake-js is required to build the GUI; to pull the full external sources.

* see requirements in *[sack.vfs](https://npmjs.org/package/sack.vfs)*

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

#### Mac

  *  (ODBC might be optioned out; just uses sqlite typically)
  *  brew (brew install unixODBC)

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
font.save( "Picked font" );
var font = sack.Image.Font( &lt;font name&gt;, width, height, render flags (0-3 for mono, 4bit, 8bit render) ); 

var font = sack.Image.Font.load( "Picked Font" );

```

When creating a font by name, it can either be the name of a font, or a filename that contains a font.

| Font Method | Parameters | Description | 
|----|-----|----|
| load  | (filename)  | Load a font from a file |


| Font Method | Parameters | Description | 
|----|-----|----|
| measure  | (string)  | /* unfinished; should result with some { width : xx, height: yy } type object */ |
| save  | (filename)  | Save font description to a file |


## Render Methods

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
| load | (filename) | Load a frame from a file |
| Border | (description object) | make a custom border to apply to a frame |

| Frame Methods | arguments | description |
|----|----|----|
| Frame | ( title, x, y, width, height ) | create a new frame over the existing frame.  This is a modal operation... that the existing frame will not get events until this frame is closed. |
| Control | ( controlType, x, y, width, height ) | Create a control within the frame, the controlType is a string specifying the type of the control to be created.  Control types are available in an array as `sack.control.types`. |
| show | () | show the Frame.  Frame is hidden allowing creating controls efficiently before showing the completed dialog. |
| hide | () | hide the frame.  Does not close the frame, just removes it from being displayed. |
| reveal | () | restore a frame.  (may be same as show()?) |
| close | () | close a frame. |
| border | accessor | Set border of frame | 
| font | accessor | Set Font of this control (and all child controls) | 
| focus | () | Set focus to this. |
| save | (filename) | Save this frame to a file |
| get | (control ID) | Get control by text ID | 
| edit | () | Enable/begin editing on thie dialog frame. | 
| close | () | Destroy/close a frame/control. | 
| color | ... | accessor container for all colors of this control (and all child controls |
| ... colors | 
| id | ( [source [, version]] ) | Version indicates which entropy generator to use internally |

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
| focus | () | Set focus to this. |
| get | (control ID) | Get control by text ID | 
| close | () | Destroy/close a frame/control. | 
| color | ... | accessor container for all colors of this control (and all child controls |
|  |  |  |
| (other) | ... | Depending on the control created, various addtional methods may be added.  (A button will get a way to set click event, listbox will have methods to add list items, etc) |
| size | accessor | gets an object {width:#, height:#} which has the current width and height of the control.  Passing a similar object will set the width and height of a control. |
| position | accessor | gets an object {x:#, y:#} which has the current x and y position of the control.  Passing a similar object will set the position of a control. |
| layout | accessor |  gets an object {x:#, y:#, width:#, height:#} which has the current x and y position of the control and also size of the control.  Passing a similar object will set the position of a control and also size of the control. |
| text | accessor | sets/gets the caption/text string of a control.  All controls have this, but not all controls show this. |

## Frame and Control Color names

| Color Name | Usage |
|:-----|-----|
| highlight | used on highlight edges of controls (borders of controls) | 
| normal | Normal background color | 
| shade | used on shadow area of controls (borders of controls) | 
| shadow | used on shadow area of controls (borders of controls) | 
| textColor | Normal Text color used on most controls that have text | 
| caption | The caption of the frame, this is the background | 
| captionText | This is the color of the caption text. | 
| inactiveCaption | When frame is not in focus, background color of caption | 
| InactiveCaptionText | When frame is not in focus, text color used for caption | 
| selectBack | Edit Control; this is the background of selected text | 
| selectText | Edit control; this is the text of selected text | 
| editBackground | edit control; background of edit control | 
| editText | edit control; text of edit control | 
| scrollBarBackground | background color of scrollbar (behind thumb) | 

## Border flags 

| Border Flag (sack.PSI.control.border.(flag) ) | Description
|-----|----|
|normal     | This is 0 value.  It shows a medium thick border. |
|none     | no border at all, surface area of control is full size of control. |
|thin  | a thin border that shows as a raised element |
|thinner  | A 1 pixel border that shows as a raised element |
|dent | A grooved border, fall outside, rise inside |
|thinDent | A thiner grooved border, fall outside, rise inside |
|thickDent | A wider grooved border, fall outside, rise inside |
|user | User drawn; requires additional callbacks to be registered |
|invert | A 4 pixel frame that is inset instead of raised |
|invertThinner | A 1 pixel frame that is inset instead of raised |
|invertThin | A 2 pixel frame that is inset instead of raised |
|bump | a bump frame (rise outside, fall inside) |
|caption | Show caption for control (title bar) |
|noCaption | no caption (title bar) | 
|noMove | do not allow movement of the frame |
|close | has close button? |
|resizable | can resize this Frame.  (Only applies for frame border) |
|within | This is used with `Frame` controls, this allows a frame to be within this (modifies size behavior)
|wantMouse | want to get mouse events for control, even if normally they are filtered |
|exclusive | ? |
|fixed | Control is not re-scaled because of font changes |
|noExtraInit | Don't call extra init or extra destroy  (internal use only?) |
|captionCloseButton | add caption close button |
|noCaptionCloseButton | make sure there's no caption close button |
|closeIsDone  | When close is clicked, trigger done instead of destroy |


## Border Anchor Flags

If the required border to draw is larger than the image, the image is stretched to cover the area.

| Anchor Flag (sack.PSI.control.borderAnchor.(flag) ) | Description |
|------|------|
|topMin     | top edge is anchors so left side is 0, and border chops right side. |
|topCenter | top edge is anchored at center, and border chops left and right sides.|
|topMax | top edge is anchored at right, left side is chopped |
|leftMin | left edge, anchored at top, chop bottom |
|leftCenter| left edge, anchored at center, chop bottom and top |
|leftMax| left edge, anchored at bottom, chop top |
|rightMin | right edge, anchored at top, chop bottom |
|rightCenter| right edge, anchored at center, chop bottom and top |
|rightMax| right edge, anchored at bottom, chop top |
|bottomMin     | bottom edge is anchors so left side is 0, and border chops right side. |
|bottomCenter | bottom edge is anchored at center, and border chops left and right sides.|
|bottomMax | bottom edge is anchored at right, left side is chopped |




### Specific control type extensions

Builtin controls have various methods that are associated with them.  Such as a button's click event, which no other control has.

#### Button Controls (Image, Normal, Custom )

| Control Methods | arguments | description |
|----|----|----|
| click | (cb) | Set callback to be triggerd when button is clicked |
| on | (event,cb) | Set Event handler for button.  Suppoprted events (click).

#### Check/Radio Button Controls

| Control Methods | arguments | description |
|----|----|----|

#### Scroll Bar Controls

| Control Methods | arguments | description |
|----|----|----|

#### Edit Field

| Control Methods | arguments | description |
|----|----|----|
| password | &lt;accessor&gt; | Set password attribute of edit control (show ****) |

#### List Box

| Control Methods | arguments | description |
|----|----|----|
| addItem | (string) | Add a string item to listbox; results with an object representing the item |
| removeItem | (item) | Remove an item from a listbox |
| setTabs | (number array) | set position array for tab stops in listbox |
| measure | (string) | measure the length of an item in the listbox |
| header | accessor - setonly | set the header of a listbox |
| hScroll | (enable, max) | Enable parameter is a boolean (true/false).  Max sets the max scroll length of the bar (use measure item?) |
| onSelect | (cb) | set callback to be triggered when an item in listbox is selected; it is passed the selected item |
| onDoubleClick | (cb) | set callback to be triggered when an item in listbox is double clicked; it is passed the selected item |

#### Sheet Control (tabbed pages)

| Control Methods | arguments | description |
|----|----|----|
| addPage | (title, frame) | Add a page to a sheet control |

#### Progress Control

| Control Methods | arguments | description |
|----|----|----|
| range | accessor | Set the max range of progress |
| progress | accessor | Set the current progress (within 0->range) |
| colors | (a,b) | Set colors of the progress bar |
| text | accessor | boolean; sets whether to show the text percentage or not |

#### Clock Control

| Control Methods | arguments | description |
|----|----|----|
|analog| () | Enable analog mode for clock control (instead of digital text) |


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

| InterShell Object Methods | description |
|----|----|
| setSave | set callback for onSave(global info) |
| setLoad | set callback for onLoad( globalinfo?") |
| start | show intershell surface |
| Button | (name) register a button method for creating buttons |
| Control | (name) register a control |
| Custom | (name) register a custom control |


| InterShell Button Methods | description |
|-----|-----|
| setCreate | set callback for when button is created (return false to prevent creation) |
| setClick | set callback handler for when button is clicked |
| setSave | callback for when button is saved |
| setLoad | callback for when button is loaded |


| InterShell Button Instance Methods | description |
|-----|-----|
| setTitle | Set text shown on button |
| setStyle | set button style name |
| setTextColor | set color for button text |
| setBackground | set primary background color |
| setSecondar | set seconary background color |



| InterShell Custom Control Methods | description |
|-----|-----|
| setCreate | set callback for when button is created (return false to prevent creation) |
| setSave | callback for when button is saved |
| setLoad | callback for when button is loaded |


| InterShell Custom Control Instance Methods | description |
|-----|-----|
| setTitle | Set text shown on button |


| InterShell Control Methods | description |
|-----|-----|
| setCreate | set callback for when button is created (return false to prevent creation) |
| setSave | callback for when button is saved |
| setLoad | callback for when button is loaded |


| InterShell Control Instance Methods | description |
|-----|-----|
| setTitle | Set text shown on button |



---

## Changelog
- 0.9.122 - Release work in progress update; fixed link to other project.
- 0.9.121 - 
- 0.9.120 - Add listbox methods.  Make control color accessors a templated object instead of adding an object with method extensions.  
- 0.9.119 - Fix missing websocket client event accessors.  Add custom border support.  
- 0.9.118 - Update documentation and keywords. Fix building. 
- 0.9.117 - Fork from sack.vfs 0.9.117.  Initial publication to NPM.
