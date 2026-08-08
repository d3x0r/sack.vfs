
import {Popup, popups} from "/node_modules/@d3x0r/popups2/popups.js"
import "/node_modules/@d3x0r/popups2/controls/button.js"
import "/node_modules/@d3x0r/popups2/controls/text-field.js"

import {protocol} from "./protocol.js"

//import {constants,wait} from "/common/system/constants.loader.js"
//import {config as siteConfig} from "./config.js"

//await wait;
let wantTouchButton = false;



export class TaskConfiguration extends Popup {
	taskInfo = {
		name:"Unset",
	}
	taskTitle = "Unset";
	current_task = null;
	current_monitor = null;
	current_select = null;
	displays = null;
	montiorFrame = null;
	monitors = []; // controls for monitors... 
	constructor(parent,task) {
		super( "Task Settings",parent, {suffix:"-settings", modal:true} );
		this.useMouse = false;
		this.divFrame.clssName = "task-config-frame"
		this.taskInfo = task;
		
		popups.makeTextField( this, this.taskInfo, "name", "Task Name" );
		popups.makeTextField( this, this.taskInfo.info, "title", "Window Title" );

		const monitorFrame = this.monitorFrame = document.createElement( "div");
		monitorFrame.className = "monitors-frame";
		this.appendChild( monitorFrame );
		const button = popups.makeButton( this, "Done", ()=>{

			this.hide();
		} );
		button.button.className = "button monitor-confirm";
		const monitorTitle = document.createElement( "span");
		monitorTitle.className = "monitors-frame-title";
		monitorTitle.textContent = "Select Monitor"
		monitorFrame.appendChild( monitorTitle );
		this.update(task);
	}

	updateDisplays() {
		for( let monitor of this.monitors ) monitor.remove();

		protocol.getDisplays().then( displays=>{
			let top = 0;
			let left = 0;
			let bottom = 0;
			let right = 0;
			for( let monitor of displays.monitor ) {
				if( monitor.x < left ) left = monitor.x;
				if( monitor.y < top ) top = monitor.y;
				if( (monitor.x+monitor.width) > right ) right = (monitor.x+monitor.width);
				if( (monitor.y+monitor.height) > bottom ) bottom = (monitor.y+monitor.height);
			}	

			this.displays = displays;
			let monitor_number = 1;
			for( let monitor of displays.monitor ){
				const monitor_frame = document.createElement( "div" );
				monitor_frame.className = "monitor-frame"
				monitor_frame.style.position = "absolute";
				monitor_frame.style.width = (monitor.width/(right-left)*100)+"%";
				monitor_frame.style.height = (monitor.height/(bottom-top)*100)+"%";
				monitor_frame.style.left = ((monitor.x-left) / (right-left)*100)+"%";
				monitor_frame.style.top = ((monitor.y-top) / (bottom-top)*100)+"%";

				const monitor_child = document.createElement( "div" );
				monitor_child.className = "monitor-child"
				monitor_frame.appendChild( monitor_child );
				if( "moveTo" in this.taskInfo ) {
					if( "monitor" in this.taskInfo.moveTo ) {
						if( this.taskInfo.moveTo.monitor === monitor_number ) {
							this.current_monitor = monitor;
							this.current_select = monitor_child;
							monitor_child.classList.add( "green-border" );
						}
					}
					else if( "display" in this.taskInfo.moveTo ) {
						if( this.taskInfo.moveTo.display === monitor.display ) {
							this.current_monitor = monitor;
							this.current_select = monitor_child;
							monitor_child.classList.add( "green-border" );
						}
					}
					else if( this.taskInfo.moveTo.x === monitor.x 
							&& this.taskInfo.moveTo.y === monitor.y
							&& this.taskInfo.moveTo.width === monitor.width 
							&& this.taskInfo.moveTo.height === monitor.height) {
						this.current_monitor = monitor;
						this.current_select = monitor_child;
						monitor_child.classList.add( "green-border" );
					}
				}

				const monitor_info1 = document.createElement( "span" );
				monitor_info1.className = "monitor-info"
				monitor_child.appendChild( monitor_info1 );
				if( monitor.width != monitor.device.width )
					monitor_info1.textContent = monitor_number +" : " 
							+ monitor.width+"⨯" + monitor.height 
							+ "("+monitor.device.width+"⨯"+monitor.device.height+")";	
				else
					monitor_info1.textContent = monitor_number +" : " + monitor.width+"⨯" + monitor.height;	

				const monitor_info2 = document.createElement( "span" );
				monitor_info2.className = "monitor-status"
				monitor_child.appendChild( monitor_info2 );
				monitor_info2.textContent = monitor.device.monitorName + " " + (monitor.device.primary?"(Primary)":"");
				this.monitorFrame.appendChild( monitor_frame );

				monitor_frame.addEventListener( "click", ((frame,monitor,number)=>((evt)=>{
					if( monitor !== this.current_monitor ) {
						if( this.current_select )
							this.current_select.classList.remove( "green-border" );
						this.current_monitor = monitor;
						this.current_select = frame;
						this.current_select.classList.add( "green-border" );
						protocol.setTaskDisplay( this.current_task, monitor.display );
					}
				}))(monitor_child, monitor,monitor_number));

				monitor_number++;

			}
		});

	}

	async update(taskRow) {
		this.current_task = taskRow;
		protocol.getTaskInfo(taskRow.id).then( info=>{
			this.taskInfo = info;
			this.taskTitle = info.title;
			this.updateDisplays();
			this.refresh();
			this.show();
		});
	}
}



