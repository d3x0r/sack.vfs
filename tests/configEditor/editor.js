
var sack = require( "../.." );
var disk = sack.Volume();

var dialog = sack.PSI.Frame( 512, 512 );//.load( "editor.frame" ); // if not loaded, gets a default frame.

//console.log( "To Open Editor..." );
//sack.Sqlite.optionEditor();
//console.log( "Editor opened... (or not?)" );

// default borders will be made too; if you want to remove any extra 
// fany border can set this to null.

// because of the border, these set colors on the dialog border, 
// so anyother thing also using this border will inherit these colors.
// a dialog's own colors are only used if there is no border.
function loadScript() {

	var list = dialog.Control( "listbox", 5, 5, 200, 252 );
	var file;
	try {
		 file = disk.read( process.argv[2] ).toString();
	} catch(err) {
		console.log( "EXIT!" );
		return;// process.exit();
	}
	var config = sack.JSON6.parse( file );

	Object.keys(config).forEach( key=>{ 
		var obj = list.addItem( key );
		console.log( "result:", obj );
		obj.key = key;
	} );
	list.onSelect( function(){
		valueField.text = JSON.stringify( config[this.key] );
	} );
	var textLabel = dialog.Control( "TextControl", "Value", 5+200 + 5, 5, 50, 20 );
	var valueField = dialog.Control( "EditControl", 5+200+5+50+5, 5, 180, 20 );

	var btn = dialog.Control( "Button", "Done", 250, 200, 50, 20 );
	btn.on( "click", ()=>{
		console.log( "OK clicked." );
		process.exit();
	} );

	var btn = dialog.Control( "Button", "Update", 500, 5, 50, 20 );
	btn.on( "click", ()=>{
		console.log( "OK clicked." );
		process.exit();
	} );

	

	dialog.Control( "TextCOntrol", "Option Database", 5, 365, 200, 20 );
	var optlist = dialog.Control( "listbox", 5, 385, 200, 252 );

	sack.Sqlite.eo( (opt,name)=>{
		console.log( "got:", opt );
		var item = optlist.addItem( name );
		item.opt = opt;
		item.open = false;
	});
	optlist.onSelect( function(){
		this.opt.eo( (opt,name)=>{
			console.log( "got:", name, opt );
			optlist.addItem( name, 1 );
		} );
	} );


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
}

loadScript();

