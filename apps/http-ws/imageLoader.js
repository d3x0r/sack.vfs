// can count required and retreived for a progresss bar indicator...
let didWork = false;
let useOrigin = null;

let pending = [];

export async function wait() {
	await Promise.all( pending );
}

//------------------------ Image Library ---------------------------------

export function getImage(src) {
	return new Promise( (res,rej)=>{
		const i = new Image();
		let resi;
		let pi = new Promise( (res,rej)=>{
			resi = i;
		} );
		if( src[0] !== '/' && src.substring(0,4)!='http' && useOrigin ) 
			src = useOrigin + src;
		i.crossOrigin = "Anonymous";
		i.onload = ()=>{ 
			const idx = pending.find( p=>p===pi  );
			if( idx >= 0 ) pending.splice( idx, 1 );
			res( i );
		};
		i.onerror = ()=>{
			const idx = pending.find( p=>p===pi  );
			if( idx >= 0 ) pending.splice( idx, 1 );
			rej( src, i );
		}
		i.src = src;
		pending.push(pi);
	} );
}

export function getSound(src) {
	return new Promise( (res,rej)=>{
		var i = document.createElement( "AUDIO" );
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
	

export function setOrigin( origin ) {
	useOrigin = origin;
}