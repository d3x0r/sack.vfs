
var sack = require( 'sack.vfs' );
var https = sack.HTTP;

const results = [];

function getPage( addr, port ) {
		var optsxx = {  hostname:  //'216.58.192.142',
                			 addr,//'google.com',
					  port : port || 443,
					  method : "POST",
					timeout:500,
				content: "args=true",
					  ca : null,
					  rejectUnauthorized: true,
					  path : "/test/test.php",
					headers: {
						"Content-Type":"application/x-www-form-urlencoded",
					}
                                          //, agent : false
					};
		var opts = {  hostname:  //'216.58.192.142',
                			 addr,//'google.com',
					  port : port || 443,
					  method : "PUT",
					timeout:500,
					  ca : null,
					ssl:true,	
					  rejectUnauthorized: true,
					  path : "/test/test.php?args=true",
					headers: {
						"Content-Type":"application/x-www-form-urlencoded",
					}
                                          //, agent : false
					};

		var res = https.get( opts );
      if( res.error ) {
      	// error
		console.log( "error happend?" );
      }
      else
		{
			const statusCode = res.statusCode;
			const contentType = res.headers['content-type'] || res.headers['Content-Type'];
			let error;
			//console.log( "https get response happened...", contentType, res);
			if (statusCode === 301) { 
				return getPage( res.headers.Location.substring( 8, res.headers.Location.length-1 ), port );	
			}
			else if (statusCode !== 200) {
				error = new Error(`Request Failed.\n` +
						`Status Code: ${statusCode}` + JSON.stringify( opts ) );
			} else if (/^text\/javascript/.test(contentType)) {
				evalCode = true;
			} else if (/^application\/javascript/.test(contentType)) {
				evalCode = true;
			} else if (/^application\/json/.test(contentType)) {
				evalJson = true;
			} else if (/text\/html/.test(contentType)) {
				evalJson = false;
			}
			else {
				error = new Error(`Invalid content-type.\n` +
								`Expected application/json or application/javascript but received ${contentType}`);
        
			}
			if (error) {
				results.push( error.message );
			}
        
		}
}
//getPage( "google.com" );
for( let i = 0; i < 500; i++ )
	getPage( "10.173.0.1", 443 );
		
console.log( "completed" );
