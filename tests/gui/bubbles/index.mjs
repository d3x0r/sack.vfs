
import {sack} from "sack.vfs"



const imageNames = ["117.png","118.png","119.png","120.png","121.png","123.png"]
const images = imageNames.map( sack.Image );

var display = sack.Renderer.getDisplay( 0 );
console.log( "display 0:", display );
//for( var n = 1; n < 8; n++ ) {
//	console.log( "display %d:", n, sack.Renderer.getDisplay( n ) );
//}
var r = sack.Renderer( "test", 0, 0, display.width, display.height );
console.log( "created renderer?", r, Object.keys( Object.getPrototypeOf(r)) );




const l = { shadow : null,
	 shaded : null,
	 cover : null,
	w:1920,h:1080,


	base:null,dest:null,current:null,
	bubbles:[],
	lastTick : 0
};

const wd = images[0].width;
const ht = images[0].height;

class Bubble {
	base = [0,0,0,0];
dest=[0,0,0,0];
current=[0,0,0,0];
length=0;
step=0;
x=0;
y=0;
over=true;
delx = 0;
dely = 0;
r=0;
grn=0;
b=0;
//cover = ;
//ctx = this.cover.getContext("2d" );

explode = false;
explodeAt = 0;
explodeUntil = 0;
explodeTo = {x:0, y:0};

	constructor() {
	}

	updateColor( image, r,g,b ) {
		
	}


}



function ChooseColorDest( bubble )
{
	if( bubble.step >= bubble.length )
	{
		bubble.base = bubble.dest;
		bubble.step = 0;
		bubble.length = ( Math.random() * 50 ) + 1;
		bubble.dest = [Math.random() * 256 
									, Math.random() * 256 
									, Math.random() * 256 
									, 255 ];
	}
	bubble.current = ColorAverage( bubble.base, bubble.dest
											, bubble.step++, bubble.length );
	bubble.r = (bubble.current[0] |0)+ (bubble.current[1]|0)*256 + (bubble.current[2]|0)*256*256 + 0xff000000; 
	bubble.grn = bubble.r;
	bubble.b = bubble.r;


//	console.log( "color:", bubble.r.toString(16), bubble.grn, bubble.b );
	//bubble.updateColor( l.cover, bubble.current, bubble.current,bubble.current );
}

const Avg=( c1, c2, d, max )=>((((c1)*(max-(d))) + ((c2)*(d)))/max);

// where d is from 0 to 255 between c1, c2
function ColorAverage( c1,  c2, d,  max )
{
 let r, g, b, a;
  r = Avg( c1[0],  c2[0],   d, max );
  g = Avg( c1[1], c2[1], d, max );
  b = Avg( c1[2],  c2[2],  d, max );
  a = Avg( c1[3], c2[3], d, max );
  return [r,g,b,a];
}


function  MoveBubbles(  perSec )
{
	l.bubbles.forEach( bubble=>{
      bubble.x += bubble.delx * perSec;
		bubble.y += bubble.dely * perSec;
		if( !bubble.delx || (bubble.x > l.w) )
         bubble.delx = -((Math.random() * 120 )+1);
		if( !bubble.dely || (bubble.y > l.h) )
         bubble.dely = -((Math.random() * 120 )+1);
		if( bubble.x < -150 )
			bubble.delx = ((Math.random() * 120 )+1);
		if( bubble.y < -150 )
			bubble.dely = ((Math.random() * 120 )+1);

	} );
}

function DrawBubbles( image, now, bubble, x,  y,  c )
{
	if( bubble.explode ) {
		const del = (now - bubble.explodeAt)/(bubble.explodeUntil-bubble.explodeAt);
		if( now > bubble.explodeUntil ) {
			bubble.explode = false;
			bubble.x = bubble.explodeTo.x;
			bubble.y = bubble.explodeTo.y;
		} else {
			/*
			ctx.beginPath();
			ctx.strokeStyle = "red";
			const del0 = del > 0.1?del-0.1:0;
			ctx.moveTo( bubble.x * (1-del0) + bubble.explodeTo.x * del0, bubble.y * (1-del0) + bubble.explodeTo.y * del0 );
			ctx.lineTo( bubble.x * (1-del) + bubble.explodeTo.x * del, bubble.y * (1-del) + bubble.explodeTo.y * del );
			ctx.stroke();
			ctx.beginPath();
			ctx.strokeStyle = "yellow";
			ctx.moveTo( Math.random() * 10 - 5 + bubble.x * (1-del0) + bubble.explodeTo.x * del0
			          , Math.random() * 10 - 5 + bubble.y * (1-del0) + bubble.explodeTo.y * del0 );
			ctx.lineTo( Math.random() * 10 - 5 + bubble.x * (1-del) + bubble.explodeTo.x * del
			           , Math.random() * 10 - 5 + bubble.y * (1-del) + bubble.explodeTo.y * del );
			ctx.stroke();
			*/
		}
	} else {
		//console.log( "Draw at:", x, y );
	if( x > 1920 || y > 1080) return;
      image.drawImageOver( l.cover, x,y,150,150, 0, 0, wd, ht );
      image.drawImageMS( l.shaded, x,y,150,150, 0, 0, wd, ht, bubble.r,bubble.grn,bubble.b );
		let ux = ( x < 0 )?0:(x>1920)?0:x;
		let uy = ( y < 0 )?0:(y>1080)?0:y;
		let uw = ( x<0)?(150+x):x > (1920-150)?150-(1920-x):150;
		let uh = ( y<0)?(150+y):y > (1080-150)?150-(1080-y):150;
//		if( uw > 0 && uh > 0 ) 
//		   r.update( ux,uy,uh,uw);
	}
}

const color = sack.Image.Color( 0 );

const redraw = ( image )=>{	
	const tick = performance.now();
	//console.log( "draw tick:", image, tick, l );
	MoveBubbles( (tick-l.lastTick)/1000 );
	l.lastTick = tick;
   image.fill( 0, 0, image.width, image.height, sack.Image.colors.purple );
	//ctx.clearRect( 0, 0, canvas.width, canvas.height );
	l.bubbles.forEach( bubble=>{ ChooseColorDest( bubble ); DrawBubbles( image, tick, bubble,bubble.x-75,bubble.y-75,bubble.current ) } );
	//requestAnimationFrame( UpdateImage );

	//image.fill( 0, 0, 0, 0, sack.Image.Color( 0x7f7f7f7f ) );
	if(1)
	for( let i = 0;i < 255; i++ ) {
		color.a = i;
		color.g = 0;
		image.line( 0, i, 127, i, color );
		color.g = 128;
		image.line( 128, i, 256, i, color );
	}

  //    image.drawImageOver( l.shaded );
  //    image.drawImageOver( l.cover );

	r.update();
	//return true;
}

r.setDraw( redraw );
//r.on( "draw", redraw )


	l.shadow = images[0];
	l.cover = images[5];
	l.shaded = images[4];

			for( let n = 0; n < 40; n++ )
			{
				const bubble = new Bubble();
				bubble.x = n * 160;
				while( bubble.x > 1650 )
				{
					bubble.x -= 1650;
					bubble.y += 160;
				}
				l.bubbles.push( bubble );
			}

	r.show();
function animate() {
	r.redraw();
	setTimeout( animate, 5 );
}
animate();


r.on( "mouse", (evt )=> {
	const mx = evt.x;
	const my = evt.y;
	const bub = l.bubbles.find( (bub)=>{ const dx = mx - bub.x, dy = my-bub.y; if( (dx*dx+dy*dy) > 75*75 ) return false; return true; } );
	if( bub ) {
		bub.explode = true;
		bub.explodeAt = l.lastTick;
		bub.explodeUntil = l.lastTick + 500;
		bub.explodeTo.x = Math.random() * 1920;
		bub.explodeTo.y = Math.random() * 1080;
	}
} );

