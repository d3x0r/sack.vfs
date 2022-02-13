const sack=require( ".." );
const vfs = sack.Volume( null, "breakme.vfs" );
const db = vfs.Sqlite( "broken.db" );
//db.autoTransact(true);
db.makeTable( "create table stuff ( text ) ");

var text = '';
for( var n = 0; n < 80; n++ )
	text += n;
for( var n = 0; n < 8; n++ )
	text += text;
        
console.log( "item:", text.length );

var m = 0;
function tick() {
	if( m < 1000 ) {
		db.do( "insert into stuff (text)values('" + text + "')" );
		m++ ;
		setTimeout( tick, 100 );
	}
}
tick();

