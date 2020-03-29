

var sack = require( "../.." );
sack.Volume().unlink( "test.db" );
//var db = sack.Sqlite( "Mysql" );
var db = sack.Sqlite( "test.db" );

db.do( "create table if not exists t1(id int);" );
db.do( "create table if not exists t2(id int, t1Id int);" );
db.do( "insert into t1 values (0);" );
db.do( " insert into t2 values (1,0);" );
console.log( db.do("select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id;") );
console.log( db.do("select x1.id, x2.id from t1 as x1 join t2 as x2 on x1.id = x2.t1Id;") );

//id|id
//0|1
try {
console.log( db.do("select * from (select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id) t;") );
} catch(err) {
console.log( "Error:", err );
}

if(0) {
db.do( "pragma short_column_names=on" );
console.log( db.do("select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id;") );

//id|id
//0|1
console.log( db.do("select * from (select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id);") );

//id|id:1
//0|1

db.do( "pragma short_column_names=off" );
db.do( "pragma full_column_names=on" );
console.log( db.do("select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id;") );

//id|id
//0|1
console.log( db.do("select * from (select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id);") );


db.do( "pragma short_column_names=on" );
db.do( "pragma full_column_names=on" );
console.log( db.do("select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id;") );

//id|id
//0|1
console.log( db.do("select * from (select t1.id, t2.id from t1 join t2 on t1.id = t2.t1Id);") );
}



db.do( "drop table  t1;" );
db.do( "drop table  t2;" );
