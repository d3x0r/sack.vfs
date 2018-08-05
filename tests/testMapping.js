var sack=require(".." );
var start = Date.now();

var i = 0; 
function tick() {


sack.Volume.mapFile( "data.vfs"
, ()=>{
	if( i< 1000 ) { console.log( "tada:", i,  Date.now() - start ); start = Date.now(); }
	i++;
	if( ( i % 10000 )==0) console.log( i );
	if( i < 1000000 )
		tick();
} );

}
tick();
