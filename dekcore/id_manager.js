"use strict";

const fs = require( 'fs');
var config = require( './config.js' );
console.log( "config is ", config.run.root );
const fc = require( config.run.root + '/file_cluster.js')
console.log( "config is ", config.run.root );
const Entity = require( config.run.root + '/entity.js')
console.log( "config is ", config.run.root );

//const  = require( './config.js' );
var idGen = require( "./id_generator.js").generator;
var key_frags = new Map();
// key_frags[key] = keys
// #1: { }
    //Map.prototype.length = function() { return Object.keys( keys ).length - 5
//}
var keys = new Map();
//keys["Λ"] =  config.run.Λ;
     //length: ()=>{ return Object.keys( keys ).length - 5 }
    , keys.Λ : config.run.Λ
    , first : null
    //, forEach: (f)=>{ Object.keys(keys).forEach((k)=>{ if( k.Λ ) { return(k);} });}
    /*
    keys.toString: () => {
        let buffer = undefined;
        Object.keys(keys).forEach( (key,index,mem)=>{
            if( keys[key] && keys[key].Λ ){
                //console.log( "is a candidate.", key );
                if( !buffer ) buffer = '{'; else buffer += ',';
                buffer +='"' + keys[key].Λ +'":' +  keys[key].toString()  ;
            }
            });
        if( buffer ) {
            buffer += '}';
        }
        return buffer;
    }
    */
};// = [{ key : idGen.next(), maker : key, trusted : true } ];

function Key(key) {
    var key = { maker : ( key && key.Λ ) ? key.Λ : key
            , Λ : idGen()
            , authby : null
            , requested : 0  // count
            , trusted : false // boolean
            , invalid : false // fail all auth and do not persist
            , authonly : true // don't send copies to other things
            , created : new Date().getTime()
            , timeOffset : config.run.timeOffset
            , toString : ()=> {
                if( !mystring )
                    mystring = JSON.stringify( key );
                return mystring;
            }
        };

        var mystring;
        //console.log( "generated key ", key.Λ )
        return key;
}

function KeyFrag( fragof ) {
    return {
        Λ : ID( fragof ).Λ
        //_ :
        , keys : []
        , toString : ()=>{
            var mystring;
            if( !mystring )
                mystring = keys.toString();
            return mystring;
        }
    }
}

exports.Auth = ( key ) => {
    if( keys[key])
        return validateKey( key );
    return false;
}

exports.ID = ID; function ID( making_key, authority_key )  {
    //console.log( "Making a key...... ")
    //console.log( config.run.Λ );
    //console.log( making_key );
    //console.log( keys[making_key.Λ] );
    //console.log( authority_key );
    if( making_key === authority_key )
        throw "Invalid source keys";
    if( !making_key || !validateKey( making_key ) )
        if( !authority_key || !validateKey( authority_key ) )
            throw "Invalid source key";
    var newkey = Key( making_key );
    keys[newkey.Λ] = newkey;
        if( authority_key )
            newkey.authority = authority_key
        //console.log( "made a new key", keys );
        return newkey;
}

    function validateKey( key, callback ) {
    let ID;
    if( !key ) {
        return false;
    }
    //console.log( "validate key ", key )
    if( key.Λ ? (ID= key):(ID=keys[key] ) )
    {
        //console.log( "validate key had a key find", ID)
        if( ID.trusted )
            return true;
        if( ID.key == key )
            return true;
        ID.requested++;
        if( ID.invalid )
            return false;
        if( key === config.Λ ) {
            return true;
        }
        return validateKey( ID.maker );
    }
    else {
        // requestKey and callback?

        if( callback ) {
            callback( yesno );
        }
        console.log( "no such key" )
        //exports.ID( config.run.Λ );
        //return true;
    }
    return false;
}

function saveKeys() {
    var fileName = "core/id.json"
    //console.log( "save keys", keys );
        fs.stat(fileName, function(error, stats) {
            fs.open(fileName, "w", function(error, fd) {
                var buffer;
                    console.log( "-------- Save Keys In open File ")
                        buffer = undefined;

                        Object.keys(keys).forEach( (key,index,mem)=>{
                            //console.log( "foreach does ", key )
                            if( key == 'first' ) return;
                                if( keys[key] && keys[key].Λ ){
                                    //console.log( "is a candidate.", key );
                                    if( !buffer ) buffer = '{'; else buffer += ',';
                                    buffer +='"' + keys[key].Λ +'":' +  keys[key].toString()  ;
                                }
                        });
                        if( buffer ) {
                            buffer += '}';
                        // keys isn't happy as a stringify....
                        // var buffer = JSON.stringify( keys );
                        // have to hide first as an anum from json
                        //console.log( "just for fun the json would be ", JSON.stringify( keys ));
                            fs.write(fd, buffer, 0, buffer.length, null, function(error, bytesRead, buffer) {
                             if( error ); }
                            );
                        }
                   fs.close(fd);
            });
          });
}


function loadKeyFragments( o ) {

        var result;
        console.log( 'loadkeyFragment',  o )
        fc.reloadFrom( o, (error, files)=>{

            if( error ){
                    console.log(" koadfrom directoryerror: " + error)
            }
            else if( files.length === 0 ) {
                console.log( "no files...")
                return;
            }
        //var fileName = fs.;
        console.log( "loading fragments ", files );
        if( !error )
        fc.reload( files, (error, buffer )=> {
            console.log( "reload id : ", error, " ", buffer)
            if( error ) {
                  /* initial run, or the file was invalid*/
                  console.log( "can only load fragments that exist so... ", error );
                }else {
                  var data = fc.Utf8ArrayToStr( buffer );
                  try {
                      //console.log( "about to parse this as json", data );
                      keys = JSON.parse( data );
                      //console.log("no error", keys);
                      //if( keys.length === 0 )
                  }catch(error) {
                      console.log( data );
                      console.log( "bad keyfrag file", error );
                  }
              }
          });
      });
}

function loadKeys() {

    var result;
    var fileName = "core/id.json";
    fc.reload( fileName, (error, buffer )=> {
        if( error ) {
                if( !keys ) {
                    //let newkey =  idGen.generator();
                    //console.log(  newkey.toString() );
                    console.log( "new id generator id created" );
                    var newkey = Key(newkey);
                    newkey.trusted = true;
                    newkey.maker = newkey.Λ;
                    keys[newkey.Λ] = newkey;
                    console.log( keys[newkey] )
                }
                saveKeys();
            }else {
              var data = fc.Utf8ArrayToStr( buffer );
              try {
                  //console.log( "data to parse ", data);
                  var newkeys = JSON.parse( data );
                  Object.assign( newkeys, keys );
                  //console.log( "new keys after file is ... ", newkeys );
                  keys = newkeys;
                  //console.log( "new keys after file is ...and re-keying it... ", keys.length());
                  //console.log( )
                  //Object.defineProperty( keys.define
              }catch(error) {
                  console.log( "bad key file", error );
                  //fs.unlink( fileName );
              }
          }
              //console.log("no error", keys.length());
              //#ifdef IS_VOID
              if( keys.size === 0 )
                {
                    //console.log( Object.keys( keys ).length)
                    //console.log( "NEED  A KEY")
                    var key = Key( config.run.Λ );
                    key.authby = key.Λ;
                    key.trusted = true;
                    keys[key.Λ] = key;
                    keys.first = keys[key.Λ];
                    var runkey = ID( key );
                    runkey.authby = key.Λ;
                    //console.log( "throwing away ", runkey.Λ );
                    delete keys[runkey.Λ];
                    keys[config.run.Λ] = runkey;
                    runkey.Λ = config.run.Λ;
                    //console.log( "new keys ", Object.keys(keys) );
                    let frag = key_frags[0];
                    if( !frag ) {
                    // keys is a valid type to pass to create
                    // but is any value.
                        key_frags = Entity( config.run ).create( keys );
                    //key_frags.Λ = Key( config.run.Λ ).Λ;
                    //frag = KeyFrag( config.run.Λ );
                    //frag_ent = Entity( frag  );
                    //frag_ent.value = keys;
                    //key_frags.store( frag_ent );

                    //key_frags.push( frag );
                    //frag.keys = keys;
                        console.log( "Initial Write?")
                        fc.store( config.run, key_frags );
                    }
                        //console.log( "fragment keys is a function??", key_frags[0] )
                //key_frags.keys[frag.Λ] = key.Λ;
                    saveKeys();

                }
                //#endif
                if( keys.size < 100 )
                {
                    console.log( "manufacture some keys......-----------")
                    Key( keys[0] )
                }

      });
}

function saveKeyFragments( ) {
    key_frags.forEach( (a)=>{
            console.log(" doing store IN", key_frags, a  );
            fc.store( key_frags, a );
    } )

}

exports.SetKeys = setKeys;
function setKeys( runkey ) {
    console.log( "O is config.run");
    loadKeyFragments( config.run );
    if( !key_frags.size ) {
        console.log( 'need some keys')
    }
    else {
        if( key_frags.size == 1 )
        {
            console.log( "Key_frags ... these should be on the disk....")
            //console.log( key_frags );
        //key_frags.forEach( (a)=>{
            //    console.log(" doing store IN", key_frags, a  );
                //fc.store( key_frags, a );
        //} )
        //fc.store( key_frags, key_frags.keys );
        }
    else {
        console.log( "recovered key_frags", key_frags.size, " plus one ");
    }
    }
}


//function
loadKeys();
