var sack= require( "../.." );

function Color() {
	this.r = 100;
        this.g = 150;
        this.b = 20;
}
Color.prototype.toString = function() {
	return '"--'+this.r.toString(16)+this.g.toString(16)+this.b.toString(16)+ '"';
}
Color.prototype.toJSOX = function() {
	return '"#'+this.r.toString(16)+this.g.toString(16)+this.b.toString(16)+ '"';
}

Color.prototype.fromJSOX = function() {
	return new Color();
}


sack.JSOX.registerToFrom( "color", Color.prototype, Color.prototype.toJSOX, Color.prototype.fromJSOX );

var c = new Color();

console.log( "Color:", sack.JSOX.stringify( c ) );
console.log( "Color:", sack.JSOX.parse(sack.JSOX.stringify( c )) );
