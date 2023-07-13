
export let loadName = "";
export let wait = null;
export let _waitImports_ = null;

export let loadModules = null;

export function setupImports( name ) {
	const waiter = {
		name, imports : [] , resolve:null
	}
	wait = new Promise( (res,rej)=>{
		waiter.resolve = res;
	} );
	return waiter;
}


export function resolveImports( waiter, modules ) {
	_waitImports_ = modules;
	console.log( 'resolving:', waiter.name );
	waiter.resolve();
}
