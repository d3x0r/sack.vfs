"use strict";

module.exports = function(sack) {

sack.JSON6.stringify = JSON.stringify;
sack.JSON.stringify = JSON.stringify;

try {
	var disk = sack.Volume();
	require.extensions['.json6'] = function (module, filename) {
		var content = disk.read(filename).toString();
		module.exports = sack.JSON6.parse(content);
	};

	require.extensions['.jsox'] = function (module, filename) {
		var content = disk.read(filename).toString();
		module.exports = sack.JSOX.parse(content);
	};
} catch(err) {
	console.log( "JSOX Module could not register require support..." );
}
const _DEBUG_STRINGIFY = false;
const DEBUG_STRINGIFY_OUTPUT = _DEBUG_STRINGIFY|| false;
var toProtoTypesByName = new Map();
var toProtoTypes = new WeakMap();
var toProtoTypeRegistrations = []; // external registrations may need to be updated too....
const toObjectTypes = new Map();
const fromProtoTypes = new Map();
var commonClasses = [];

sack.JSOX.fromProtoType = fromProtoTypes;

function pushToProto(p,a) {
    if( !toProtoTypesByName.get( p.constructor.name ) )
	    toProtoTypesByName.set( p.constructor.name, a );
    else
        console.log( "duplicating...", p );
    toProtoTypes.set( p, a );
}

function escape(string) {
	//return string.replace( "\\", "\\\\" ).replace( '\"', "\\\"" ).replace( "\'", "\\\'" );
	var n;
	var output = '';
	if( !string ) return string;
	//console.log( "escape:", string );
	for( n = 0; n < string.length; n++ ) {
		if( ( string[n] == '"' ) || ( string[n] == '\\' ) || ( string[n] == '`' )|| ( string[n] == '\'' )) {
			output += '\\';
		}
		output += string[n];
	}
	return output;
}

initPrototypes();
function initPrototypes()
{
    	//console.log( "Doing setup of classes:", toProtoTypes.get( Object.getPrototypeOf( [] ) ) );
	// hook module native code to JS interface.
	sack.JSOX.setFromPrototypeMap( fromProtoTypes );
	pushToProto( Object.prototype, { external:false, name:Object.prototype.constructor.name, cb:null } );

	function this_value() {_DEBUG_STRINGIFY&&console.log( "this:", this, "valueof:", this&&this.valueOf() ); return this&&this.valueOf(); }
	// function https://stackoverflow.com/a/17415677/4619267
	pushToProto( Date.prototype, { external:false,
		name : null, // this doesn't get a tag name, it returns a literal.
		cb : function () {
			var tzo = -this.getTimezoneOffset(),
				dif = tzo >= 0 ? '+' : '-',
				pad = function(num) {
					var norm = Math.floor(Math.abs(num));
					return (norm < 10 ? '0' : '') + norm;
				},
				pad3 = function(num) {
					var norm = Math.floor(Math.abs(num));
					return (norm < 100 ? '0' : '') + (norm < 10 ? '0' : '') + norm;
				};
			return this.getFullYear() +
				'-' + pad(this.getMonth() + 1) +
				'-' + pad(this.getDate()) +
				'T' + pad(this.getHours()) +
				':' + pad(this.getMinutes()) +
				':' + pad(this.getSeconds()) +
				'.' + pad3(this.getMilliseconds()) +
				dif + pad(tzo / 60) +
				':' + pad(tzo % 60);
		}
	} );
	pushToProto( Boolean.prototype, { external:false, name:null, cb:this_value  } );
	pushToProto( Number.prototype, { external:false, name:null
	    , cb:function(){
			if( isNaN(this) )  return "NaN";
			return (isFinite(this))
				? String(this)
				: (this<0)?"-Infinity":"Infinity";
		}
	} );
	pushToProto( String.prototype, { external:false
	                                    , name : null
	                                    , cb:function(){ return '"' + escape(this_value.apply(this)) + '"' } } );
	if( typeof BigInt === "function" )
		pushToProto( BigInt.prototype
		                , { external:false, name:null, cb:function() { console.log( "BIGINT TOSTR"); return this + 'n' } } );

	pushToProto( ArrayBuffer.prototype, { external:true, name:"ab"
		, cb:function() { return "["+base64ArrayBuffer(this)+"]" }
	} );

	pushToProto( Uint8Array.prototype, { external:true, name:"u8"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	pushToProto( Uint8ClampedArray.prototype, { external:true, name:"uc8"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	pushToProto( Int8Array.prototype, { external:true, name:"s8"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	pushToProto( Uint16Array.prototype, { external:true, name:"u16"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	pushToProto( Int16Array.prototype, { external:true, name:"s16"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	pushToProto( Uint32Array.prototype, { external:true, name:"u32"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	pushToProto( Int32Array.prototype, { external:true, name:"s32"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	if( typeof Uint64Array !== "undefined" )
		pushToProto( Uint64Array.prototype, { external:true, name:"u64"
			, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
		} );
	if( typeof Int64Array !== "undefined" )
		pushToProto( Int64Array.prototype, { external:true, name:"s64"
			, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
		} );
	pushToProto( Float32Array.prototype, { external:true, name:"f32"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );
	pushToProto( Float64Array.prototype, { external:true, name:"f64"
		, cb:function() { return "["+base64ArrayBuffer(this.buffer)+"]" }
	} );

	pushToProto( Symbol.prototype, { external:true, name:"sym"
		, cb:function() { return '"'+this.description+'"' }
	} );
	pushToProto( Map.prototype, mapToJSOX = { external:true, name:"map"
		, cb:null
	} );
	fromProtoTypes.set("map", {
		protoCon: Map.prototype.constructor, cb:function(field, val) {
			if (!field) return this;
			this.set( field,val );
	} } );
	
	pushToProto( Array.prototype, arrayToJSOX = { external:false, name:Array.prototype.constructor.name
		, cb: null
	} );
}

sack.JSOX.defineClass = function( name, obj ) {
	var cls;
	var denormKeys = Object.keys(obj);
	for( var i = 1; i < denormKeys.length; i++ ) {
		var a, b;
		if( ( a = denormKeys[i-1] ) > ( b = denormKeys[i] ) ) {
			denormKeys[i-1] = b;
			denormKeys[i] = a;
			if( i ) i-=2; // go back 2, this might need to go further pack.
			else i--; // only 1 to check.
		}
	}
	//console.log( "normalized:", denormKeys );
	commonClasses.push( cls = { name : name
		   , tag:denormKeys.toString()
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
	_DEBUG_STRINGIFY &&
	        console.log( "Register prototype:", prototype, prototype.prototype );
	if( !prototype.prototype || prototype.prototype !== Object.prototype ) {
		if( toProtoTypes.get(prototype.prototype) ) {
                    console.trace( "Duplicated definition of toJSOX for", name );
                    //throw new Error( "Existing toJSOX has been registered for prototype" );
                }else {
		_DEBUG_STRINGIFY && console.log( "PUSH PROTOTYPE" );
		pushToProto( prototype.prototype, { external:true, name:(name===undefined)?f.prototype.constructor.name:name, cb:f } );
                }
	} else {
		var key = Object.keys( prototype ).toString();
		if( toObjectTypes.get(key) ) throw new Error( "Existing toJSOX has been registered for object type" );
		_DEBUG_STRINGIFY && console.log( "TEST SET OBJECT TYPE:", key );
		toObjectTypes.set( key, { external:true, name:name, cb:f } );
	}
}

sack.JSOX.updateContext = function() {
    if( toProtoTypes.get( Map.prototype ) ) return;
    console.log( "Do init protoypes for new context objects..." );
    initPrototypes();
}
sack.JSOX.toJSOX = sack.JSOX.registerToJSOX;
sack.JSOX.registerFromJSOX = function (prototypeName, o, f) {
	throw new Error("registerFromJSOX  was deprecated, please update to use 'fromJSOX'");
}

sack.JSOX.fromJSOX = function( prototypeName, o, f ) {
	//console.log( "Registration:", prototypeName, o );
	if( fromProtoTypes.get(prototypeName) ) throw new Error( "Existing fromJSOX has been registered for prototype" );
	if( o && !("constructor" in o )) {
		throw new Error( "Please pass a proper prototype...." );
	}
	var z;
	fromProtoTypes.set(prototypeName, z = { protoCon: o && o.prototype.constructor, cb: f });
        //console.log( "registered", z );
}
sack.JSOX.registerToFrom = function( prototypeName, prototype, to, from ) {
	//console.log( "INPUT:", prototype );
	sack.JSOX.registerToJSOX( prototypeName, prototype, to );
	sack.JSOX.fromJSOX( prototypeName, prototype, from );
}
sack.JSOX.addType = function( prototypeName, prototype, to, from ) {
	sack.JSOX.toJSOX( prototypeName, prototype, to );
	sack.JSOX.fromJSOX( prototypeName, prototype, from );
}

var JSOXBegin = sack.JSOX.begin;

sack.JSOX.begin = function(cb, reviver) {
	var parser = JSOXBegin( cb, reviver );
	var localFromProtoTypes = new Map();;
	var localPromiseFromProtoTypes = new Map();;
	parser.localFromProtoTypes = localFromProtoTypes;
	parser.setFromPrototypeMap( localFromProtoTypes );
	parser.setPromiseFromPrototypeMap( localPromiseFromProtoTypes );
	parser.registerFromJSOX = function (prototypeName, o, f) {
		throw new Error("registerFromJSOX  was deprecated, please update to use 'fromJSOX'");
	}
	parser.fromJSOX = function( prototypeName, o, f ) {
		if( localFromProtoTypes.get(prototypeName) ) throw new Error( "Existing fromJSOX has been registered for prototype" );

		if(o && !("constructor" in o))
			throw new Error("Please pass a prototype like object...");
		localFromProtoTypes.set( prototypeName, { protoCon:o && o.prototype.constructor, cb:f } );
	}
	parser.registerPromiseFromJSOX = function( prototypeName, o, f ) {
		throw new Error( "Deprecated 'registerPromiseFromJSOX', pluse use fromJSOX has been registered for prototype" );
	}
	return parser;
}

var arrayToJSOX;
var mapToJSOX;

const keywords = {	["true"]:true,["false"]:false,["null"]:null,["NaN"]:NaN,["Infinity"]:Infinity,["undefined"]:undefined }
var id = 1;
sack.JSOX.stringifierActive = null;
sack.JSOX.stringifier = function() {
	var classes = [];
	var useQuote = '"';

	var fieldMap = new WeakMap();
	var path = [];
	var encoding = [];
	var objectToJSOX = null;
	var localToProtoTypes = new WeakMap();
	localToProtoTypes.id = id++;
	var localToObjectTypes = new Map();
	const stringifying = []; // things that have been stringified through external toJSOX; allows second pass to skip this toJSOX pass and encode 'normally'
	var ignoreNonEnumerable = false;

	return {
		defineClass(name,obj) {
			var cls;
			var denormKeys = Object.keys(obj);
			for( var i = 1; i < denormKeys.length; i++ ) {
				var a, b;
				if( ( a = denormKeys[i-1] ) > ( b = denormKeys[i] ) ) {
					denormKeys[i-1] = b;
					denormKeys[i] = a;
					if( i ) i-=2; // go back 2, this might need to go further pack.
					else i--; // only 1 to check.
				}
			}
			classes.push( cls = { name : name
				   , tag:denormKeys.toString()
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
			//console.log( "is object encoding?", encoding.length, o, encoding );
			return !!encoding.find( (eo,i)=>eo===o && i < (encoding.length-1) )
		},
		encodeObject(o) {
			if( objectToJSOX ) 
				return objectToJSOX.apply(o, [this]);
			return o;
		},
		stringify(o,r,s,as) { return stringify(this, o,r,s,as) },
		setQuote(q) { useQuote = q; },
		toJSOX( name,proto,f) { return this.registerToJSOX( name,proto,f) },
		registerToJSOX( name, ptype, f ) {
			if( ptype.prototype && ptype.prototype !== Object.prototype ) {
				if( localToProtoTypes.get(ptype) ) throw new Error( "Existing toJSOX has been registered for prototype" );
				_DEBUG_STRINGIFY && console.log( "Adding prototype to  local objects:", name, ptype.prototype, localToProtoTypes );
				const newThing = { external:true, name:(name===undefined)?f.prototype.constructor.name:name, cb:f };
				localToProtoTypes.set( ptype.prototype, newThing );
			} else {
				_DEBUG_STRINGIFY && console.log( "This is set by key?!" );
				var key = Object.keys( ptype ).toString();
				if( localToObjectTypes.get(key) ) throw new Error( "Existing toJSOX has been registered for object type" );
				localToObjectTypes.set( key, { external:true, name:name, cb:f } );
			}
		},
		getReference: getReference,
		get ignoreNonEnumerable() { return ignoreNonEnumerable; },
		set ignoreNonEnumerable(val) { ignoreNonEnumerable = val; },
	}

	function getReference( here ) {
		if( here === null ) return undefined;
		var field = fieldMap.get( here );
		_DEBUG_STRINGIFY && console.trace( "get reference path:", here, sack.JSON.stringify(path), field );
		if( !field ) {
			fieldMap.set( here, sack.JSON.stringify(path) );
			return undefined;
		}
		return 'ref'+field;
	}

	function getIdentifier(s) {
		if( "number" === typeof s && !isNaN( s ) ) {
			return ["'",s.toString(),"'"].join();
		}
		if( !s.length ) return useQuote+useQuote;
		// should check also for if any non ident in string...
		return ( ( s in keywords /* [ "true","false","null","NaN","Infinity","undefined"].find( keyword=>keyword===s )*/
			|| /([0-9\-])/.test(s[0])
			|| /((\n|\r|\t)|[ \#\{\}\(\)\<\>\!\+\-\*\/\.\:\, ])/.test( s ) )?(useQuote + escape(s) +useQuote):s )
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

		if( classes.length || commonClasses.length ) {
			if( useK )  {
				useK = useK.map( v=>{ if( typeof v === "string" ) return v; else return undefined; } );
				k = useK.toString();
			} else {
				var denormKeys = Object.keys(o);
				for( var i = 1; i < denormKeys.length; i++ ) {
					var a, b;
					if( ( a = denormKeys[i-1] ) > ( b = denormKeys[i] ) ) {
						denormKeys[i-1] = b;
						denormKeys[i] = a;
						if( i ) i-=2; // go back 2, this might need to go further pack.
						else i--; // only 1 to check.
					}
				}
				k = denormKeys.toString();
			}
			cls = classes.find( cls=>{
				if( cls.tag === k ) return true;
			} );
			if( !cls )
				cls = commonClasses.find( cls=>{
					if( cls.tag === k ) return true;
				} );
		}
		return cls;
	}


	function stringify( stringifier, object, replacer, space, asField ) {
		if( object === undefined ) return "undefined";
		if( object === null ) return "null";
		var firstRun = true;
		var gap;
		var indent;
		var rep;

		var i;
		const spaceType = typeof space;
		const repType = typeof replacer;
		gap = "";
		indent = "";
		const stringifier_ = sack.JSOX.stringifierActive;
		sack.JSOX.stringifierActive = stringifier;

		const pathBase = path.length;
		if( !asField ) {
			asField = "";
		}else {
			path.push( asField );
			encoding[pathBase] = object;
		}


		if( "object" === typeof object && stringifier_ ) {
			var ref = stringifier_.getReference( object );
                        //console.log("This added object as ref", ref, object );
			if( ref ) {
				if( !(path.length = encoding.length = pathBase ) ) fieldMap = new WeakMap();
				return ref;
			}else { //if( asField )
				fieldMap.delete(object)
				//console.log( "Deleting object reference from fieldMap" );
                        }
		}

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
		
		const r  = str( asField, {[asField]:object} );
		sack.JSOX.stringifierActive = stringifier_;
		commonClasses.length = 0;
		if( !(path.length = encoding.length = pathBase ) ){
			fieldMap = new WeakMap();
		}else{
			//console.log( "Stringifier is still in a stack?", path);
		}
		DEBUG_STRINGIFY_OUTPUT && console.trace( "Stringify Result:", r );
		return r;





		// from https://github.com/douglascrockford/JSON-js/blob/master/json2.js#L181
		// (highly modified since then)
		function str(key, holder) {
			//console.trace( "-------------------------------------- STR -----------------------", key, holder );
			//console.log( "Encode object:", holder[key], "field:", key );
			function doArrayToJSOX() {
				var v;
				var partial = [];
				const thisNodeNameIndex = path.length;
				{
					// The value is an array. Stringify every element. Use null as a placeholder
					// for non-JSOX values.
					for (let i = 0; i < this.length; i += 1) {
						path[thisNodeNameIndex] = i;
						partial[i] = str(i, this) || "null";
					}
					path.length = thisNodeNameIndex;
					//console.log( "remove encoding item", thisNodeNameIndex, encoding.length);
					encoding.length = thisNodeNameIndex;
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

			function mapToObject(){
				var tmp = {tmp:null};
				var out = '{'
				var first = true;
				//console.log( "CONVERT:", map);
				for (var [key, value] of this) {
					//if( "function" === typeof value ) continue;
					//console.log( "er...", key, value )
					tmp.tmp = value;
					const thisNodeNameIndex = path.length;
					path[thisNodeNameIndex] = key;
					out += (first?"":",") + getIdentifier(key) +':' + str("tmp", tmp);
					path.length = thisNodeNameIndex;
					first = false;
				}
				out += '}';
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
			const thisNodeNameIndex = path.length;
			var value = holder[key];
			let isObject = (typeof value === "object");
			if( "string" === typeof value ) value = getIdentifier( value );
			_DEBUG_STRINGIFY &&
				console.log( "Prototype lists:", localToProtoTypes.length, value && localToProtoTypes.get( Object.getPrototypeOf( value ) )
					, value && Object.getPrototypeOf( value ), value && value.constructor.name
					);

			if( isObject && ( value !== null ) ) {
				if( objectToJSOX ){
                                    	_DEBUG_STRINGIFY &&
						console.log( "doing generic object conversion phase..." );
					if( !stringifying.find( val=>val===value ) ) {
						stringifying.push( value );
						encoding[thisNodeNameIndex] = value;
						value = objectToJSOX.apply(value, [stringifier]);
	                                    	_DEBUG_STRINGIFY &&
        	                                        console.log( "Value should still be value:", value );
						/*
						if( value !== encoding[thisNodeNameIndex] )
							console.log( "Converted by object lookup -it's now a different type"
								, Object.getPrototypeOf(encoding[thisNodeNameIndex])
								, Object.getPrototypeOf(value )
								, protoConverter, objectConverter );
						*/
						stringifying.pop();
						encoding.length = thisNodeNameIndex;
						isObject = (typeof value === "object");
					}
					//console.log( "Value convereted to:", key, value );
				}				
			}
			var protoConverter = (value !== undefined && value !== null)
				&& ( localToProtoTypes.get( Object.getPrototypeOf( value ) )
				|| toProtoTypes.get( Object.getPrototypeOf( value ) )
                                || toProtoTypesByName.get( Object.getPrototypeOf( value ).constructor.name )
				|| null )
			var objectConverter = !protoConverter && (value !== undefined && value !== null)
				&& ( localToObjectTypes.get( Object.keys( value ).toString() )
				|| toObjectTypes.get( Object.keys( value ).toString() )
				|| null )

			//_DEBUG_STRINGIFY && console.log( "TEST1()", value, protoConverter, objectConverter, localToProtoTypes );

			var toJSOX = ( protoConverter && protoConverter.cb )
			             || ( objectConverter && objectConverter.cb )
						 //|| ( isObject && objectToJSOX )
						 ;
			// If the value has a toJSOX method, call it to obtain a replacement value.
			//_DEBUG_STRINGIFY && console.log( "type:", typeof value, protoConverter, !!toJSOX, path, isObject );

			if( value !== undefined
				&& value !== null
				&& typeof value === "object"
			    && typeof toJSOX === "function"
			) {
				// is encoding?
				if( !stringifying.find( val=>val===value ) ) {
					gap += indent;
					if( typeof value === "object" ) {
						v = getReference( value );
						if( v ) return v;
					}
					stringifying.push( value );
					encoding[thisNodeNameIndex] = value;
					value = toJSOX.call(value, stringifier);
					stringifying.pop();
					if( protoConverter && protoConverter.name ) {
						// stringify may return a unquoted string
						// which needs an extra space betwen its tag and value.
						if( "string" === typeof value 
							&& value[0] !== '"'
							&& value[0] !== '\'' 
							&& value[0] !== '`' 
							&& value[0] !== '[' 
							&& value[0] !== '{' 
							){
							value = ' ' + value;
						}
					}
					//console.log( "Value converted:", value );
					encoding.length = thisNodeNameIndex;

					gap = mind;
				} else {
					v = getReference( value );
				}
			} else
				if( typeof value === "object" ) {
					v = getReference( value );
				}

			_DEBUG_STRINGIFY && console.log( "Protoconverter leftover: ", protoConverter );
			// If we were called with a replacer function, then call the replacer to
			// obtain a replacement value.

			if (typeof rep === "function") {
				value = rep.call(holder, key, value);
			}

			// What happens next depends on the value's type.
			switch (typeof value) {
			case "bigint":
				return value + 'n';
				break;
			case "string":
			case "number":
				{
					let c = '';
					//console.log( "outputting string result:", value, c );
					if( key==="" ) {
						c = classes.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"")
						    || commonClasses.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"");
					}
					if( protoConverter && protoConverter.external ){
						//  && protoConverter.proto === Object.getPrototypeOf(value) && protoConverter.name
						return c + protoConverter.name + value;
					}
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

				_DEBUG_STRINGIFY && console.log( "ENTERING OBJECT EMISSION WITH:", v, value );
				if( v ) return v;

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
					_DEBUG_STRINGIFY && console.log( "Working through replacer" );
					partialClass = matchObject( value, rep );
					for (i = 0; i < length; i += 1) {
						if (typeof rep[i] === "string") {
							k = rep[i];
							path[thisNodeNameIndex] = k;
							//console.log( "set encoding item", thisNodeNameIndex, encoding.length);
							encoding[thisNodeNameIndex] = value;
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
					path.length = thisNodeNameIndex;
					//console.log( "remove encoding item", thisNodeNameIndex, encoding.length);
					encoding.length = thisNodeNameIndex;
				} else {

					// Otherwise, iterate through all of the keys in the object.
					partialClass = matchObject( value );
					var keys = [];
					_DEBUG_STRINGIFY && console.log( "is something in something?", k, value );
					for (k in value) {
						_DEBUG_STRINGIFY && console.log( "Ya...", k );
						if( ignoreNonEnumerable )
							if( !Object.prototype.propertyIsEnumerable.call( value, k ) ){
								_DEBUG_STRINGIFY && console.log( "skipping non-enuerable?", k );
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
							if( n === keys.length )
								keys.push(k);
						}
					}
					_DEBUG_STRINGIFY && console.log( "Expanding object keys:", v, keys );
					for(let k of keys ) {
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
					path.length = thisNodeNameIndex;
					//console.log( "remove encoding item", thisNodeNameIndex, encoding.length);
					encoding.length = thisNodeNameIndex;
				}

				// Join all of the member texts together, separated with commas,
				// and wrap them in braces.
				_DEBUG_STRINGIFY && console.log( "partial:", partial, protoConverter )
				{
				let c;
				if( key==="" )
					c = classes.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"")
					    || commonClasses.map( cls=> cls.name+"{"+cls.fields.join(",")+"}" ).join(gap?"\n":"")+(gap?"\n":"");
				else
					c = '';

				if( protoConverter && protoConverter.external ) {
					c = c + getIdentifier( protoConverter.name );
				}

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
				_DEBUG_STRINGIFY && console.log(" Resulting phrase from this part is:", v );
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
		} else if (byteRemainder === 2) {
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
[ [ 0,264,[ 0xffd9ff,0xff6aff,0x1fc00,0x380000,0x0,0xfffff8,0xffffff,0x7fffff,0x800000,0x0,0x80 ] ] ].map( row=>{ return{ firstChar : row[0], lastChar: row[1], bits : row[2] }; } );

}
