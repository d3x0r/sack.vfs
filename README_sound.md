

# sack.sound

This is an interface to sounds.  Can play a sound using ffmpeg and openal. May be able to preload sounds for caching.

## Windows Sound Devices

``` js

// get an array of current devices.
const devices = sack.sound.devices;

// then each device has
for( let device of devices ) {
	// {
	// 	name : string,
	//    id : string,
	//    volume : get/set number  // returns the current volume or sets the current volume level
	//    default : true/false   // indicates if device was current when the list was retrieved.
	//    communications : true/false   // indicates if device was the current communications device.
	//    console : true/false   // indicates if device is default windows system sound device.
   //    setDefault()   // sets the default sound device (moves console and default)
	// }
}

```



## Play a Sound

``` js

// this plays a file from the local filesystem.
sack.sound.play( "filename" );

```

