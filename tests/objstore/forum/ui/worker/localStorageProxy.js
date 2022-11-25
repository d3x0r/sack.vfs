
function getStorage( send ) {

const self_ = self;
const storageRequests = [];

const config = {run:{ devkey:null,
		clientKey : null,
		sessionKey : null
			} };


const localStorage = {
	getItem(key) {
		if( config.run[key] ) {
			return Promise.resolve(config.run[key]);
		}
		return new Promise( (res,rej)=>{
			storageRequests.push( {res:res,key:key} );
			send( {op:"getItem", key:key} );
		} );
	}
	, setItem(key,val) {
		config.run[key] = val;
		send( {op:"setItem", key:key, val:val} );
	}
	, respond( val ) {
		const dis = storageRequests.shift();
		config.run[dis.key] = val;
		dis.res(val);
	}
}
	return { config:config,
            	localStorage:localStorage }
}

export {getStorage}
