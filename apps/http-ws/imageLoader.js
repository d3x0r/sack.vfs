
export const moreWork = [];
const requiredImages = [];
// can count required and retreived for a progresss bar indicator...
const pendingLoad = [];
let didWork = false;
let useOrigin = null;

//------------------------ Image Library ---------------------------------
export function newImage(src) {  
	var i = new Image();
	var timeout;
	if( src[0] !== '/' && src.substring(0,4)!='http' && useOrigin ) 
		src = useOrigin + src;
	i.crossOrigin = "Anonymous";
	i.onerror = (err)=>{	
		console.log( "ERROR:", err );
		
	}
	i.onload = ()=>{ 
		var pl = pendingLoad;
		const rii = requiredImages.indexOf( i );
		if( rii >= 0 )
			requiredImages.splice( rii,1); 

		if( pl.length ) {
			var pi = pl.shift();
			requiredImages.push( pi.i ); 
			//console.log( "loading:", pi.src );
			window.lastImage = pi.i;
			pi.i.src = pi.src;
			tick();
			pendingLoad = pl;
			//return;
		}

			if( requiredImages.length == 0 ) 
				doWork(); 
	};
	i.onerror = ()=>{
		console.log( "Error result; removing item" );
		const rii = requiredImages.indexOf( i );
		if( rii >= 0 )
			requiredImages.splice( rii,1); 
		if( requiredImages.length == 0 ) 
			doWork(); 

	}

	//if( pendingLoad.length ) { pendingLoad.push( {i:i,src:src} ); return i; }
	//if( requiredImages.length ) { pendingLoad.push({i:i,src:src}); return i; }
	i.src = src;

	if( requiredImages.length == 0 ) {
		if( didWork ) console.log( "Clearing that we did work already");
		didWork = false;
	}
	requiredImages.push( i ); 

	return i;
}


export function getImage(src) {
	return new Promise( (res,rej)=>{
		var i = new Image();
		if( src[0] !== '/' && src.substring(0,4)!='http' && useOrigin ) 
			src = useOrigin + src;
		i.crossOrigin = "Anonymous";
		i.onload = ()=>{ 
			res( i );
		};
		i.onerror = ()=>{
			rej( src, i );
		}
		i.src = src;
	} );
}


//------------------------ Image Library ---------------------------------
export function newSound(src) {  
	var i = document.createElement( "AUDIO" );
	var timeout;
	i.crossOrigin = "Anonymous";
	i.onerror = (err)=>{	
		console.log( "ERROR:", err );
	}

	//if( pendingLoad.length ) { pendingLoad.push( {i:i,src:src} ); return i; }
	//if( requiredImages.length ) { pendingLoad.push({i:i,src:src}); return i; }
	i.src = src; 
//	requiredImages.push( i ); 

	return i;
}
	
export function newVideo(src) {  
	var i = document.createElement( "video" );
	var timeout;
	i.crossOrigin = "Anonymous";
	i.onload = ()=>{ 
		//clearTimeout( timeout );
		var pl = pendingLoad;
		requiredImages.pop(); 
		if( pl.length ) {
			var pi = pl.shift();
			requiredImages.push( pi.i ); 
			console.log( "loading:", pi.src );
			window.lastImage = pi.i;
			pi.i.src = pi.src;
			pendingLoad = pl;
			//return;
		}
		console.log( "newVIdeo got message: ", requiredImages.length );
		if( requiredImages.length == 0 ) doWork(); 
	};

	i.src = src; 
	//requiredImages.push( i ); 
	return i;
}
	
export function addWork( cb ) {
	if( !didWork && requiredImages.length )
		moreWork.push(cb);
	else
		cb();
}


async function doWork() {
	doOne(0);
	function doOne( n ){
		if( n < moreWork.length ){
			const w = moreWork[n];
			const result = w();
			if( result instanceof Promise ) 
				result.then( ()=>{
					doOne( n+1 );
				})			
			else	
				doOne( n+1 );
		}else {
			moreWork.length = 0;
			didWork = true;
		}
	}
}

export function setOrigin( origin ) {
	useOrigin = origin;
}