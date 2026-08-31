import {Popup,popups} from "/node_modules/@d3x0r/popups2/popups.js"
import {PagedFrame} from "/node_modules/@d3x0r/popups2/controls/paged-frame.js"
import {TextInput} from "/node_modules/@d3x0r/popups2/controls/text-input.js"
import {Button} from "/node_modules/@d3x0r/popups2/controls/button.js"
import {DataGrid} from "/node_modules/@d3x0r/popups2/controls/data-grid.js"
import {Checkbox} from "/node_modules/@d3x0r/popups2/controls/checkbox.js"
import {ChoiceInput} from "/node_modules/@d3x0r/popups2/controls/choice-input.js"
import {createSimpleForm} from "/node_modules/@d3x0r/popups2/forms/simple-form.js"
import {local} from "./local.js"
import {config as protocolConfig, protocol} from "./protocol.js"

export class TaskInfoEditor extends Popup {
	
	argVal = [];
	envKeys = [];
	page = 0;
	// `group` is the task list this editor was opened from - `local` for this
	// service manager, or a System for one of the upstreamed ones.  Without it
	// a create or an update always landed on the local system.
	constructor( taskId, task_, group ) {
		super( "Task Config Editor", document.body, { suffix:"-task-config", shadowFrame: true, enableClose: true });
		// so a dropped connection can close this - the task id it is editing does
		// not survive the reconnect.
		local.dialogs.add( this );
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
			this.positionEditor();
		})
		const task = Object.assign( {}, task_ );
		// hidden/minimized/maximized are mutually exclusive on the launch side (they all end up
		// as a single wShowWindow value), so the form drives them from one selection.
		const showStates = [ { text:"Normal",    value:"normal" }
		                   , { text:"Minimized", value:"minimized" }
		                   , { text:"Maximized", value:"maximized" }
		                   , { text:"Hidden",    value:"hidden" }
		                   ];
		function getShowState( t ) {
			if( t.hidden ) return "hidden";
			if( t.minimized ) return "minimized";
			if( t.maximized ) return "maximized";
			return "normal";
		}
		function setShowState( t, state ) {
			// always write all three; task.update() only copies keys that are present, so a
			// missing key would leave the previous state set in the stored config.
			t.hidden    = state === "hidden";
			t.minimized = state === "minimized";
			t.maximized = state === "maximized";
		}
		const taskOpts = {
			moveToEnable : false,
			showState : getShowState( task ),
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

		this.pageFrame =new PagedFrame( flexFrame );
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
			this.create = new Button( buttonFrame, "Save", ()=>{
				processForm(false);
						this.on( "close", true );
				this.remove();
			} )
			this.create.tooltip = "Update this current task with the new settings";
		}

		// The Name field on the form is the name now, so there is normally
		// nothing left to ask for - only fall back to the prompt when the name
		// would still collide with the task this editor opened on, or when a
		// new task hasn't been given one.  A new task has nothing to save "as",
		// so its button is just Save.
		const originalName = task_ ? task_.name : null;
		this.saveAs = new Button( buttonFrame, task_ ? "Save As" : "Save", ()=>{
			if( task.name && task.name !== originalName ) {
				processForm(true);
				this.on( "close", true );
				this.remove();
				return;
			}
			// getter, not a value: the Name field above may have been edited
			// since this editor opened.
			const form = createSimpleForm( "Save As", "Name", ()=>task.name, (name)=>{
				task.name = name;
				processForm(true);
				this.on( "close", true );
				form.remove();
				this.remove();
			} );
			form.show();
		} )
		this.saveAs.tooltip = task_ ? "Save these settings as a new task, under the name above"
		                            : "Create this task with these settings";

		if( !task ) {
			return;
		}
		this.group1 = document.createElement( "div" );
		this.group1.className = "task-config-group1"
		page1.appendChild( this.group1 );
		// the name is what the task list shows and what loadTask() matches on,
		// so editing it here and saving is a rename; without this field the only
		// way to change a name was Save As, which leaves the original behind.
		this.taskName = new TextInput( this.group1, task, "name", "Name" );
		this.taskName.tooltip = "Name of the task; changing this renames the task";
		this.taskName.on( "change", ()=>{
			headerTitle.textContent = task.name || "New Task";
		} );
		this.bin = new TextInput( this.group1, task, "bin", 'Program');
		this.bin.tooltip = "Program to run for this task";
		this.altbin = new TextInput( this.group1, task, "altbin", 'Alternate Program');
		this.altbin.tooltip = "Alternate Program to run for this task (if bin fails to run)";
		this.work = new TextInput( this.group1, task, "work", "Start In Path" );
		this.work.tooltip = "This is the directory this task starts in.";
		this.programName = new TextInput( this.group1, task, "programName", "Program Name", false, false, false, "" );
		this.programName.tooltip = "Program name used for 'use signal'";

		this.groupContainer = document.createElement( "div" );
		this.groupContainer.className = "task-config-group-container";

		this.groupOpts = document.createElement( "div" );
		this.groupOpts.className = "task-config-group-opts"
		
		this.groupContainer.appendChild( this.groupOpts );
		page2.appendChild( this.groupContainer );
		let c;
		c = new Checkbox( this.groupOpts, task, "noAutoRun", "No Auto Run" );
		c.tooltip = "Do not automatically run this task when the service manager starts";
		c = new Checkbox( this.groupOpts, task, "restart", "Restart" );
		c.tooltip = "Automatically restart this task if it stops";
		c = new Checkbox( this.groupOpts, task, "noKill", "No Auto Kill" );
		c.tooltip = "Do not automatically kill this task when the service manager stops";
		c = new Checkbox( this.groupOpts, task, "useSignal", "Use Signal" );
		c.tooltip = "Use a signal to stop this task";
		c = new Checkbox( this.groupOpts, task, "newGroup", "New Group" );
		c.tooltip = "Run this task in a new process group";
		c = new Checkbox( this.groupOpts, task, "newConsole", "New Console" );
		c.tooltip = "Run this task in a new console";
		c = new Checkbox( this.groupOpts, task, "usePty", "Use PTY (WIN32)" );
		c.tooltip = "Run this task through a pseudo terminal instead of plain pipes";
		c = new Checkbox( this.groupOpts, task, "useBreak", "Use Break (WIN32)" );
		c.tooltip = "Use a break signal to stop this task";
		c = new Checkbox( this.groupOpts, task, "noInheritStdio", "Own Standard IO" );
		c.tooltip = "The task keeps its own stdin/stdout/stderr - required for 'New Console' to show anything, but its output will not appear in Show Log";
		c = new ChoiceInput( this.groupOpts, taskOpts, "showState", showStates, "Window State"
		                   , { change: ()=>setShowState( task, taskOpts.showState ) } );
		// ChoiceInput has no tooltip setter; match what the other controls render.
		{
			const tip = document.createElement( "span" );
			tip.className = "tooltip-text";
			tip.textContent = "How the task's window is shown when it starts (WIN32)";
			c.el.appendChild( tip );
			c.el.classList.add( "has-tooltip" );
		}
		c = new TextInput( this.groupOpts, task, "style", "Style", false, false, true, "" );
		c.tooltip = "Style of the window (WIN32)"
		
		const moveOpts = document.createElement( "div" );
		moveOpts.className = "task-config-move-opts"
		this.groupContainer.appendChild( moveOpts );
		taskOpts.moveToEnable = !!task.moveTo;
		const checkMove = new Checkbox( moveOpts, taskOpts, "moveToEnable", "Enable Move" );
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
		c = new TextInput( moveSubOpts, task.moveTo, "timeout", "Timeout", false, false, true );
		c.tooltip = "Timeout for moving the window (some windows are stubborn)";
		c = new TextInput( moveSubOpts, task.moveTo, "display", "Display", false, false, true );
		c.tooltip = "Display to move the window to; 0 to ignore (0 for primary display? disable to ignore?)";
		c = new TextInput( moveSubOpts, task.moveTo, "monitor", "Monitor", false, false, true );
		c.tooltip = "Monitor to move the window to; 0 to ignore";
		const moveOptsXY = document.createElement( "div" );
		moveOptsXY.className = "task-config-move-opts-xy"
		moveSubOpts.appendChild( moveOptsXY );
		c = new TextInput( moveOptsXY, task.moveTo, "x", "X", false, false, true );
		c.tooltip = "X position to move the window to";
		c = new TextInput( moveOptsXY, task.moveTo, "y", "Y", false, false, true );
		c.tooltip = "Y position to move the window to";
		const moveOptsWH = document.createElement( "div" );
		moveOptsWH.className = "task-config-move-opts-wh"
		moveSubOpts.appendChild( moveOptsWH );
		c = new TextInput( moveOptsWH, task.moveTo, "width", "Width", false, false, true );
		c.tooltip = "Width to move the window to";
		c = new TextInput( moveOptsWH, task.moveTo, "height", "Height", false, false, true );
		c.tooltip = "Height to move the window to";
		

		this.group2 = document.createElement( "div" );
		this.group2.className = "task-config-group2"
		page3.appendChild( this.group2 );
		if( task.args)
			this.argVal = task.args.map( (val,key)=>({arg:val}) )
		this.args = new DataGrid( this.group2, this, "argVal", { noSort: true,
			columns: [
				{field:"arg", name:"Arguments", className: "argument-field", type:{edit:true} },
				{name: "", className: "-arg-up", type :{ suffix:" blue", text:"▲", click:(gridRow)=>{
					const row = gridRow.rowData;
					this.args.moveRowUp( row );
				}}},
				{name: "", className: "-arg-down", type :{suffix:" blue", text: "▼", click:(gridRow)=>{
					const row = gridRow.rowData;
					this.args.moveRowDown( row );
				}}},
				{name: "", className: "-arg-delete", type :{ suffix:" red", text:"X", click:(gridRow)=>{
					const row = gridRow.rowData;
					const arg = this.argVal.findIndex( (arg)=>arg===row );
					if( arg >= 0 ) this.argVal.splice( arg, 1 );
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

		this.env = new DataGrid( this.group3, this, "envKeys", { noSort: true,
			columns: [
				{field:"key", name:"Key", className: "env-key", type:{edit:true} },
				{field:"val", name:"Value", className: "env-value", type:{edit:true} },
				{name: "", className: "-env-delete", type :{suffix:" red",  text:"X", click:(gridRow)=>{
					const row = gridRow.rowData;
					const envKey = this.envKeys.findIndex( (key)=>key===row );
					if( envKey >= 0 ) this.envKeys.splice( envKey, 1 );
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
		const prevButton = new Button( footer, "Prev", ()=>{
			--this.page;
			showPage();
		}, { suffix: "-prev" } )
		const nextButton = new Button( footer, "Next", ()=>{
			++this.page;
			showPage();
		}, { suffix: "-next" } )
		this.appendChild( footer );

		//this.center();
		const this_ = this;		
		function processForm(create) {
			//Object.assign( task_, task );
			task.args = this_.argVal.map( arg=>arg.arg );
			setShowState( task, taskOpts.showState );
			task.env = {};
			for( let key of this_.envKeys ){
				task.env[key.key] = key.val;
			}
			if( create)
				protocol.createTask( group, task );
			else protocol.updateTask( group, taskId, task );
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
					nextButton.style.visibility = 'visible';
					break;
				case 3:
					this_.pageFrame.activate( page4);
					prevButton.style.visibility = 'visible';
					nextButton.style.visibility = 'hidden';
					break;
			}
		}
		showPage();
	};

	remove() {
		local.dialogs.delete( this );
		super.remove();
	}

	positionEditor() {
		const frame = this.divShadow || this.divFrame;
		const rect = this.divFrame.getBoundingClientRect();
		const left = Math.max( 16, Math.round( ( window.innerWidth - rect.width ) / 2 ) );
		frame.style.left = left + "px";
		frame.style.top = "96px";
	}
}
