import {Popup,popups} from "/node_modules/@d3x0r/popups/popups.mjs"
import {local} from "./local.js"
import {config as protocolConfig, protocol} from "./protocol.js"

export class TaskInfoEditor extends Popup {
	
	argVal = [];
	envKeys = [];
	page = 0;
	constructor( taskId, task_ ) {
		super( "Task Config Editor", local.display, { suffix:"-task-config", shadowFrame: true, enableClose: true });
		this.on("captionClose", ()=>{
			//console.log( "This sort of close? turn into remove?" );
			this.on( "close", true );
			this.remove();
		})
    this.hide();
		// injects...
		const styles = [popups.utils.addStyleSheetSrc( this, "/css/styles.css" )
		  ,popups.utils.addStyleSheetSrc( this, "/css/task-config.css" )];
    Promise.all( styles ).then( ()=>{
      this.show();
      this.center();
    })
		const task = Object.assign( {}, task_ );
		const taskOpts = {
			moveToEnable : false,
		}
		const flexFrame = document.createElement( "div" );
		flexFrame.className = "task-config-flex-frame";
		const buttonFrame = document.createElement( "div" );
		buttonFrame.className = "task-config-buttons";
		const headerContainer = document.createElement( "div" );
		headerContainer.className = "task-config-header";
		const headerTitleContainer = document.createElement( "div" );
		headerTitleContainer.className = "task-config-header-title";
		const headerTitle = document.createElement( "div" );
		headerTitle.className = "task-config-header-title-text";
		headerTitle.textContent = task.name || "New Task";
		headerTitleContainer.appendChild( headerTitle );
		headerContainer.appendChild( headerTitleContainer );
		headerContainer.appendChild( buttonFrame);
		this.appendChild( headerContainer );

		// this.appendChild( buttonFrame );

		this.appendChild( flexFrame );

		this.pageFrame =new popups.PagedFrame( flexFrame );
		const page1 = this.pageFrame.addPage( "General" );
		page1.tooltip = "General settings for the task";
		const page2 = this.pageFrame.addPage( "Options" );
		page2.tooltip = "Options to control how and where the task runs";
		const page3 = this.pageFrame.addPage( "Arguments" );
		page3.tooltip = "Arguments to pass to the task";
		const page4 = this.pageFrame.addPage( "Environment" );
		page4.tooltip = "Environment variables to set for the task";
		
		this.pageFrame.on( "activate", (page)=>{
			if( page === page1 ) {
				prevButton.style.visibility = 'hidden';
				nextButton.style.visibility = 'visible';
				this.page = 0;
			}
			if( page === page2 ) {
				prevButton.style.visibility = 'visible';
				nextButton.style.visibility = 'visible';
				this.page = 1;
			}
			if( page === page3 ) {
				prevButton.style.visibility = 'visible';
				nextButton.style.visibility = 'visible';
				this.page = 2;
			}
			if( page === page4 ) {
				prevButton.style.visibility = 'visible';
				nextButton.style.visibility = 'hidden';
				this.page = 3;
			}
		})

		if( task_) {
      this.create = popups.makeButton( buttonFrame, "Save", ()=>{
        processForm(false);
				this.on( "close", true );
        this.remove();
      } )
      this.create.tooltip = "Update this current task with the new settings";
    }

		this.saveAs = popups.makeButton( buttonFrame, "Save As", ()=>{
			const form = popups.simpleForm( "Save As", "Name", task.name, (name)=>{
				task.name = name;
				processForm(true);
				this.on( "close", true );
				form.remove();
				this.remove();
			} );
			form.show();
		} )
		this.saveAs.tooltip = "Pick a new name to save as a new task";

		if( !task ) {
			return;
		}
		this.group1 = document.createElement( "div" );
		this.group1.className = "task-config-group1"
		page1.appendChild( this.group1 );
		//this.name = popups.makeTextField( this.group1, task, "name", "Name" );
		//this.name.tooltip = "Name of the task";
		this.bin = popups.makeTextInput( this.group1, task, "bin", 'Program');
		this.bin.tooltip = "Program to run for this task";
		this.altbin = popups.makeTextInput( this.group1, task, "altbin", 'Alternate Program');
		this.altbin.tooltip = "Alternate Program to run for this task (if bin fails to run)";
		this.work = popups.makeTextInput( this.group1, task, "work", "Start In Path" );
		this.work.tooltip = "This is the directory this task starts in.";

		this.groupContainer = document.createElement( "div" );
		this.groupContainer.className = "task-config-group-container";

		this.groupOpts = document.createElement( "div" );
		this.groupOpts.className = "task-config-group-opts"
		
		this.groupContainer.appendChild( this.groupOpts );
		page2.appendChild( this.groupContainer );
		let c;
		c = popups.makeCheckbox( this.groupOpts, task, "noAutoRun", "No Auto Run" );
		c.tooltip = "Do not automatically run this task when the service manager starts";
		c = popups.makeCheckbox( this.groupOpts, task, "restart", "Restart" );
		c.tooltip = "Automatically restart this task if it stops";
		c = popups.makeCheckbox( this.groupOpts, task, "noKill", "No Auto Kill" );
		c.tooltip = "Do not automatically kill this task when the service manager stops";
		c = popups.makeCheckbox( this.groupOpts, task, "useSignal", "Use Signal" );
		c.tooltip = "Use a signal to stop this task";
		c = popups.makeCheckbox( this.groupOpts, task, "newGroup", "New Group" );
		c.tooltip = "Run this task in a new process group";
		c = popups.makeCheckbox( this.groupOpts, task, "newConsole", "New Console" );
		c.tooltip = "Run this task in a new console";
		c = popups.makeCheckbox( this.groupOpts, task, "useBreak", "Use Break (WIN32)" );
		c.tooltip = "Use a break signal to stop this task";
		c = popups.makeCheckbox( this.groupOpts, task, "noInheritStdio", "No Inherit Standard IO" );
		c.tooltip = "Do not inherit standard IO (stdin,stdout,stderr, handles 0,1,2,...";
		c = popups.makeTextInput( this.groupOpts, task, "style", "Style", false, false, true, "" );
		c.tooltip = "Style of the window (WIN32)"
		
		const moveOpts = document.createElement( "div" );
		moveOpts.className = "task-config-move-opts"
		this.groupContainer.appendChild( moveOpts );
		taskOpts.moveToEnable = !!task.moveTo;
		const checkMove = popups.makeCheckbox( moveOpts, taskOpts, "moveToEnable", "Enable Move" );
		checkMove.tooltip = "Enable moving the window to a specific location";
		const moveSubOpts = document.createElement( "div" );

		if( taskOpts.moveToEnable ) moveSubOpts.style.display = "";
		else 	moveSubOpts.style.display = "none";

		if( !task.moveOpts ) checkMove.checked = false;
		else checkMove.checked = true;
		moveOpts.appendChild( moveSubOpts );
		checkMove.on( "change", (evt)=>{
				if( taskOpts.moveToEnable ) {
					moveSubOpts.style.display = "";
				}else 	moveSubOpts.style.display = "none";
			})		
		c = popups.makeTextInput( moveSubOpts, task.moveTo, "timeout", "Timeout", false, false, true );
		c.tooltip = "Timeout for moving the window (some windows are stubborn)";
		c = popups.makeTextInput( moveSubOpts, task.moveTo, "display", "Display", false, false, true );
		c.tooltip = "Display to move the window to; 0 to ignore (0 for primary display? disable to ignore?)";
		c = popups.makeTextInput( moveSubOpts, task.moveTo, "monitor", "Monitor", false, false, true );
		c.tooltip = "Monitor to move the window to; 0 to ignore";
		const moveOptsXY = document.createElement( "div" );
		moveOptsXY.className = "task-config-move-opts-xy"
		moveSubOpts.appendChild( moveOptsXY );
		c = popups.makeTextInput( moveOptsXY, task.moveTo, "x", "X", false, false, true );
		c.tooltip = "X position to move the window to";
		c = popups.makeTextInput( moveOptsXY, task.moveTo, "y", "Y", false, false, true );
		c.tooltip = "Y position to move the window to";
		const moveOptsWH = document.createElement( "div" );
		moveOptsWH.className = "task-config-move-opts-wh"
		moveSubOpts.appendChild( moveOptsWH );
		c = popups.makeTextInput( moveOptsWH, task.moveTo, "width", "Width", false, false, true );
		c.tooltip = "Width to move the window to";
		c = popups.makeTextInput( moveOptsWH, task.moveTo, "height", "Height", false, false, true );
		c.tooltip = "Height to move the window to";
		

		this.group2 = document.createElement( "div" );
		this.group2.className = "task-config-group2"
		page3.appendChild( this.group2 );
		if( task.args)
			this.argVal = task.args.map( (val,key)=>({arg:val}) )
		this.args = new popups.DataGrid( this.group2, this, "argVal", { noSort: true,
			columns: [
				{field:"arg", name:"Arguments", className: "argument-field", type:{edit:true} },
				{name: "", className: "-arg-up blue", type :{ text:"▲", click:(row)=>{
					this.args.moveRowUp( row );
				}}},
				{name: "", className: "-arg-down blue", type :{text: "▼", click:(row)=>{
					this.args.moveRowDown( row );
				}}},
				{name: "", className: "-arg-delete red", type :{ text:"X", click:(row)=>{
					this.args.deleteRow( row );
				}}},
			],
			onNewRow() {
				// initialize an empty record for this row
				// this is added to the array by the caller of this function
				return {arg:"" };
			}
		} )
		this.args.tooltip = "Click on a field below to edit arguments";
		this.group3 = document.createElement( "div" );
		this.group3.className = "task-config-group3"
		page4.appendChild( this.group3 );
		if( task.env )
			this.envKeys = Object.entries( task.env ).map( ent=>({key:ent[0],val:ent[1]}) );

		this.env = new popups.DataGrid( this.group3, this, "envKeys", { noSort: true,
			columns: [
				{field:"key", name:"Key", className: "env-key", type:{edit:true} },
				{field:"val", name:"Value", className: "env-value", type:{edit:true} },
				{name: "", className: "-env-delete red", type :{ text:"X", click:(row)=>{
					this.env.deleteRow( row );
				}}},
			],
			onNewRow() {
				// initialize an empty record for this row
				// this is added to the array by the caller of this function
				return {key:"",val:"" };
			}
		} )
		
		this.env.tooltip = "Click on a field below to set the environment variables";

		const footer = document.createElement( "div" );
		footer.className = "task-config-footer";
		const prevButton = popups.makeButton( footer, "Prev", ()=>{
			--this.page;
			showPage();
		}, { suffix: "-prev" } )
		const nextButton = popups.makeButton( footer, "Next", ()=>{
			++this.page;
			showPage();
		}, { suffix: "-next" } )
		this.appendChild( footer );

		//this.center();
		const this_ = this;		
		function processForm(create) {
			//Object.assign( task_, task );
			task.args = this_.argVal.map( arg=>arg.arg );
			task.env = {};
			for( let key of this_.envKeys ){
				task.env[key.key] = key.val;
			}
			if( create)
				protocol.createTask( task );
			else protocol.updateTask( taskId, task );
		}
		function showPage() {
			console.log("showPage", this_.page);
			switch( this_.page ) {
				case 0:
					this_.pageFrame.activate( page1);
					prevButton.style.visibility = 'hidden';
					nextButton.style.visibility = 'visible';
					break;
				case 1:
					this_.pageFrame.activate( page2);
					prevButton.style.visibility = 'visible';
					nextButton.style.visibility = 'visible';
					break;
				case 2:
					this_.pageFrame.activate( page3);
					prevButton.style.visibility = 'visible';
					nextButton.style.visibility = 'hidden';
					break;
			}
		}
		showPage();
	};
}
