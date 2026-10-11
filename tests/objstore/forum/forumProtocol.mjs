import {sack} from "sack.vfs";

const JSOX = sack.JSOX;

function safeSend(ws, message) {
	try {
		ws.send( typeof message === "string" ? message : JSOX.stringify(message) );
		return true;
	} catch( error ) {
		return false;
	}
}

function publicUser(user) {
	return {
		id:String( user.UID ?? user.id ?? user.userId ?? user.clientId ),
		name:String( user.name ?? user.displayName ?? "User" ),
	};
}

function requestKey(ws) {
	const url = String( ws.url || "/" );
	return decodeURIComponent( url.split("?")[0].replace(/^\/+/, "") );
}

function guestFromRequest(ws, key) {
	const query = String(ws.url || "").split("?")[1] || "";
	const params = new URLSearchParams(query);
	return {
		UID:key,
		name:(params.get("name") || "Local guest").slice(0, 120),
		guest:true,
	};
}

export class ForumProtocol {
	constructor(database, options = {}) {
		this.database = database;
		this.consumeToken = options.consumeToken || (()=>null);
		this.allowGuests = !!options.allowGuests;
		this.connections = new Set();
	}

	accept(ws) {
		const headers = ws.headers || ws.connection?.headers || {};
		const protocol = headers["Sec-WebSocket-Protocol"]
			|| headers["Sec-Websocket-Protocol"]
			|| headers["sec-websocket-protocol"];
		return String(protocol || "").split(",").map(value=>value.trim()).includes("sack.forum");
	}

	connect(ws) {
		const key = requestKey(ws);
		let user = key ? this.consumeToken(key) : null;
		if( !user && this.allowGuests && key.startsWith("guest-") )
			user = guestFromRequest( ws, key );
		if( !user ) {
			ws.close( 4001, "Login required" );
			return;
		}

		const connection = {ws, user, subscriptions:new Set()};
		this.connections.add( connection );
		ws.on( "message", raw=>this.#message(connection, raw) );
		ws.on( "close", ()=>this.connections.delete(connection) );
		safeSend( ws, {op:"ready", user:publicUser(user)} );
	}

	async #message(connection, raw) {
		let message;
		try {
			message = JSOX.parse(raw);
		} catch( error ) {
			safeSend( connection.ws, {op:"error", error:"Invalid JSOX message."} );
			return;
		}
		const rid = message.rid;
		try {
			const data = await this.#dispatch( connection, message );
			if( rid !== undefined ) safeSend( connection.ws, {op:"result", rid, data} );
		} catch( error ) {
			safeSend( connection.ws, {op:"error", rid, error:error?.message || String(error)} );
		}
	}

	async #dispatch(connection, message) {
		const db = this.database;
		switch( message.op ) {
		case "bootstrap":
			return {user:publicUser(connection.user), groups:await db.listGroups(null, 0, message.limit)};
		case "listGroups":
			return db.listGroups( message.parentId, message.offset, message.limit );
		case "createGroup": {
			const group = await db.createGroup( connection.user, message );
			this.#broadcast( {type:"group", groupId:group.id, parentId:group.parentId} );
			return group;
		}
		case "listTopics":
			return db.listTopics( message.groupId, message.offset, message.limit );
		case "createTopic": {
			const result = await db.createTopic( connection.user, message );
			this.#broadcast( {type:"topic", groupId:result.topic.groupId, topicId:result.topic.id} );
			return result;
		}
		case "readTopic":
			return db.readTopic( message.topicId, message.offset, message.limit, connection.user );
		case "reply": {
			const post = await db.reply( connection.user, message );
			const topic = await db.getTopicRecord( post.topicId );
			this.#broadcast( {type:"post", groupId:topic.groupId, topicId:post.topicId, postId:post.id} );
			return post;
		}
		case "editPost": {
			const post = await db.editPost( connection.user, message );
			const topic = await db.getTopicRecord( post.topicId );
			this.#broadcast( {type:"edit", groupId:topic.groupId, topicId:post.topicId, postId:post.id} );
			return post;
		}
		case "subscribe":
			connection.subscriptions = new Set( (message.ids || []).map(String) );
			return {ok:true};
		case "ping":
			return {now:new Date()};
		default:
			throw new Error( `Unsupported operation: ${message.op}` );
		}
	}

	#broadcast(change) {
		const message = {op:"changed", ...change};
		for( const connection of this.connections ) {
			if( !connection.subscriptions.size
				|| connection.subscriptions.has(String(change.groupId))
				|| connection.subscriptions.has(String(change.topicId)) )
				safeSend( connection.ws, message );
		}
	}
}
