

import {findTask,createTask,deleteTask} from "./schTask.mjs"

const task = {name:"Calculator Test", bin:process.cwd() + "\\msedge.bat", work:"c:\\" };
findTask( task ).then( (yesno)=>{
	if( yesno )
		deleteTask( task );
	else
		createTask( task );
} );

