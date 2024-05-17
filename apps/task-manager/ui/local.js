export const local = {
	tasks : {},
	systemMap : {},
	ws : null,
	logs : {},
	taskData : null,
	display : null,
	statusDisplay : null,
	statusTimer: 0,
	systems: [],
	refresh : null,
	taskInfo: {},
	activePage : null, // which page is currently active to default to main
	pageFrame : null, // page control for systems.
	firstPage : null, // the first status has a page itself...
	reset() {
		if( local.statusDisplay )
			local.statusDisplay.remove();
		const tasks = Object.keys( local.tasks );
		local.pageFrame.activate( local.firstPage	);
		for( let task of tasks ) {
			if( task in local.logs )
				local.logs[task].logFrame.remove();
			delete local.logs[task];
			delete local.tasks[task];
		}
		for( let system of local.systems )  {
			//system.dataGrid.remove();
			system.page.remove();
		}
		local.systems.length = 0;

	}
};