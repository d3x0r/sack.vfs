

var config = require ('./config.js');

setTimeout( waitrun,10 );

function waitrun(){
    //console.log( "Wait for config");
    if( !config.run.Λ)
        setTimeout(  waitrun, 10 );
    else
        try { run(); } catch(err) { console.log( err);run3(); }
}

function run2() {
    var discoverer = require( "http://localhost:3212/discovery.js" );
    discoverer.discover( { timeout: 1000
        , ontimeout : ()=>{
            console.log( "i'm all alone. but all I can do is wait" );
        }
        , onconnect : ( sock ) => {
            console.log( "possible addresses ...", config.run.addresses );
            var idGen = require( "./id_generator.js");
            idMan.sync( sock );
        }
    })

}

function run3() {
    var discoverer = require( "./discovery.js" );
    discoverer.discover( { timeout: 1000
        , ontimeout : ()=>{
            console.log( "i'm all alone. but all I can do is wait" );
        }
        , onconnect : ( sock ) => {
            console.log( "possible addresses ...", config.run.addresses );
            var idGen = require( "./id_generator.js");
            idMan.sync( sock );
        }
    })

}

function run() {
    var Cluster = require( './cluster.js' );
    var idMan = require( "./id_manager.js");
    var vfs = require( "./file_cluster.js" );
    var discoverer = require( "./discovery.js" );
    discoverer.discover( { timeout: 1000
        , ontimeout : ()=>{
            console.log( "i'm all alone.", config.run.Λ )
            Cluster.start();
            idMan.SetKeys( idMan.ID( config.run.Λ ) );
        }
        , onconnect : ( sock ) => {
            var idGen = require( "./id_generator.js");
            idMan.sync( sock );
        }
    })
}


//discoverer.
