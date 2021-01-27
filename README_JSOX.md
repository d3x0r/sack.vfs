

## [JSOX](https://www.github.com/d3x0r/jsox), and JSON( [JSON6](https://www.github.com/d3x0r/json6) ) - JSON and JSON6 compatible processor 

[JSOX](https://www.github.com/d3x0r/jsox) further extends JSON6 with Date support(subtype of numbers), BigInt support and TypedArray encoding.
It also adds the ability to define classes, which reduces data replication in the output, but also reduces
overhead serializing by having to parse shorter data; and gathering just values instead of `<Field> ':' <Value>` is less output usage.
Also, objects that are recovered with a class tag share the same prototype, allowing the objects read to be more readily used in applications.
JSOX parsing is 100% compatible with JSON6, JSON, and ES6(non code/function) parsing; that is JSOX can read their content with no issues.  There
is one small exception, when reading JSON6 in a streaming mode, it was possible to parse `1 23[45]` as `1`,`23`,`[45]` (similarly with {} instead of []), 
but JSOX requires at least one whitepsace between a number and a open brace or bracket, otherwise it tries to interpret it as a class tag.

JSON6 is a slightly extended json parser to allows simple types to be returned as values, not requiring { or [ to start
the JSON input.  JSON6 parsing is 100% compatible with JSON parsing; that is JSON6 can read JSON content with no issues.

Simple values, objects and arrays can result from parsing.  Simple values are true,false,null, Numbers and Strings.

Added support 'reviver' parameter.


  - JSON
     - parse( string [,reviver] )
     - begin( callback [,reviver] )
         - write( data )
  - JSON6
     - parse( string [,reviver] )
     - begin( callback [,reviver] )
         - write( data )
  - JSOX
     - parse( string [,reviver] )
     - begin( callback [,reviver] )
         - write( data ) - post data to parse, array buffer(utf8) or string allowed throws on error.  Is synchronous with callback.
         - reset( ) - if an error happened during a write, this resets the error state to allow more data.
         - parse() - if no callback is specified, parse can be used to get one-shot messages.
         - fromJSOX(typeName,prototype,fromCb) - if an object of the specified name is encountered, the related object/array/string is passed to the callback, and the result is used as the revived object.
     - stringifier() - create a reusable stringifier which can be used for custom types
         - toJSOX(typeName,prototype,cb) - if an object that is an instance of prototype, the object is passed to the callback, and the resulting string used for output.
         - stringify(value[,replacer[,space]] ) - stringify using this stringifier.
         - setQuote( quote ) - specifies the perfered quoting to use (instead of default double-quote (") )
         - defineClass( name, object ) - registers a typed object for output; first object defintion is output which contains the failes, and later, typed objects are just their values.  Uses prototype of the object, unless it is the same as Object, or uses the fields of the object to compare that the object is of the same type.
     - stringify(object,replacer,pretty) - stringify an object; same API as JSON.
     - toJSOX(typeName,userType,cb) - if an object that is an instance of prototype, the object is passed to the callback, and the resulting string used for output.  cb(stringify)
     - fromJSOX(typeName,userType,fromCb) - (fromCb(val,field); undefinedat end ) if an object of the specified name is encountered, the related object/array/string is passed to the callback, and the result is used as the revived object.
     - addType( name,userType,toCb, fromCb) - register both ToJSOX and FromJSOX handlers.


|JSOX Methods | parameters | Description |
|-----|-----|-----|
|parse| (string [,reviver]) | supports all of the JSOX features listed above, as well as the native [`reviver` argument][json-parse]. |
|stringify | ( value[,replacer[,space]] ) | converts object to JSOX.  [stringify][jsox-stringify] |
|stringifier | () | Gets a utility object that can stringify.  The object can have classes defined on it for stringification |
|escape | ( string ) | substitutes ", \, ', and \` with backslashed sequences. (prevent 'JSON injection') |
|begin| (cb [,reviver] ) | create a JSOX stream processor.  cb is called with (value) for each value decoded from input given with write().  Optional reviver is called with each object before being passed to callback. |
|toJSOX  | (name,userType,toCb) | For each object that matches the prototype, the name is used to prefix the type; and the cb is called to get toJSOX |
|fromJSOX| (name,userType,fromCb) | fromCb is called whenever the type 'name' is revived.  The type of object following the name is passd as 'this'. |
|addType | (name,userType,toCb, fromCb) | register both to and from for the same name |


Reviver callback parameter is not provided for streaming callback.

``` javascript
import {sack} from sack.vfs;
//var vfs = require( "sack.vfs" );

//var object = vfs.JSON.parse(string [, reviver]);

var object2 = sack.JSON6.parse(string [, reviver]);

const object3 = sack.JSOX.parse(string [, reviver]);

```

### Active Stringifier 


The stringifier is passed as a paramter to the toJSOX callback, which is where the active Stringifier was used.
JSOX JS version does not support this interface.

```
sack.JSOX.stringifierActive = <stringifier that is currently being run>
```

### Stringifier reference

|Stringifier method | parameters | Description |
|-------|------|-----|
|encodeObject | (o) | Call the default object to JSOX method (if there is one); this allows specific types to be resolved as lower level objects |
|getReference | (object or array) | Returns a string reference path for the object (if there is one) |
|stringify | (value[,replacer[,space]] ) | converts object to JSOX attempting to match objects to classes defined in stringifier.  [stringify][json-stringify] |
|setDefaultObjectToJSOX| (cb) | sets the function to call when an otherwise unknown object class is encoded |
|isEncoding | (object) | returns whether the current object is being encoded
|setQuote | ( quote ) | the argument passed is used as the default quote for strings and identifiers as required. |
|defineClass | ( name, object ) | Defines a class using name 'name' and the fields in 'object'.  This allows defining for some pre-existing object; it also uses the prototype to test (if not Object), otherwise it matches based on they Object.keys() array. |
|ignoreNonEnumerable | setter/getter | allows controlling whether to include fields that have been marked non-enumerable |

|Parser Methods | parameters | Description |
|-----|-----|-----|
|write | (buffer) | add data to the parser stream |
|reset| () | When an error is thrown from parsing a stream, the parser must be reset using `reset()`, otherwise it will continue throwing an error. |
|parse | (buffer) | use a parser instance to parse a single message |
|currentRef | () | Returns an object containing `{o:, f:}` where `o` is the object containing the field being revived, and `f` is the name of the field being revive; together this pair makes a reference to a value.  |
|fromJSOX| (name,Function/Class,fromCb) | fromCb is called whenever the type 'name' is revived.  The type of object following the name is passd as 'this'. Will throw an exception if duplicate set happens. |



### Streaming JSON/JSOX Parsing

  - begin(cb)  returns a parser object, pass a callback which receives objects as they are recognized in the stream.
    - parser.write( data )  Write data to the parser to parse into resulting objects.

Simple values, objects and arrays can result from parsing.
because a streaming input is expected, cannot pass just simple objects to be parsed, because an input of "1234" might
be just part of a number, and more of the number might be added in the next write.  If there is a trailing newline or other 
whitespace character after the number, the number will be sent to the callback registered in the begin.

``` javascript
import {sack} from "sack.vfs";

var parser = sack.JSON6.begin( objectCallback );
parser.write( "123 " );
function objectCallback( o ) {
    console.write( "Received parsed object:", typeof o, o );
}

// another example.
var streamExample = sack.JSON6.begin( (o)=> { console.log( "received value:", o ) };
streamExample.write( "123\n" );   // generate a single event with number '123'
streamExample.write( '"A string"' );    // generate another event with the string
streamExample.write( '{a : 1} { b : 2}{c:3}' );   // generates 3 callback events, 1 for each object

```

