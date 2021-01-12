const sack = require( ".." );

sack.sound.play( "emerald.wav" );

setTimeout( ()=>sack.sound.play( "ruby.wav" ), 1000 );
setTimeout( ()=>sack.sound.play( "diamond.wav" ), 2000 );
setTimeout( ()=>sack.sound.play( "on hit 4.wav" ) , 3000);

setTimeout( ()=>console.log( "Done" ), 4000 );
