
export const pwdBare = process.cwd();

export async function reloadConfig() {
	return   (await import( (process.platform==="win32"?"file://":"")+pwdBare+"/"+(process.env.TASK_MANAGER_RUN_CONFIG||"config.run.jsox") ).catch( err=>(console.log( "parsing error:", err),{default:null}) )).default
			|| (await import( (process.platform==="win32"?"file://":"")+pwdBare+"/"+(process.env.TASK_MANAGER_TASK_CONFIG||"config.tasks.jsox") ).catch( err=>(console.log( "parsing error:", err),{default:null}) )).default
	      || (await import( (process.platform==="win32"?"file://":"")+pwdBare+"/"+(process.env.TASK_MANAGER_CONFIG||"config.jsox") ).catch( err=>({default:null}) )).default 
	      || { extraModules:[]
	         , hostname:""
	         , useUpstream: false
	         , upstreamServer: ""
	         , port:0
	         , tasks:[] 
	         };

	return config;
}
