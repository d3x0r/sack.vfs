import {sack} from "sack.vfs"

const regVars = sack.Task.env;
const regVarKeys = Object.keys( regVars );
const envVarKeys = Object.keys( process.env );
console.log( "Vars:", regVars )

for( let i = 0; i < regVarKeys.length; i++ ) {
	if( regVarKeys[i] in process.env ) {
		if( regVars[regVarKeys[i]] !== process.env[regVarKeys[i]] ) {
			console.log( "Different value for:", regVarKeys[i] );
			console.log( " v1:", regVars[regVarKeys[i]] );
			console.log( " v2:", process.env[regVarKeys[i]] );
		}
		regVarKeys.splice( i, 1 );
		i--;
	}
}


for( let i = 0; i < envVarKeys.length; i++ ) {
	if( envVarKeys[i] in regVars ) {
		envVarKeys.splice( i, 1 );
		i--;
	}
}
console.log( "unmatched env:", envVarKeys );

for( let i = 0; i < regVarKeys.length; i++ ) {
	if( regVarKeys[i] in process.env ) {
		regVarKeys.splice( i, 1 );
		i--;
	}
}
console.log( "unmatched reg:", regVarKeys );


