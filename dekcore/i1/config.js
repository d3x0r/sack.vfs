"use strict";
const fc = require( '../file_cluster.js');
const idGen = require( "../id_generator.js" );
const fs = require( 'fs');
const os = require("os");
//console.log( fc );
var config = module.exports = exports = {
    run : {
        Λ : undefined
        , hostname : os.hostname()
        , root : "."
         , addresses : []  // who I am...
         , friends : []  // discover should use this setting for off-network contact
         , timeOffset : new Date().getTimezoneOffset() * 60000
         , commit : () => {
            saveConfig();
        }
    }
}

function saveConfig() {
    fc.store( "core/config.json", config.run );
}

//res.sendfile(localPath);
//res.redirect(externalURL);
//
function loadConfig() {
    fc.reload( "core/config.json",
        function(error, result) {
            console.log( "loadCOnfig" );
            if( !error ) {
                //console.log( "attempt to re-set exports...", result);
                var str = fc.Utf8ArrayToStr( result )
                var object = JSON.parse( str );
                Object.assign( config.run, object );
                //config.run = object;
            }
            else
            {
                if( !config.run.Λ )
                    config.run.Λ =idGen.generator()
                saveConfig();
            }
        });
}

loadConfig();
