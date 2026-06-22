

import {sack} from "sack.vfs"


const devices = sack.sound.devices

for( let dev of devices ) if ( dev.name === "Speakers (Plantronics .Audio 476 DSP)" )
		console.log( "Reset:", dev.reset() );
