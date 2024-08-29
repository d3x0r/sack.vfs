
import {sack} from "sack.vfs"

export const local = {
	tasks : [],
	taskMap : {},
	replyMap : {},
	connections: [],
	systems : [],
	upstreamWS : null,
	id : sack.Id(),
	logRelays : {},
	addTask : null,
}
