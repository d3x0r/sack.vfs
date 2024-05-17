
import {sack} from "sack.vfs"



// 1815, 384
// 615 560 55

const base = {x:1858, y:158};
const base_positions = [
   {x:3620, y:504}, // dim1    0
   {x:3620, y:559}, // dim2    1
   {x:3620, y:619}, // dim3    2
   {x:3620, y:674}, // dim4    3

   {x:3620, y:729}, // dim5    4
   {x:3620, y:784}, // dim6    5
   {x:3620, y:833}, // dim7    6
   {x:3620, y:894}, // dim8    7

   {x:2869, y:461}, // tickspeed         8
	{x:2111, y:414}, // buy amount 1/10   9
   {x:2400, y:1050}, // dimboost         10
   {x:3400, y:1050}, // AM_gal           11
];




let positions;


let click = 0;
const clicks = [
	0, 1, 2,3,
   3,3,3,3,3,3,3,3,3,
	3, 10,

	0, 1, 2,3,4,
   4,4,4,4,4,4,4,4,4,
	4, 10,
   

	0, 1, 2,3,4,5,
   5,5,5,5,5,5,5,5,5,
	5, 10,

	0, 1, 2,3,4,5,6,
   6,6,6,6,6,6,6,6,6,
	6, 10,

	0, 1, 2,3,4,5,6, 9,7,
   7,7,7,7,7,7,7,7,7,
	7, 10, 9,


	0, 1, 2,3,4,5,6, 
	9,7,7,4,2,
   0, 3, 
   7,6,5,4,3,2,1,0,
   0, 1, 2, 1,
	7, 10, 9, 


	0, 1, 2,3,4,5,6, 
	9,7,7,4,2,
   0, 3, 
   7,6,5,4,3,2,1,0,
   0, 1, 2, 1,
	

//	7, 10, 9,

]

const delays = [
    100, 100, 100, 100,

    100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100,

    100, 100, 100, 100,100,
    100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100,

    100, 100, 100, 100,100,100,
    100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100,

    100, 100, 100, 100,100,100,
    100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100,


    100, 100, 100, 100,100,100,100,
    100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100,

    100, 100, 100, 100,100,100,100,
    100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100,

];


let _4down = false;
let clicking = false;
const clickPerSec = 10;
const mouse = sack.Mouse( mouseCallback );
let clickPos = null;

function mouseCallback(event){
	console.log( "Constructor event:", event );
	if( event.buttons & 32 ) {
		if( !_4down ) { 
			clickPos = sack.Mouse.cursor;               	
			_4down = true;
        	clicking = !clicking;
			if( clicking ) {
				positions = base_positions.map( pos=>({x:(pos.x-base.x+clickPos.x),y:(pos.y-base.y+clickPos.y)}) );
				//console.log( "positions:", positions );
				click = 0;
				setTimeout( ()=>{
				doClick(positions[clicks[0]]);
				}, 250 ); // allow time to unclick
			}
		} else {
		}
	} else {
        	_4down = false;
        }
}

function doClick( pos ) {
	if( !pos ) return;
	//const oldPos = sack.Mouse.cursor;
	//const pos = clickPos;//sack.Mouse.cursor;
        console.log( "clickat POS:", pos );
	sack.Mouse.event( pos.x, pos.y, sack.Mouse.buttons.left );
	sack.Mouse.event( pos.x, pos.y, 0 );        
        if( clicking ){
				const n = ++click;
				if( n < clicks.length )
		        setTimeout( ()=>{
						doClick(positions[clicks[n]])
						}, 1000/clickPerSec );
			}
}
