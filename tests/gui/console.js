

var sack = require( "../.." );
var disk = sack.Volume();
require.extensions['.json6'] = function (module, filename) {
    var content = disk.read(filename).toString();
    module.exports = sack.JSON6.parse(content);
};

var dialog = sack.PSI.Frame.load( "console.frame" ); // if not loaded, gets a default frame.

if( !disk.exists( "console.font" ) ) {
	sack.Image.Font.dialog( (font)=>{
		console.log( "set font..." );
		font.save( "console.font" );
		dialog.font = font;	
		console.log( "setter called?" );
	} );
} else {
	dialog.font = sack.Image.Font.load( "console.font" );
}
var myBorderDef = require( "./windowFrame.json6" );
var border = sack.PSI.Frame.Border( myBorderDef );        
//console.log( "Got border:", border );
dialog.border = border;

dialog.edit()

dialog.on( "ok", ()=>{
	console.log( "OK clicked." );
	//dialog = null;  // let this evporate ?
	//dialog.close();  // close it?
	//dialog.hide();
	dialog.show();  // re-wait for a trigger the dialog.
} );
dialog.on( "cancel", ()=>{
	console.log( "Cancel clicked." );
} );
dialog.on( "abort", ()=>{
	console.log( "Abort clicked." );
} );


dialog.show();


