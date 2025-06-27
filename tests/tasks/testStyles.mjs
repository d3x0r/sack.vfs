
import {sack} from "sack.vfs"

const originalStyles = {
  window: [
    'WS_BORDER',
    'WS_CAPTION',
    'WS_CLIPCHILDREN',
    'WS_CLIPSIBLINGS',
    'WS_DLGFRAME',
    'WS_GROUP',
    'WS_MAXIMIZEBOX',
    'WS_MINIMIZEBOX',
    'WS_OVERLAPPEDWINDOW',
    'WS_SIZEBOX',
    'WS_SYSMENU',
    'WS_TABSTOP',
    'WS_THICKFRAME',
    'WS_TILEDWINDOW',
    'WS_VISIBLE'
  ],
  windowEx: [ 'WS_EX_WINDOWEDGE' ],
  class: [ 'CS_DBLCLKS' ]
};

const wantStyles = {
  window: [
    'WS_POPUP',
    'WS_SYSMENU', // so we can send a SC_CLOSE
    'WS_VISIBLE', 
	"WS_MINIMIZEBOX", "WS_MAXIMIZEBOX"
  ],
  windowEx: [],
  class: [ 'CS_DBLCLKS' ]
};

const nwtasks = sack.Task.getProcessList( "nw.exe" );
if( nwtasks && nwtasks.length ) {
	console.log( "task:", nwtasks[0], sack.Task.style, sack.Task.getTitle( nwtasks[0].id) );
	const styles = sack.Task.getStyles( nwtasks[0].id );
	const dec_styles = decodeStyles( styles );
        console.log( "Styles:", styles, dec_styles, encodeStyles( dec_styles ) );
	console.log( "Want styles:", wantStyles );
	const enc_styles = encodeStyles( wantStyles );
	sack.Task.setStyles( nwtasks[0].id, enc_styles.window );//, -1, -1 );
	const pos = sack.Task.getPosition( nwtasks[0].id );
	console.log( "At pos:", pos );
	pos.x /= 2;
	sack.Task.setPosition( nwtasks[0].id, pos );
}



function decodeStyles(styles ){
	const wStyles = Object.keys( sack.Task.style.window );
	const wXStyles = Object.keys( sack.Task.style.windowEx );
	const cStyles = Object.keys( sack.Task.style.class );
	const result = {window:[], windowEx:[], class:[] };
	for( let style of wStyles ){
		if( !sack.Task.style.window[style] ) continue;
		if( ( styles.window   & sack.Task.style.window[style] ) === sack.Task.style.window[style] )
			result.window.push( style );
		//console.log( "Testing: ", sack.Task.style.window[style].toString(16), styles.window.toString(16), result.window );
	}
	for( let style of wXStyles )
		if( sack.Task.style.windowEx[style] )
			if( ( styles.windowEx & sack.Task.style.windowEx[style] ) === sack.Task.style.windowEx[style] )
				result.windowEx.push( style );
	for( let style of cStyles )
		if( ( styles.class    & sack.Task.style.class[style] ) === sack.Task.style.class[style] )
			result.class.push( style );
	return result;
}


function encodeStyles(styles ){
	const result = {window:0, windowEx:0, class:0 };
	
	let s = 0;
	//console.log( "Window style:", styles.window );
	for( let style of styles.window   ) {
		s |= sack.Task.style.window[style];
		//console.log( "Window style:", s.toString(16), style );
	}
	result.window = s; s= 0;
	for( let style of styles.windowEx ) s |= sack.Task.style.windowEx[style];
	result.windowEx = s; s= 0;
	for( let style of styles.class    ) s |= sack.Task.style.class[style];
	result.class = s;
	return result;
}
