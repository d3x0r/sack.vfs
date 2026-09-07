import {Popup,popups} from "/node_modules/@d3x0r/popups2/popups.js"
import {Button} from "/node_modules/@d3x0r/popups2/controls/button.js"
import {DataGrid} from "/node_modules/@d3x0r/popups2/controls/data-grid.js"
import {local} from "./local.js"
import {protocol} from "./protocol.js"

/*
  Editor for a service manager's plugin list (`extraModules` in its config).

  The list is ordered and loaded in sequence at start-up: an entry with a
  Function waits for that export to finish before the next one begins; an entry
  without one is imported for its side effects and the list moves straight on.
  Nothing here takes effect until that system is next started - the modules have
  already been loaded by the time there is a UI to edit them from.
*/
export class PluginsEditor extends Popup {

	rows = [];

	constructor( group, plugins ) {
		super( "Plugins", document.body, { suffix:"-plugins", shadowFrame: true, enableClose: true } );
		this.on( "captionClose", ()=>{
			this.on( "close", true );
			this.remove();
		} );
		this.hide();
		local.dialogs.add( this );

		const styles = [ popups.utils.addStyleSheetSrc( this, "/css/styles.css" )
		               , popups.utils.addStyleSheetSrc( this, "/css/task-config.css" ) ];
		Promise.all( styles ).then( ()=>{
			this.show();
			this.center();
		} );

		const header = document.createElement( "div" );
		header.className = "task-config-header";
		const titleBox = document.createElement( "div" );
		titleBox.className = "task-config-header-title";
		const title = document.createElement( "div" );
		title.className = "task-config-header-title-text";
		title.textContent = ( local.systems.indexOf( group ) >= 0 ) ? group.system : "This Service Manager";
		titleBox.appendChild( title );
		const buttons = document.createElement( "div" );
		buttons.className = "task-config-buttons";
		header.appendChild( titleBox );
		header.appendChild( buttons );
		this.appendChild( header );

		this.save = new Button( buttons, "Save", ()=>{
			protocol.setPlugins( group, this.rows.map( row=>{
				const plugin = { name: row.name };
				// an empty Function column means "just import it"; storing "" would
				// send the loader looking for an export named "".
				if( row.function ) plugin.function = row.function;
				return plugin;
			} ) );
			this.on( "close", true );
			this.remove();
		} );
		this.save.tooltip = "Store this list; it loads the next time this service manager starts";

		const note = document.createElement( "div" );
		note.className = "plugins-note";
		note.textContent = "Loaded in order at start-up. An entry with a Function waits for it to"
		                 + " finish before the next entry runs; without one the module is only"
		                 + " imported. Changes apply on the next start.";
		this.appendChild( note );

		const gridFrame = document.createElement( "div" );
		gridFrame.className = "plugins-grid";
		this.appendChild( gridFrame );

		this.rows = ( plugins || [] ).map( plugin=>({ name: plugin.name || ""
		                                            , function: plugin.function || "" }) );

		this.grid = new DataGrid( gridFrame, this, "rows", { noSort: true,
			columns: [
				{ field:"name", name:"Module", className: "plugin-name", type:{edit:true} },
				{ field:"function", name:"Function", className: "plugin-function", type:{edit:true} },
				{ name: "", className: "-plugin-up", type:{ suffix:" blue", text:"▲", click:(gridRow)=>{
					this.grid.moveRowUp( gridRow.rowData );
				} } },
				{ name: "", className: "-plugin-down", type:{ suffix:" blue", text:"▼", click:(gridRow)=>{
					this.grid.moveRowDown( gridRow.rowData );
				} } },
				{ name: "", className: "-plugin-delete", type:{ suffix:" red", text:"X", click:(gridRow)=>{
					const row = gridRow.rowData;
					const at = this.rows.findIndex( test=>test === row );
					if( at >= 0 ) this.rows.splice( at, 1 );
					this.grid.deleteRow( row );
				} } },
			],
			onNewRow() {
				return { name:"", function:"" };
			}
		} );
		this.grid.tooltip = "Click a field to edit; the module path is relative to the service manager's working directory";
	}

	remove() {
		local.dialogs.delete( this );
		super.remove();
	}
}
