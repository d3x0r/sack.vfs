const JSOX = require( "../.." ).JSOX;

const o = JSOX.parse ('{op:f,id:2,ret:"mAOyCJ59I3Y0$79Z_B2y0uM6ofFk$$OQnvf4MZsRSS0="}' )
console.log( "o:", o );

const o2 = JSOX.parse( `{op:'f',id:1,f:'create',args:["MOOSE-HTTP","(HTTP)Master Operator of System Entites.","startupWeb.js"]}` );
console.log( "qo:", o2 );