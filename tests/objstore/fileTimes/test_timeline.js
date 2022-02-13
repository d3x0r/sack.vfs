
const sack = require( "../../.." );
const store = sack.ObjectStorage(  "storage.os" );

const timeline = store.timeline;


//const records = timeline.length;
console.log( "passing from int", new Date() );
const records = timeline.get( {from:0, limit:10} );
console.log( "Got:", records );


const opts2 = {from:new sack.JSOX.DateNS(), limit:10};
opts2.from.setDate(opts2.from.getDate()-3);
console.log( "Passing a from dateNS", opts2 );
const records2 = timeline.get( opts2 );
console.log( "Got:", records2 );


console.log( "Passing no from" );
const records3 = timeline.get( {read:true, limit:20} );
console.log( "Got:", records3.length, records3 );


const dataRecords = timeline.get({from:new Date( "2022-01-01T00:00:00"), limit:10, read:true } ); 