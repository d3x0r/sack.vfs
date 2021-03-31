import {sack} from "sack.vfs"


var http = sack.HTTP;


//import {url} from "url";
class AxiosX {
    constructor() {
    }
    async post( url, content, config ){
        const opts = {};
        if( content ) {
            console.log( "content:", typeof content );
               if( "Object" === typeof content ) {
                   Object.assign( opts, content );
                   content = null;
               }else
                   Object.assign( opts, config );
        }
        url = new URL( url );
	opts.method = "POST";
        opts.path = url.pathname;
       	opts.port = opts.port || url.port;
        opts.hostname = opts.hostname || url.hostname;5
        opts.content = content;
        opts.url = url;
        console.log( "request:", opts );
	const res = http.get( opts );
        console.log( "Request response:", res );
        return res;
    }
}


const axios = new AxiosX();

test();

async function test() {
  //console.table(args);
  const postURL = 'http://localhost:8080/mavin_dev/backend/public/index.php/api/v1/user/login';
  
  // not sure if this changes, they were in 3 different config files
  const headers = { 
    "CustomerApiKey": "Kroau0N7puR0YGgUe3m7MKo3SBuCcfeuIwu0MADqpkrhkCP5jRo3vHnKcDKK",
    "AppVersion": "23",
    "AppType": "User",
  };

  const result = await axios.post(
    postURL,
    "Post Content Body",
    {
    	"headers": headers
    },
  )
  //return result.data
}



/*
		for( var i = 0; i < 10000; i++ ) {
		var opts = {  hostname:  //'216.58.192.142',
                			 '::1',
					  //port : 443,
					  method : "POST",
                                          //, agent : false
                        		headers:
					};
		var res = http.get( opts );
		console.log( "Result:", res );

*/
