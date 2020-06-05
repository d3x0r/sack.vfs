
var JSOX = require( "../.." ).JSOX;

console.log( JSOX.parse( JSOX.stringify( [{},[],null,[]] ) ) )

const str = `
entity{_module,attached_to,created_by,description,name,sandbox,value,within,$}[
  entity{
    {
      exports: {},
      file: "memory://",
      filename: "internal",
      includes: [],
      loaded: true,
      parent: null,
      paths: [
        "/home/d3x0r/work/javascript/dekcore/Entity/.."
],
      rawData: "",
      source: "this",
      src: null
    },
    [],
    null,
    "Glistening transparent black clouds swirl in the dark.",
    "The Void",
    {},
    null,
    "YOf5MX64PHyrm_KD_FWKMUsq3fRDBFcR2XR063AsM7A=",
    "YOf5MX64PHyrm_KD_FWKMUsq3fRDBFcR2XR063AsM7A="
  }
]
`

console.log( "Parsed:", JSOX.parse( str ) );
