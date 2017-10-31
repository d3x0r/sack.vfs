
var sack = require( ".." );
var JSON6 = sack.JSON6;



try {
	var result = JSON6.parse( "{ my_ key:3}" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ my _  key:3}" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ my_key { failure:true}:3}" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ { my_key:3 } }" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ [ my_key:3 } }" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ my_key[:3 } }" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ my_key]:3 } }" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ my_key [:3 } }" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

try {
	var result = JSON6.parse( "{ my_key ]:3 } }" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}



var result = JSON6.parse( "{ 'my  _  key':3}" );
console.log( "result:", result );

try {
	var result = JSON6.parse( "{ my_key\\m&m+*|:3}" );
	console.log( "result:", result );
} catch( err ) {
	console.log( 'error expected:', err.message );
}

var result = JSON6.parse( "{ mykey //test \n :3}" );
console.log( "result:", result );

var result = JSON6.parse( "{ mykey /*test */ :3}" );
console.log( "result:", result );


/*

{ my- key:3} // fault
{ my -  key:3}  // fault
{ my-key { failure:true}:3} // fault
{ { my-key:3 } }   // fault
{ [ my-key:3 } }   // fault
{ my-key [:3 } }    // fault
{ my-key ]:3 } }    // fault
{ my-key[:3 } }    // fault
{ my-key]:3 } }    // fault

{ 'my  -  key':3}   // valid
{ my-key\\m&m+*|:3}  // valid(in JS version) invalid

{ my-key //test 
   :3}   // valid

{ my-key /*test * / :3}  // valid

*/