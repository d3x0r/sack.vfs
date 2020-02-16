const util = require('util');
const JSOX = require("../..").JSOX;

function Id() {
	
}
util.inherits( Id, Array );

Id.prototype.show = function() {
	console.log( "Id Method" );
}


function IdInserter( field, value ) {
	if( !field ) return this;
	// if this was Map()... this.set(field,value)
	console.log("Object field:", field,"=", value );
	this[field] = value;
}

JSOX.fromJSOX( "id", Id.prototype, IdInserter );

function ArInserter( index, value ) {
	if( index === undefined ) {
		console.log( "End of array?" );
		return this;
	}
	console.log( "This sort of thing doesn't have named fields..>", index, value );
	this.push(value );
}

JSOX.fromJSOX( "ar", null, ArInserter );


function logObject(o) {
	try {
		console.log( "JSOX Resulted:", o, Object.getPrototypeOf(o), Object.getPrototypeOf(o.a) );
		if( o.a.show )
			o.a.show();
	}catch(err) {
		console.log( "?", err );
	}
}

var parser = JSOX.begin( logObject );

console.log( "test id field" );
parser.write( "{a:id{field:'123'}}" );


console.log( "test ar:");
parser.write( "{a:ar['123',0x555,,,,unquotedString]}" );

console.log( "Test builtin map..." );
parser.write( "{a:map{field:'123'}}" );

console.log( "Done?" );
