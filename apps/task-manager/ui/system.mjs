
export class System {
	id = "unique id";
	system = "hostname";
	tasks = null;
	port = 0; // system:port should be able to be connected to...
	page = null; // used for the GUI to associate the datagrid
	upstream = null; // tracks which system owns this system by upstream relation 
	#connection = null; // websocket connection to this system(saves IP address)
	#taskMap = {};
	get address() {
		return this.#connection.address;
	}
	get connection() {
		return this.#connection;
	}
	constructor(connection, id, port, system, tasks ) {
		this.id = id;
		this.port = port;
		this.#connection = connection;
		this.system = system;
		this.tasks = tasks;
	}
	createTask( msg ) {
		this.#connection.send( msg );
	}
	addTask( id, task ) {
		this.tasks.push( task );
		this.#taskMap[id] = task;

	}
	updateTask( id, task ) {
		const checkTask = this.#taskMap[id];
		if( checkTask )
			Object.assign( checkTask, task );
		if( this.dataGrid ) this.dataGrid.refresh();
	}
	deleteTask( id ) {
		const task = this.#taskMap[id];
		for( let t = 0; t < this.tasks.length; t++ ){
			const checkTask = this.tasks[t];
			if( checkTask === task ) {
				this.tasks.splice( t, 1 ); 
				return;
			}
		}
		if( this.dataGrid ) this.dataGrid.refresh();

	}
}
