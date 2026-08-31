
export class System {
	id = "unique id";
	system = "hostname";
	tasks = null;
	port = 0; // system:port should be able to be connected to...
	page = null; // used for the GUI to associate the datagrid
	upstream = null; // tracks which system owns this system by upstream relation 
	#connection = null; // websocket connection to this system(saves IP address)
	#taskMap = {};
	#systems = []; // systems beyond this system that were upstreamed to this
	get address() {
		return this.#connection.address;
	}
	get connection() {
		return this.#connection;
	}
	// a peer that reconnects keeps its id but arrives on a new Connection;
	// without this setter the re-bind throws (getter-only, and modules are strict).
	set connection( val ) {
		this.#connection = val;
	}
	get systems() { return this.#systems }

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
		// same as deleteTask: the grid has to be rebuilt to gain a row.  Without
		// this the next refresh() walks a data row that has no rendered row and
		// dies on `cells`.
		if( this.dataGrid ) {
			this.dataGrid.reinit();
			this.dataGrid.fill();
		}
	}
	updateTask( id, task ) {
		const checkTask = this.#taskMap[id];
		if( checkTask )
			Object.assign( checkTask, task );
		if( this.dataGrid ) this.dataGrid.refresh();
	}
	deleteTask( id ) {
		// #taskMap only holds tasks added after construction, so match on the
		// id as well - otherwise nothing from the initial list could be found.
		const task = this.#taskMap[id];
		delete this.#taskMap[id];
		for( let t = 0; t < this.tasks.length; t++ ){
			const checkTask = this.tasks[t];
			if( checkTask === task || checkTask.id === id ) {
				this.tasks.splice( t, 1 ); 
				break;
			}
		}
		// refresh() only re-renders the cells the grid already has; dropping a
		// row needs the rebuild.  (This used to sit after an early return.)
		if( this.dataGrid ) {
			this.dataGrid.reinit();
			this.dataGrid.fill();
		}
	}
}
