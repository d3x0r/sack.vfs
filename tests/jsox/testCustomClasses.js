
const JSOX = require("../..").JSOX;

function Id() {
	
}
Id.prototype.show = function() {
	console.log( "Id Method" );
}


function IdInserter( field, value ) {
// if this was Map()... this.set(field,value)
	this[field] = value;
}

JSOX.registerFromJSOX( "id", Id.prototype, IdInserter );

function logObject(o) {
	console.log( "JSOX Resulted:", o );
}

var parser = JSOX.begin( logObject );
parser.write( "{a:id{field:'123'}}" );

console.log( "Done?" );
