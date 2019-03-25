"use strict";

module.exports = function(sack) {


sack.JSON6.stringify = JSON.stringify;
sack.JSON.stringify = JSON.stringify;

var disk = sack.Volume();
require.extensions['.json6'] = function (module, filename) {
    var content = disk.read(filename).toString();
    module.exports = sack.JSON6.parse(content);
};

require.extensions['.jsox'] = function (module, filename) {
    var content = disk.read(filename).toString();
    module.exports = sack.JSOX.parse(content);
};

const _DEBUG_STRINGIFY = false;
var toProtoTypes = new WeakMap();
var toObjectTypes = new Map();
var fromProtoTypes = new Map();
var commonClasses = [];

{
	// hook module native code to JS interface.
	sack.JSOX.setFromPrototypeMap( fromProtoTypes );
	toProtoTypes.set( Object.prototype, { external:false, name:Object.prototype.constructor.name, cb:null } );


	function this_value() {_DEBUG_STRINGIFY&&console.log( "this:", this, "valueof:", this&&this.valueOf() ); return this&&this.valueOf(); }
	// function https://stackoverflow.com/a/17415677/4619267
	toProtoTypes.set( Date.prototype, { external:false,
		name : "Date",
		cb : function () {
			var tzo = -this.getTimezoneOffset(),
				dif = tzo >= 0 ? '+' : '-',
				pad = function(num) {
					var norm = Math.floor(Math.abs(num));
					return (norm < 10 ? '0' : '') + norm;
				};
			return this.getFullYear() +
				'-' + pad(this.getMonth() + 1) +
				'-' + pad(this.getDate()) +
				'T' + pad(this.getHours()) +
				':' + pad(this.getMinutes()) +
				':' + pad(this.getSeconds()) +
				dif + pad(tzo / 60) +
				':' + pad(tzo % 60);
		} 
	} );
	toProtoTypes.set( Boolean.prototype, { external:false, name:"Boolean", cb:this_value  } );
	toProtoTypes.set( Number.prototype, { external:false, name:"Number"
	    , cb:function(){ 
			if( isNaN(this) )  return "NaN";
			return (isFinite(this))
				? String(this)
				: (this<0)?"-Infinity":"Infinity";
	    }
	} );
	toProtoTypes.set( String.prototype, { external:false
	    , name : "String"
	    , cb:function(){ return '"' + sack.JSOX.escape(this_value.apply(this)) + '"' } } );
	if( typeof BigInt === "function" )
		toProtoTypes.set( BigInt.prototype
		     , { external:false, name:"BigInt", cb:function() { return this + 'n' } } );

	toProtoTypes.set( ArrayBuffer.prototype, { external:true, name:"ab"
	    , cb:function() { return "["+base64ArrayBuffer(this)+"]" }
	} );

	toProtoTypes.set( Uint8Array.prototype, { external:true, name:"u8"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	toProtoTypes.set( Uint8ClampedArray.prototype, { external:true, name:"uc8"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	toProtoTypes.set( Int8Array.prototype, { external:true, name:"s8"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	toProtoTypes.set( Uint16Array.prototype, { external:true, name:"u16"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	toProtoTypes.set( Int16Array.prototype, { external:true, name:"s16"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	toProtoTypes.set( Uint32Array.prototype, { external:true, name:"u32"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	toProtoTypes.set( Int32Array.prototype, { external:true, name:"s32"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	if( typeof Uint64Array != "undefined" )
		toProtoTypes.set( Uint64Array.prototype, { external:true, name:"u64"
		    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
		} );
	if( typeof Int64Array != "undefined" )
		toProtoTypes.set( Int64Array.prototype, { external:true, name:"s64"
		    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
		} );
	toProtoTypes.set( Float32Array.prototype, { external:true, name:"f32"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	toProtoTypes.set( Float64Array.prototype, { external:true, name:"f64"
	    , cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );

	toProtoTypes.set( Symbol.prototype, { external:true, name:"sym"
	    , cb:function() { return '"'+this.description+'"' }
	} );
	toProtoTypes.set( Map.prototype, mapToJSOX = { external:true, name:"map"
	    , cb:null
	} );
	fromProtoTypes.set( "map", function (){
		var newMap = new Map();
		for( var key in this ) newMap.set( key, this[key] );
		return newMap;
	} );


	toProtoTypes.set( Array.prototype, arrayToJSOX = { external:false, name:Array.prototype.constructor.name
	    , cb: null		    
	} );
}

sack.JSOX.defineClass = function( name, obj ) {
			var cls; 
			commonClasses.push( cls = { name : name
			       , tag:Object.keys(obj).toString()
			       , proto : Object.getPrototypeOf(obj)
			       , fields : Object.keys(obj) } );
			for(var n = 1; n < cls.fields.length; n++) {
				if( cls.fields[n] < cls.fields[n-1] ) {
					let tmp = cls.fields[n-1];
					cls.fields[n-1] = cls.fields[n];
					cls.fields[n] = tmp;
					if( n > 1 )
						n-=2;
				}
			}
			if( cls.proto === Object.getPrototypeOf( {} ) ) cls.proto = null;
}

sack.JSOX.registerToJSOX = function( name, prototype, f ) {
	_DEBUG_STRINGIFY && console.log( "Register prototype:", prototype, prototype.prototype );
	if( !prototype.prototype || prototype.prototype !== Object.prototype ) {
		if( toProtoTypes.get(prototype) ) throw new Error( "Existing toJSOX has been registered for prototype" );
		_DEBUG_STRINGIFY && console.log( "PUSH PROTOTYPE" );
		toProtoTypes.set( prototype, { external:true, name:name||f.constructor.name, cb:f } );
	} else {
		var key = Object.keys( prototype ).toString();
		if( toObjectTypes.get(key) ) throw new Error( "Existing toJSOX has been registered for object type" );
		_DEBUG_STRINGIFY && console.log( "TEST SET OBJECT TYPE:", key );
		toObjectTypes.set( key, { external:true, name:name, cb:f } );
	}
}
sack.JSOX.registerFromJSOX = function( prototypeName, f ) {
	if( fromProtoTypes.get(prototypeName) ) throw new Error( "Existing fromJSOX has been registered for prototype" );
	fromProtoTypes.set( prototypeName, f );
}
sack.JSOX.registerToFrom = function( prototypeName, prototype, to, from ) {
	//console.log( "INPUT:", prototype );
	sack.JSOX.registerToJSOX( prototypeName, prototype, to );
	sack.JSOX.registerFromJSOX( prototypeName, from );
}

var JSOXBegin = sack.JSOX.begin;

sack.JSOX.begin = function(cb) {
	var parser = JSOXBegin( cb );
	var localFromProtoTypes = new Map();;
	var localPromiseFromProtoTypes = new Map();;
	parser.setFromPrototypeMap( localFromProtoTypes );
	parser.setPromiseFromPrototypeMap( localPromiseFromProtoTypes );
	parser.registerFromJSOX = function( prototypeName, f ) {
		if( localFromProtoTypes.get(prototypeName) ) throw new Error( "Existing fromJSOX has been registered for prototype" );
		localFromProtoTypes.set( prototypeName, f );
	}
	parser.registerPromiseFromJSOX = function( prototypeName, f ) {
		if( localPromiseFromProtoTypes.get(prototypeName) ) throw new Error( "Existing fromJSOX has been registered for prototype" );
		localPromiseFromProtoTypes.set( prototypeName, f );
	}
	return parser;
}

var arrayToJSOX;
var mapToJSOX;

const keywords = {	["true"]:true,["false"]:false,["null"]:null,["NaN"]:NaN,["Infinity"]:Infinity,["undefined"]:undefined }

sack.JSOX.stringifier = function() {
	var classes = [];
	var useQuote = '"';

	var fieldMap = new WeakMap();
	var path = [];
	var encoding = [];
	var objectToJSOX = null;
	var localToProtoTypes = new WeakMap();
	var localToObjectTypes = new Map();
	var stringifying = []; // things that have been stringified through external toJSOX; allows second pass to skip this toJSOX pass and encode 'normally'
	var ignoreNonEnumerable = false;

	return {
		defineClass(name,obj) { 
			var cls; 
			classes.push( cls = { name : name
			       , tag:Object.keys(obj).toString()
			       , proto : Object.getPrototypeOf(obj)
			       , fields : Object.keys(obj) } );
			for(var n = 1; n < cls.fields.length; n++) {
				if( cls.fields[n] < cls.fields[n-1] ) {
					let tmp = cls.fields[n-1];
					cls.fields[n-1] = cls.fields[n];
					cls.fields[n] = tmp;
					if( n > 1 )
						n-=2;
				}
			}
			if( cls.proto === Object.getPrototypeOf( {} ) ) cls.proto = null;
		},
		setDefaultObjectToJSOX( cb ) { objectToJSOX = cb },
		isEncoding(o) { 
			/*console.log( "is object encoding?", o, encoding ); */
			return !!encoding.find( (eo,i)=>eo===o && i < (encoding.length-1) )
		},
		stringify(o,r,s) { return stringify(o,r,s) },
		setQuote(q) { useQuote = q; },
		registerToJSOX( name, prototype, f ) {
			if( prototype.prototype && prototype.prototype !== Object.prototype ) {
				if( localToProtoTypes.get(prototype) ) throw new Error( "Existing toJSOX has been registered for prototype" );
				localToProtoTypes.set( prototype, { external:true, name:name||f.constructor.name, cb:f } );
			} else {
				var key = Object.keys( prototype ).toString();
				if( localToObjectTypes.get(key) ) throw new Error( "Existing toJSOX has been registered for object type" );
				localToObjectTypes.set( key, { external:true, name:name, cb:f } );
			}
		},
		get ignoreNonEnumerable() { return ignoreNonEnumerable; },
		set ignoreNonEnumerable(val) { ignoreNonEnumerable = val; },
	}

	function getReference( here ) {
		if( here === null ) return undefined;
		var field = fieldMap.get( here );
		_DEBUG_STRINGIFY && console.log( "path:", sack.JSON.stringify(path), field );
		if( !field ) {
			fieldMap.set( here, sack.JSON.stringify(path) );
			return undefined;
		}
		return field;
	}



	function matchObject(o,useK) {
		var k;
		var cls;
		var prt = Object.getPrototypeOf(o);
		cls = classes.find( cls=>{
			if( cls.proto && cls.proto === prt ) return true;
		} );
		if( cls ) return cls;
		cls = commonClasses.find( cls=>{
			if( cls.proto && cls.proto === prt ) return true;
		} );
		if( cls ) return cls;

		if( useK )  {
			useK = useK.map( v=>{ if( typeof v === "string" ) return v; else return undefined; } );
			k = useK.toString();
		} else
			k = Object.keys(o).toString();

		cls = classes.find( cls=>{
			if( cls.tag === k ) return true;
		} );
		return cls;
		cls = commonClasses.find( cls=>{
			if( cls.tag === k ) return true;
		} );
		return cls;
	}


	function stringify( object, replacer, space ) {
		if( object === undefined ) return "undefined";
		if( object === null ) return;
		var firstRun = true;
		var gap;
		var indent;
		var meta;
		var rep;

		var i;
		const spaceType = typeof space;
		const repType = typeof replacer;
		gap = "";
		indent = "";

		// If the space parameter is a number, make an indent string containing that
		// many spaces.

		if (spaceType === "number") {
			for (i = 0; i < space; i += 1) {
				indent += " ";
			}

		// If the space parameter is a string, it will be used as the indent string.
		} else if (spaceType === "string") {
			indent = space;
		}

		// If there is a replacer, it must be a function or an array.
		// Otherwise, throw an error.

		rep = replacer;
		if( replacer && repType !== "function"
		    && ( repType !== "object"
		       || typeof replacer.length !== "number"
		   )) {
			throw new Error("JSOX.stringify");
		}

		path = [];
		fieldMap = new WeakMap();

		return str( "", {"":object} );





		// from https://github.com/douglascrockford/JSON-js/blob/master/json2.js#L181
		function str(key, holder) {
			function doArrayToJSOX() {
				var v;
				var partialClass = null;
				var partial = [];
				let thisNodeNameIndex = path.length;
				{
					// The value is an array. Stringify every element. Use null as a placeholder
					// for non-JSOX values.
			
					for (let i = 0; i < this.length; i += 1) {
						path[thisNodeNameIndex] = i;
						partial[i] = str(i, this) || "null";
					}
					path.splice( thisNodeNameIndex, 1 );
					encoding.splice( thisNodeNameIndex, 1 );
										// Join all of the elements together, separated with commas, and wrap them in
					// brackets.
			
					v = ( partial.length === 0
						? "[]"
						: gap
							? [
								"[\n"
								, gap
								, partial.join(",\n" + gap)
								, "\n"
								, mind
								, "]"
							].join("")
							: "[" + partial.join(",") + "]" );
					return v;
				}
			} 

			function getIdentifier(s) {
				if( !isNaN( s ) ) {
					return ["'",s.toString(),"'"].join();
				}
				// should check also for if any non ident in string...
				return ( ( s in keywords /* [ "true","false","null","NaN","Infinity","undefined"].find( keyword=>keyword===s )*/
					|| /((\n|\r|\t)|[ \{\}\(\)\<\>\!\+\-\*\/\.\:\, ])/.test( s ) )?(useQuote + sack.JSOX.escape(s) +useQuote):s )
			}
			
			function mapToObject(){
				var tmp = {tmp:null};
				var out = '{'
				var first = true;
				//console.log( "CONVERT:", map);
				for (var [key, value] of this) {
					//console.log( "er...", key, value )
					tmp.tmp = value;
					out += (first?"":",") + getIdentifier(key) +':' + str("tmp", tmp);
					first = false;
				}
				out += '}';
				//console.log( "out is:", out );
				return out;
			}
			
			if( firstRun ) {
				arrayToJSOX.cb = doArrayToJSOX;
				mapToJSOX.cb = mapToObject;
				firstRun = false;
			}
			
		// Produce a string from holder[key].

			var i;          // The loop counter.
			var k;          // The member key.
			var v;          // The member value.
			var length;
			var mind = gap;
			var partialClass;
			var partial;
			let thisNodeNameIndex = path.length;
			var value = holder[key];
			let isObject = (typeof value === "object");
			var protoConverter = (value !== undefined && value !== null) 
				&& ( localToProtoTypes.get( Object.getPrototypeOf( value ) ) 
				|| toProtoTypes.get( Object.getPrototypeOf( value ) ) 
				|| null )
			var objectConverter = !protoConverter && (value !== undefined && value !== null) 
				&& ( localToObjectTypes.get( Object.keys( value ).toString() ) 
				|| toObjectTypes.get( Object.keys( value ).toString() ) 
				|| null )

				//console.log( "VALUE:", value );
				//if( value !== null && value !== undefined ) console.log( "PROTOTYPE:", Object.getPrototypeOf( value ) )
				//console.log( "PROTOTYPE:", toProtoTypes.get(Object.getPrototypeOf( value )) )
			_DEBUG_STRINGIFY && console.log( "TEST()", value, protoConverter, objectConverter );

			var toJSOX = ( protoConverter && protoConverter.cb ) 
			          || ( objectConverter && objectConverter.cb )
        		          || ( isObject && objectToJSOX );
			// If the value has a toJSOX method, call it to obtain a replacement value.
			_DEBUG_STRINGIFY && console.log( "type:", typeof value, protoConverter, !!toJSOX, path, isObject );

			if( value !== undefined
			    && value !== null
			    && typeof toJSOX === "function"
			) {
				if( !stringifying.find( val=>val===value ) ) {
					gap += indent;
					if( typeof value === "object" ) {
						v = getReference( value );
						if( v ) return "ref"+v;
					}
					stringifying.push( value );
					let newValue = toJSOX.apply(value);
					stringifying.pop();
					if( newValue === value ) {
						protoConverter = null;
					}
					if(_DEBUG_STRINGIFY ) { 
						console.log( "translated ", newValue, value );
					}
					value = newValue;

					gap = mind;
				}
			} else 
				if( typeof value === "object" ) {
					v = getReference( value );
				}

			// If we were called with a replacer function, then call the replacer to
			// obtain a replacement value.

			if (typeof rep === "function") {
				value = rep.call(holder, key, value);
			}

			// What happens next depends on the value's type.
			switch (typeof value) {
			case "string":
			case "number": 
				{
					let c = '';
					if( key==="" ) {
						c = classes.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"")
						    || commonClasses.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"");
					}
					if( protoConverter && protoConverter.external ) 
						return c + protoConverter.name + value;
					if( objectConverter && objectConverter.external ) 
						return c + objectConverter.name + value;
					return c + value;//useQuote+JSOX.escape( value )+useQuote;
				}
			case "boolean":
			case "null":

				// If the value is a boolean or null, convert it to a string. Note:
				// typeof null does not produce "null". The case is included here in
				// the remote chance that this gets fixed someday.

				return String(value);

				// If the type is "object", we might be dealing with an object or an array or
				// null.

			case "object":

				_DEBUG_STRINGIFY && console.log( "ENTERINT OBJECT EMISSION WITH:", v );
				if( v ) return "ref"+v;

				// Due to a specification blunder in ECMAScript, typeof null is "object",
				// so watch out for that case.
				if (!value) {
					return "null";
				}

				// Make an array to hold the partial results of stringifying this object value.
			
				gap += indent;
				partialClass = null;
				partial = [];

				// If the replacer is an array, use it to select the members to be stringified.
				if (rep && typeof rep === "object") {
					length = rep.length;
					partialClass = matchObject( value, rep );
					for (i = 0; i < length; i += 1) {
						if (typeof rep[i] === "string") {
							k = rep[i];
							path[thisNodeNameIndex] = k;
							v = str(k, value);

							if (v) {
								if( partialClass ) {
									partial.push(v);
							} else
									partial.push(getIdentifier(k) + (
										(gap)
											? ": "
											: ":"
									) + v);
							}
						}
					}
					path.splice( thisNodeNameIndex, 1 );
					encoding.splice( thisNodeNameIndex, 1 );
				} else {

					// Otherwise, iterate through all of the keys in the object.
					partialClass = matchObject( value );
					var keys = [];
					for (k in value) {
						if( ignoreNonEnumerable )
							if( !Object.prototype.propertyIsEnumerable.call( value, k ) ){
								console.log( "skipping non-enuerable?", k );
								continue;
							}

						// sort properties into keys.
						if (Object.prototype.hasOwnProperty.call(value, k)) {
							var n;
							for( n = 0; n < keys.length; n++ ) 
								if( keys[n] > k ) {	
									keys.splice(n,0,k );
									break;
								}
							if( n == keys.length )
								keys.push(k);
						}
					}
					for(n = 0; n < keys.length; n++) {
						k = keys[n];
						if (Object.prototype.hasOwnProperty.call(value, k)) {
							path[thisNodeNameIndex] = k;
							encoding[thisNodeNameIndex] = value;
							v = str(k, value);

							if (v) {
								if( partialClass ) {
									partial.push(v);
								} else
									partial.push(getIdentifier(k)+ (
										(gap)
											? ": "
											: ":"
									) + v);
							}
						}
					}
					path.splice( thisNodeNameIndex, 1 );
					encoding.splice( thisNodeNameIndex, 1 );
				}

				// Join all of the member texts together, separated with commas,
				// and wrap them in braces.
				_DEBUG_STRINGIFY && console.log( "partial:", partial )
				{
				let c;
				if( key==="" )
					c = classes.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"")
					    || commonClasses.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"");
				else
					c = '';
				if( protoConverter && protoConverter.external ) 
					if( key==="" ) 
						c = c + protoConverter.name;
					else
						c = c + '"' + protoConverter.name + '"';

				var ident = null;
				if( partialClass )
					ident = getIdentifier( partialClass.name );
				v = c +
					( partial.length === 0
					? "{}"
					: gap
							? (partialClass?ident:"")+"{\n" + gap + partial.join(",\n" + gap) + "\n" + mind + "}"
							: (partialClass?ident:"")+"{" + partial.join(",") + "}"
					);
				}
				gap = mind;
				return v;
			}
		}

	}

	
	
}

	// Converts an ArrayBuffer directly to base64, without any intermediate 'convert to string then
	// use window.btoa' step. According to my tests, this appears to be a faster approach:
	// http://jsperf.com/encoding-xhr-image-data/5
	// doesn't have to be reversable....
	const encodings = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
	const decodings = { '=':-1 };
	
	for( var x = 0; x < 256; x++ ) {
		if( x < 64 ) {
			decodings[encodings[x]] = x;
		}
	}
	
	function base64ArrayBuffer(arrayBuffer) {
		var base64    = ''
	
		var bytes	 = new Uint8Array(arrayBuffer)
		var byteLength    = bytes.byteLength
		var byteRemainder = byteLength % 3
		var mainLength    = byteLength - byteRemainder
	
		var a, b, c, d
		var chunk
		//throw "who's using this?"
		//console.log( "buffer..", arrayBuffer )
		// Main loop deals with bytes in chunks of 3
		for (var i = 0; i < mainLength; i = i + 3) {
			// Combine the three bytes into a single integer
			chunk = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2]
	
			// Use bitmasks to extract 6-bit segments from the triplet
			a = (chunk & 16515072) >> 18 // 16515072 = (2^6 - 1) << 18
			b = (chunk & 258048)   >> 12 // 258048   = (2^6 - 1) << 12
			c = (chunk & 4032)     >>  6 // 4032     = (2^6 - 1) << 6
			d = chunk & 63               // 63       = 2^6 - 1
	
			// Convert the raw binary segments to the appropriate ASCII encoding
			base64 += encodings[a] + encodings[b] + encodings[c] + encodings[d]
		}
	
		// Deal with the remaining bytes and padding
		if (byteRemainder == 1) {
			chunk = bytes[mainLength]
			a = (chunk & 252) >> 2 // 252 = (2^6 - 1) << 2
			// Set the 4 least significant bits to zero
			b = (chunk & 3)   << 4 // 3   = 2^2 - 1
			base64 += encodings[a] + encodings[b] + '=='
		} else if (byteRemainder == 2) {
			chunk = (bytes[mainLength] << 8) | bytes[mainLength + 1]
			a = (chunk & 64512) >> 10 // 64512 = (2^6 - 1) << 10
			b = (chunk & 1008)  >>  4 // 1008  = (2^6 - 1) << 4
			// Set the 2 least significant bits to zero
			c = (chunk & 15)    <<  2 // 15    = 2^4 - 1
			base64 += encodings[a] + encodings[b] + encodings[c] + '='
		}
		//console.log( "dup?", base64)
		return base64
	}
	
	
	function DecodeBase64( buf )
	{	
		//console.log( "length:", buf.length, (((buf.length+3)/4)|0), (buf[buf.length-1]==='='?1:0), (buf[buf.length-2]==='='?1:0) )
		var ab = new ArrayBuffer( (3*(((buf.length+3)>>2)|0)) - ((buf[buf.length-1]==='='?1:0) + (buf[buf.length-2]==='='?1:0)) );
		//console.log( "LENGHT:", (3*(((buf.length+3)/4)|0)) - ((buf[buf.length-1]==='='?1:0) + (buf[buf.length-2]==='='?1:0)) );
		var out = new Uint8Array(ab);
		{
			var n;
			var l = (buf.length+3)>>2;
			for( n = 0; n < l; n++ )
			{
				var index0 = decodings[buf[n*4]];
				var index1 = decodings[buf[n*4+1]];
				var index2 = decodings[buf[n*4+2]];
				var index3 = decodings[buf[n*4+3]];
				
				out[n*3+0] = (( index0 ) << 2 | ( index1 ) >> 4);
				if( index2 >= 0 )
					out[n*3+1] = (( index1 ) << 4 | ( ( ( index2 ) >> 2 ) & 0x0f ));
				if( index3 >= 0 )
					out[n*3+2] = (( index2 ) << 6 | ( ( index3 ) & 0x3F ));
			}
		}
		return ab;
	}
	
	
sack.JSOX.stringify = function( object, replacer, space ) {
	var stringifier = sack.JSOX.stringifier();
	return stringifier.stringify( object, replacer, space );
}

const nonIdent = 
[ [ 0,264,[ 0xffd9ff,0xff6aff,0x1fc00,0x380000,0x0,0xfffff8,0xffffff,0x7fffff,0x800000,0x0,0x80 ] ],
]/*
[ 384,768,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3c00,0xe0fffc,0xffffaf ] ],
[ 768,1152,[ 0x0,0x0,0x0,0x0,0x200000,0x3040,0x0,0x0,0x0,0x0,0x40,0x0,0x0,0x0,0x0,0x0 ] ],
[ 1152,1536,[ 0x304,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xfc,0x0,0xe6,0x0,0x4940,0x0,0x1800 ] ],
[ 1536,1920,[ 0xffff,0xd8,0x0,0x0,0x3c00,0x0,0x0,0x0,0x100000,0x20060,0xff6000,0xbf,0x0,0x0,0x0,0x0 ] ],
[ 1920,2304,[ 0x0,0x0,0x0,0x0,0xc00000,0x3,0x0,0x7fff00,0x0,0x40,0x0,0x0,0x0,0x0,0x40000,0x0 ] ],
[ 2304,2688,[ 0x0,0x0,0x0,0x0,0x10030,0x0,0x0,0x0,0x0,0x0,0x2ffc,0x0,0x0,0x0,0x0,0x0 ] ],
[ 2688,3072,[ 0x0,0x0,0x0,0x0,0x30000,0x0,0x0,0x0,0x0,0x0,0xfd,0x0,0x0,0x0,0x0,0x7ff00 ] ],
[ 3072,3456,[ 0x0,0x0,0x0,0x0,0x0,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x800000,0x7f00,0x3ff00 ] ],
[ 3456,3840,[ 0x0,0x0,0x0,0x0,0x100000,0x0,0x0,0x800000,0x8000,0xc,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 3840,4224,[ 0xfffffe,0xfc00fc,0x3d5f,0x0,0x0,0x2000,0x0,0xc00000,0xffdfbf,0x7,0x0,0x0,0x0,0xfc0000,0x0,0x0 ] ],
[ 4224,4608,[ 0x0,0xc0,0x0,0x0,0x0,0x8,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 4608,4992,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff0000,0x1ffc01 ] ],
[ 4992,5376,[ 0xff0000,0x3,0x0,0x0,0x0,0x100,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 5376,5760,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x60 ] ],
[ 5760,6144,[ 0x1,0x18,0x0,0x0,0x3800,0x0,0x0,0x6000,0x0,0x0,0x0,0x0,0x0,0x0,0xf70,0x3ff00 ] ],
[ 6144,6528,[ 0x47ff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3100,0x0,0x0 ] ],
[ 6528,6912,[ 0x0,0x0,0x0,0xc00000,0xffffff,0xff,0xc000,0x0,0x0,0x0,0x0,0x0,0x3f7f,0x40,0x0,0x0 ] ],
[ 6912,7296,[ 0x0,0x0,0x0,0xfc0000,0xf007ff,0x1f,0x0,0x0,0x0,0x0,0xf000,0x0,0x0,0xf8,0x0,0xc00000 ] ],
[ 7296,7680,[ 0x0,0x0,0xff0000,0x800,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 8064,8448,[ 0x0,0x0,0x3a000,0xe000e0,0xe000,0xffff60,0xffffff,0x7fffff,0xeffffe,0xffdfff,0xff7ff1,0x7f,0xffffff,0xff,0x1de000,0x0 ] ],
[ 8448,8832,[ 0xd0037b,0x2afc0,0x1f0c00,0xffffbc,0x0,0xe0000,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 8832,9216,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 9216,9600,[ 0xffffff,0x7fff,0xff0000,0x7,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 9600,9984,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 9984,10368,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 10368,10752,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 10752,11136,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffcfff ] ],
[ 11136,11520,[ 0x3fffff,0xffffff,0xffe3ff,0x7fd,0xf000,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xe00000,0xfe0007 ] ],
[ 11520,11904,[ 0x0,0x0,0x0,0x0,0x10000,0x0,0x0,0x0,0x0,0x0,0xff0000,0xffffff,0xffffff,0x3ffff,0x0,0x0 ] ],
[ 11904,12288,[ 0xffffff,0xfffffb,0xffffff,0xffffff,0xfffff,0xffff00,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0x3f,0xfff00 ] ],
[ 12288,12672,[ 0xffff1f,0x1ff,0xe0c1,0x0,0x0,0x0,0x10000,0x0,0x0,0x0,0x800,0x0,0x0,0x0,0x0,0x0 ] ],
[ 12672,13056,[ 0xff0000,0xff,0xff0000,0xffffff,0xf,0xffff00,0xff7fff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0x7fffff ] ],
[ 13056,13440,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffff,0x0,0x0,0x0,0x0,0x0 ] ],
[ 19584,19968,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xffff00,0xffffff,0xffffff ] ],
[ 41856,42240,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xffff00,0xffffff,0x7fff,0x0,0xc00000 ] ],
[ 42240,42624,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xe0,0x0,0x0,0x0,0x400f00 ] ],
[ 42624,43008,[ 0x0,0x0,0x0,0x0,0xfc0000,0xffff00,0x3007f,0x0,0x0,0x0,0x0,0x6,0x0,0x0,0x0,0x0 ] ],
[ 43008,43392,[ 0x0,0xf0000,0x3ff,0x0,0xf00000,0x0,0x0,0x0,0xc000,0x0,0x1700,0x0,0xc000,0x0,0x8000,0x0 ] ],
[ 43392,43776,[ 0x0,0x0,0xfe0000,0xc0003f,0x0,0x0,0x0,0x0,0x0,0xf0,0x380,0x0,0x0,0x0,0xc000,0x300 ] ],
[ 43776,44160,[ 0x0,0x0,0x0,0x80000,0x0,0x0,0x0,0x0,0x0,0x80000,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 64128,64512,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0x0,0xfc0000,0x3ff,0x0,0x0 ] ],
[ 64512,64896,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xc0,0x0,0x0 ] ],
[ 64896,65280,[ 0x0,0x0,0x0,0x0,0x0,0x30,0x3ff,0xffe700,0xf71fff,0xf7fff,0x0,0x0,0x0,0x0,0x0,0x800000 ] ],
[ 65280,65664,[ 0xfffe,0x1fc,0x17800,0xf80000,0x3f,0x0,0x0,0x0,0x0,0x7f7f00,0x3e00,0x0,0x0,0x0,0x0,0x0 ] ],
[ 65664,66048,[ 0x0,0x0,0x0,0x0,0x0,0xff8700,0xffffff,0xff8fff,0x0,0x0,0xffffe0,0xfff7f,0x1,0x0,0xffffff,0x1fffff ] ],
[ 66048,66432,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xfffe00,0xfff,0x0,0xf,0x0,0x0,0x0 ] ],
[ 66432,66816,[ 0x0,0x80,0x0,0x100,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 66816,67200,[ 0x0,0x0,0x0,0x0,0x8000,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 67584,67968,[ 0x0,0x0,0x0,0xff8000,0x800000,0xff,0x800000,0xff,0x0,0x0,0xf800,0x8fc000,0x0,0x80,0x0,0x0 ] ],
[ 67968,68352,[ 0x0,0x0,0xff3000,0xfffcff,0xffffff,0xff,0x0,0x0,0xff00ff,0x1,0xe000,0xe00000,0x0,0x10000,0x0,0x7ff8 ] ],
[ 68352,68736,[ 0x0,0x0,0xfe00,0xff0000,0x0,0xff,0x1e00,0xfe,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 68736,69120,[ 0x0,0x0,0x0,0x0,0x0,0xfc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 69120,69504,[ 0x0,0x0,0x0,0x0,0xffffff,0x7f,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 69504,69888,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xfc3f80,0x3fff,0x0,0x0,0x0,0x3f8,0x0,0x0 ] ],
[ 69888,70272,[ 0x0,0x0,0xf0000,0x0,0x300000,0x0,0x0,0x0,0x23e0,0xfffee8,0x1f,0x0,0x0,0x3f,0x0,0x0 ] ],
[ 70272,70656,[ 0x0,0x20000,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 70656,71040,[ 0x0,0x0,0x0,0x2800f8,0x0,0x0,0x0,0x0,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 71040,71424,[ 0x0,0x0,0xfe0000,0xffff,0x0,0x0,0x0,0x0,0xe,0x1fff00,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 71424,71808,[ 0x0,0x0,0xfc00,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 71808,72192,[ 0x0,0x0,0x0,0x0,0x7fc00,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 72192,72576,[ 0x0,0x0,0x7f8000,0x0,0x0,0x0,0x7dc00,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 72576,72960,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3e,0x1ffffc,0x3,0x0,0x0,0x0,0x0,0x0 ] ],
[ 74496,74880,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1f00 ] ],
[ 92544,92928,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xc00000,0x0,0x0,0x0,0x0,0x0,0x2000 ] ],
[ 92928,93312,[ 0x0,0x0,0x30ff80,0xf80000,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 113664,114048,[ 0x0,0x0,0x0,0x0,0x0,0x0,0xf9000,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 118656,119040,[ 0x0,0x0,0x0,0x0,0x0,0xffff00,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0x3fff ] ],
[ 119040,119424,[ 0xffffff,0xfe7fff,0xffffff,0xffffff,0xf81c1f,0xf01807,0xffffff,0xffffc3,0xffffff,0x1ffff,0xff0000,0xffffff,0xffffff,0x23ff,0x0,0x0 ] ],
[ 119424,119808,[ 0x0,0x0,0x0,0x0,0x0,0xffff00,0xffffff,0xffffff,0x7fffff,0xffff00,0x3,0x0,0x0,0x0,0x0,0x0 ] ],
[ 120192,120576,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x200,0x800,0x80000 ] ],
[ 120576,120960,[ 0x200000,0x0,0x20,0x80,0x8000,0x20000,0x0,0x2,0x8,0x0,0xff0000,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 120960,121344,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 121344,121728,[ 0x0,0x0,0x780,0x0,0xdfe000,0xfefff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 124800,125184,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff8000,0x0,0x0 ] ],
[ 125184,125568,[ 0x0,0x0,0x0,0xc00000,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 126336,126720,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x300 ] ],
[ 126720,127104,[ 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff0000,0xffffff,0xff0fff,0xffffff,0xffffff,0xffffff ] ],
[ 127104,127488,[ 0xfffff,0x7fff00,0xfefffe,0xfffeff,0x3fffff,0x1fff00,0xffffff,0xffff7f,0xffffff,0xfffff,0xffffff,0xffffff,0x1fff,0x0,0xc00000,0xffffff ] ],
[ 127488,127872,[ 0xff0007,0xffffff,0xff0fff,0x301,0x3f,0x0,0x0,0x0,0x0,0x0,0xff0000,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 127872,128256,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 128256,128640,[ 0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff ] ],
[ 128640,129024,[ 0xffffff,0xffffff,0xffffff,0x1fff,0xff1fff,0xffff01,0xffffff,0xffffff,0xffffff,0xffffff,0xff000f,0xffffff,0xffffff,0xffffff,0x1f,0x0 ] ],
[ 129024,129408,[ 0xff0fff,0xffffff,0xffffff,0x3ff00,0xffffff,0xffff,0xffffff,0x3f,0x0,0x0,0xff0000,0xffff0f,0xffffff,0x1fff7f,0xffffff,0xf ] ],
[ 129408,129792,[ 0xffffff,0x0,0x10000,0xffff00,0x7f,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 ] ],
[ 917376,917760,[ 0x0,0x0,0x0,0x0,0x0,0x200,0xff0000,0xffffff,0xffffff,0xffffff ] ]
]*/.map( row=>{ return{ firstChar : row[0], lastChar: row[1], bits : row[2] }; } );

}
