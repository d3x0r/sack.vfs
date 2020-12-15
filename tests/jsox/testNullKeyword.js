const util = require('util');
const JSOX = require("../..").JSOX;

const obj = JSOX.parse( "[null,null,null, null , null,null ,null]" );
console.log( "object:", obj );
