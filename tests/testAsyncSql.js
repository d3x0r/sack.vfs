var vfs = require( ".." );
//var db = vfs.Sqlite( "file://localhost/gun.db?nolock=1" );   
//open:file://localhost/gun.db?nolock=1
//Sqlite3 Err: (14) os_win.c:42795: (2) winOpen(M:\gun.db) - The system cannot find the file specified.


var db = vfs.Sqlite( "file:gun.db?nolock=1&mode=rwc" );

async function test() {

 	let r;
        try {
		// created an error
	        const p = db.run( "select 1+1zz" );
	        	console.log( "Got promise, right? 1", p );
                r = await p;
        } catch( err ) {
        	console.log( "Got error, right? 1", err );
        }

        try {
	        const p = db.run( "select 1+1" );
        		console.log( "Got promise, right? 2", p );
            r = await p;
        } catch( err ) {
        	console.log( "err?", err );
        }
        //db.close();
        console.log( "result?", r );

}

test();
