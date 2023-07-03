

var sack = require( "../.." );

var background = sack.Image( "numbers_1-90.png" );

const surface = background.imageSurface();

console.log( surface.slice( 300000 ) );

for( let x = 0; x < surface.length; x+= 4 ) {
	const r = surface[x]/255;
	const g = surface[x+1]/255;
	const b = surface[x+2]/255;
	const a = (r+g+b)/3;
	surface[x+0] = 255;
	surface[x+1] = 255;
	surface[x+2] = 255;
	surface[x+3] = (a*255)|0;
}

console.log( surface );

const png = background.png;
sack.Volume().write( "numbers_1-90a.png", png );

console.log( surface.slice( 300000 ) );

//console.log( "png:",  );


/*
	draw( image ) {
		//console.log ("DRAWE WITH:", this, 0+this.x_del, 10+this.y_del, 10 * this.scale, 10 * this.scale, 0, 0, -1, -1 );
	        image.drawImage( background, 0+this.x_del, 10+this.y_del, 100 * this.scale, 100 * this.scale, 0, 0, -1, -1 );
		//console.log( "Update control surface..." );
		return true;
	},
        */
