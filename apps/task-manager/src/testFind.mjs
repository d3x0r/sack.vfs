

import {findTask,createTask,deleteTask} from "./schTask.mjs"

const task = {name:"Calculator Test", bin:"calc.exe", work:"c:\\" };
findTask( task ).then( (yesno)=>{
	if( yesno )
		deleteTask( task );
	else
		createTask( task );
} );

