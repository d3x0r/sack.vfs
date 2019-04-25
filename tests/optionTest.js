
var vfs = require( '..' )

var db = vfs.Sqlite( 'option.db' );
var root = db.go( "test" );
var tmp;
var tmp2;
var path = ( tmp = root.go( "a" ).go( "b" ).go( "c" ) );

console.log( "option value is : ", tmp.value, tmp, Object.keys(tmp), Object.getPrototypeOf( tmp ) );

tmp.value = "Default Other Value";

console.log( "Initial.. no defaults...", ( tmp2 = root.go( "a" ).go( "b3" ).go( "c2" ) ).value );
console.log( "option value is : ", tmp.value, tmp, Object.keys(tmp) );

tmp2.value = "Set Value";
console.log( "after setting value..",  tmp2.value );
console.log( "after setting value..", ( tmp2 = root.go( "a" ).go( "b3" ).go( "c2" ) ).value );
//setTimeout( ()=>{console.log( "Wait a second to flush?") }, 1000 );

var level =0;
function dumptree(opt, name){
	if( name === "." ) return;
	var leader = "";
	for( var n = 0; n < level;n++ ) leader += "   ";
	console.log( leader + "", name, "=", opt.value );
        level++;
        opt.eo( dumptree );
        level--;
}
db.eo( dumptree )

console.log( "global option database ---------------" );
level =0;
vfs.Sqlite.eo( dumptree )


function tick() {
	console.log( "dump tables: \n"
        	, db.do( "select * from option4_map" ) 
                );
}
tick();