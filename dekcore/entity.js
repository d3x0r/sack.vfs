"use strict";

/*
This is to
*/
//var
var objects = [];

// Entity events
//  on( 'create', (self)=>{  } )
//  on( 'restore', (self)=>{ } )
//  on( "attachted", ( self,toOther)=>{ })
//  on( "dropped", ( self, old_container )=>{ } )
//  on( "detached", (self, fromOther){ } )
// on( "contained", ( self ){ } )
// on( "relocated", ( self, old_container )=>{ } )
// on( "rebase", ( self )=>{ } )

// onWake after Sentient

// Entity Methods
// Entity( maker ) // returns Entity
// e.create( Object )  // returns new e
//   with maker as object


// e.grab( something[ ,(something)=>{ /*something to run on success*/  }] )
//       if a thing is known, move it to internal storage
//           invoke onrelocated
//
// e.store( something )
// e.store( somthing [,inthing] )
// e.drop( thing )
//   thing has to have been contained in e
//   if thing is attachd to other things,
//        if that thing is the immediate content in e, all objects get moved.
//        if that thing is only attached to the single point, that thing itself wil be detatched
//             and that thing willl be dropped, receiving both detached and reloated events
// returns thing

// e.rebase(a)
//       if a is something that's attached, set that as the 'contained' objects
//          if it's not already the contained object... emit event rebase( a)
//    returns e
// e.debase()
//     if there is something attached to it, mkae that the contained objects
//         if( moved contained ) event rebase( a)
//          if( moved contained ) event newroot(a) to container
//    returns e


//
var idMan = require( './id_manager.js');
var fc = require( './file_cluster.js');
var config = require( './config.js');
//var temp = idMan.ID( obj, config.run.Λ )
//var temp = idMan.ID( config.run.Λ )

module.exports = exports = Entity


//Λ

function EntityExists( key, within ) {
    if( objects[key])
        return true;
    if( idMan.Auth( key ) ) {
        fc.restore( within, key, (error, buffer) => {
                if( error )
                        throw error;
                var e = Entity( config.run );
                e.fromString( buffer );
            } )
    }
}


function Entity( obj ){
    var o = {
        Λ : idMan.ID( obj, config.run.Λ )
        , within : obj
        , attached_to: []
        , contains : []
        , created_by : obj
        , loaded : false
        , has_value : false
        , value : null

        ,  get container() { return within; }
        , create( value ) {
            var newo = Entity( findContainer( o ) );
            newo.value = value;
        }
        , attach( a, b ) {
            var checked ;
            if ( isContainer(a, checked, o).Λ && isContainer(b, checked, o).Λ ) {
                a.within = null;
                if( o.Λ !== a.Λ ) {
                    if( a.within ){
                        a.attached_to[o.Λ]=o;
                        o.attached_to[a.Λ]=a;
                        o.emit( 'attached', a );
                        a.emit( 'attached', o );
                    }
                }
                else {
                    throw "cannot attach to self";
                }
            }
            else {
                throw "attachment between two differenly owned objects or one's not owned by you";
            }
         }
         , detach( a, b ) {
             if (o.Λ === a.within.Λ === b.within.Λ ) {
                 a.within = null;
                 if( o.Λ !== a.Λ ) {
                 delete a.attached_to[a.Λ];
                 delete o.attached_to[b.Λ];
                }
                else {
                    throw ""

                }
             }
          }
          , rebase( a ) {
                if( a.within ) return;
                outer = findContained( a );
                if( outer ) {
                    delete outer.within.contains[outer.Λ];
                    outer.within = null;
                }
          }
          , debase( a ) {
                if( a.within )
                    if( a.attached_to.length ) {
                        if( a.attached_to[0].within ) {
                            a.within = null;
                            throw "attempt to debase failed...";
                        }
                        a.attached_to[0].within = a.within;
                        a.within.contains[a.attached_to[0].Λ] = a.attached_to[0];
                        delete a.within.contains[a.Λ];
                        a.within = null;
                    }
                else {
                    return;
                }
          }
         , drop( a ) {
             var object
             var outer = o.within;
             if( !outer )
                outer = findContained( o );
            if( outer )
             if( a.within ) {

                 delete a.within.contains[a.Λ];
                 a.within = o.within;
                 o.within.emit( 'contained', a );
             }
             else{
                 if( o.within ) {
                 a.attached_to.forEach( (p)=>{delete p.attached_to[a.Λ] } );
                 a.within = o
             }
                 object = findContained( a )
                 if( object ) { drop( object )
                     delete object.within.contains[object.Λ] ;
                     object.within = o;
                     obj.within.emit( 'contained', a );
                }
                else {
                        a.within = o;
                    }
            }
             if( o.within )
            {
                o.within.contains[a.Λ] = a;
                a.within = o.within;
            }
            delete o.contains[a.Λ];
             var attachments = []
             var ac = getAttachments( a );
         }

         , store : ( a )=> {
             if( a.within ) {
                 delete ac.contains[a.Λ];
                 a.within = o;
             }
             else{ var object
               object = findContained( a )
                 object.within.contains[object.Λ]
                 object.within = o;
             }
            o.contains[a.Λ] = a;
         }
         , toString : ()=>{
             var attached = undefined;
             attached_to.forEach( (member)=>{if( attached ) attached += '","'; else attached = ' ["'; attached+= member.Λ})
             if( attached ) attached += '"]';
             else attached = '[]';
             var contained = undefined;
             contains.forEach( (member)=>{if( contained ) contained += '","'; else contained = ' ["'; contained+= member.Λ})
             if( contained ) contained += '"]';
             else contained = '[]';
            return '{"' + o.Λ + '":' + value.toString()
               + ',"within":"' +  o.within
               + ',"attached_to":' + attached
               + ',"contains":'+  contained
               + ',"created_by":"' + o.created_by
                + '}';
         }
         // fromString( "[]", )
         , fromString: ( string, callback ) => {
                 let tmp =  JSON.parse( string );
                 for( key in tmp) {
                     if( !idMan.Auth( key ) ) {
                        throw "Unauthorized object"
                        return;
                     tmp.Λ = key;
                     delete tmp[key];
                     break;
                 }
                 o.within = tmp.within || objects[tmp.within];
                 o.created_by = tmp.created_by || objects[tmp.created_by];
                 tmp.attached_to.forEach( (key)=>{ o.attached_to.push( objects[key] ) })
                 Object.assign( o, tmp );
             }
         }
    }
    o.attached_to[o.Λ] = o
    objects[o.Λ] = o;
    Object.assign( o, require('events').EventEmitter )
    console.log( "new is ", o);
    //o.emit( "create" );
    return o;

}

function findContained( obj, checked ){
    if( obj.within ) return obj;
    if( !checked )
        checked = [];
    for( content in obj.within ) {
        if( checked[content.Λ] )
            break;
        checked[content.Λ] = true;
        if( content.within ) return content;
        var result = findContainer( content, checked );
        if( result ) return result;
    }
    throw "Detached Entity";
}

function findContainer( obj, checked ){
    if( obj.within ) return obj.within;
    if( !checked )
        checked = [];
    for( content in obj.attached_to ) {
        if( checked[content.Λ] ) continue;
        checked[content.Λ] = true;
        if( content.within ) return content.within;
        var result = findContainer( content, checked );
        if( result ) return result;
    }
    throw "Detached Entity";
}

function isContainer( obj, checked, c ){
    if( obj.within ) return ( obj.within.Λ === c.Λ );
    if( !checked ) {
        checked = [];
        return recurse( obj, checked, c );
    } else {
        for( att in checked ) {
            if( att.within.Λ === c.Λ )
                return true;
        }
        return false;
    }

    function recurse ( obj, checked, c ) {
        for( content in obj.attached_to ) {
            if( checked[content.Λ] ) continue;
            checked[content.Λ] = true;
            if( content.within.Λ  == c.Λ ) return true;
            return recurse( content, checked, c  );
        }return false;
    }
    return false;
}


function getAttachments( obj, checked ){
    if( obj.within ) return obj.within;
    for( content in obj.attached_to ) {
        if( checked[content.Λ] )
            break;
        checked[content.Λ] = content;
        getAttachments( content, checked );
    }
    return checked;
    throw "Detached Object";
}
