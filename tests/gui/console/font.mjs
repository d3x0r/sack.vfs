import {sack} from "sack-gui"

const displays = sack.Task.getDisplays();
const disp = displays.device.find( d=>d.primary );

const w = disp.width;
const h = disp.height;


export default {
	vw(n) { return n/w*100 },
	vh(n) { return n/h*100 },
}