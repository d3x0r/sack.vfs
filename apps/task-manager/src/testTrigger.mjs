

import {findTask,createTask,deleteTask, triggerTask } from "./schTask.mjs"

const task = {name:"Calculator Test", bin:"calc.exe", work:"c:\\" };
findTask( task ).then( (yesno)=>{
	if( yesno )
		triggerTask( task );
} );

