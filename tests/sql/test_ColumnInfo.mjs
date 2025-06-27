import {sack} from "sack.vfs"

const db = sack.DB( "test.db" );
db.makeTable( "Create table test(a int, b int )" );
db.do( "insert into test (a,b) values(1,1)" );
console.log( "Result (though this is a logging level check", db.do( "select a,b notb from test not_test" ) );
