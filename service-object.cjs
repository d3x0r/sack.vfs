
module.exports = exports = function (sack) {


	function install( name, desc, ...args ) {
		const pname = sack.Task.name;//GetProgramName();
		const ppath = sack.Task.path;//GetProgramPath();

		sack.Task( {bin:"sc.exe", args:["create", name, "start=auto", "binpath=", `"${serviceName} ${...args.join(' ')}"`] } );
		sack.Task( {bin:"sc.exe", args:["description", name, desc] } );
		sack.Task( {bin:"sc.exe", args:["start", name ] } );
		
	}

	function uninstall( name ) {
	}

}
