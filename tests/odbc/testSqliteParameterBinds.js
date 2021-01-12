
const sack = require( ".." );
const sqlite = sack.Sqlite( "test.db" );

if( sqlite.makeTable( "create table users ( name char, password char, deleted integer ) " ) )
	console.log( "Made?" );
        
        sqlite.do( "insert into users (name,password,deleted) values ('userName','0123456',1 )" );

	console.log( sqlite.do( "select * from users" ) );

    var passHash = "0123456";
    sqlite.do( "select * from users where name=","userName","and password=", passHash );

    sqlite.do( "select * from users where name=? and password=?", "userName", passHash );
    sqlite.do( "select * from users where name=?2 and password=?1", passHash, "userName" );

    sqlite.do( "select * from users where name=$user and password=$pass", { $user:"userName", $pass: passHash } );
    
    sqlite.do( "select * from users where name=$user and password=$pass and deleted=?", { $user:"userName", $pass: passHash }, true );
