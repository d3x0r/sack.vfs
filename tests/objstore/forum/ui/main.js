import {Popup, popups} from "/node_modules/@d3x0r/popups2/bundles/all.js";
import {JSOX} from "/node_modules/jsox/lib/jsox.mjs";
import {renderMarkdown} from "./markdown.js";

const GROUP_PAGE = 25;
const POST_PAGE = 50;

function element(tag, className, text) {
	const node = document.createElement(tag);
	if( className ) node.className = className;
	if( text !== undefined ) node.textContent = text;
	return node;
}

function clear(node) {
	node.replaceChildren();
	return node;
}

function displayDate(value) {
	return new Date(value).toLocaleString( undefined, {dateStyle:"medium", timeStyle:"short"} );
}

function button(parent, label, callback, className = "") {
	const control = popups.makeButton( parent, label, ()=>Promise.resolve(callback()).catch(showError) );
	const node = control.control || control.button || control.el;
	if( className ) node.classList.add(className);
	return node;
}

function showError(error) {
	console.error(error);
	popups.Alert( error?.message || String(error) );
}

class ForumClient {
	constructor() {
		this.ws = null;
		this.user = null;
		this.sequence = 1;
		this.pending = new Map();
		this.listeners = new Set();
	}

	connect(key, guestName = null) {
		return new Promise( (resolve, reject)=>{
			const scheme = location.protocol === "https:" ? "wss:" : "ws:";
			const query = guestName ? `?name=${encodeURIComponent(guestName)}` : "";
			const ws = this.ws = new WebSocket( `${scheme}//${location.host}/${encodeURIComponent(key)}${query}`, "sack.forum" );
			let connected = false;
			ws.onmessage = event=>{
				let message;
				try { message = JSOX.parse(event.data); }
				catch( error ) { showError(error); return; }
				if( message.op === "ready" ) {
					connected = true;
					this.user = message.user;
					resolve(message.user);
				} else if( message.op === "result" || message.op === "error" ) {
					const request = this.pending.get(message.rid);
					if( !request ) return;
					this.pending.delete(message.rid);
					if( message.op === "error" ) request.reject( new Error(message.error) );
					else request.resolve(message.data);
				} else if( message.op === "changed" ) {
					for( const listener of this.listeners ) listener(message);
				}
			};
			ws.onerror = ()=>{ if( !connected ) reject( new Error("Could not connect to the forum service.") ); };
			ws.onclose = event=>{
				if( !connected ) reject( new Error(event.reason || "The forum rejected this login.") );
				for( const request of this.pending.values() ) request.reject( new Error("Forum connection closed.") );
				this.pending.clear();
			};
		} );
	}

	request(op, fields = {}) {
		if( !this.ws || this.ws.readyState !== WebSocket.OPEN )
			return Promise.reject( new Error("The forum is not connected.") );
		const rid = this.sequence++;
		return new Promise( (resolve, reject)=>{
			this.pending.set( rid, {resolve, reject} );
			this.ws.send( JSOX.stringify({op, rid, ...fields}) );
		} );
	}

	onChange(listener) {
		this.listeners.add(listener);
		return ()=>this.listeners.delete(listener);
	}
}

class ForumApplication {
	constructor(client) {
		this.client = client;
		this.currentGroup = null;
		this.currentTopic = null;
		this.groupOffset = 0;
		this.postOffset = 0;
		this.refreshTimer = null;

		const mount = clear( document.getElementById("game") );
		this.root = new Popup( null, null, {from:mount} );
		this.shell = this.root.appendChild( element("div", "forum-shell") );
		this.header = this.shell.appendChild( element("header", "forum-header") );
		const brand = this.header.appendChild( element("div", "forum-brand") );
		brand.appendChild( element("span", "forum-mark", "S") );
		const brandText = brand.appendChild( element("div") );
		brandText.appendChild( element("strong", "", "Sack Forum") );
		brandText.appendChild( element("small", "", "Object graph discussions") );
		this.identity = this.header.appendChild( element("div", "forum-identity", "Connecting…") );

		const main = this.shell.appendChild( element("div", "forum-main") );
		this.sidebar = main.appendChild( element("aside", "forum-sidebar") );
		const sidebarHead = this.sidebar.appendChild( element("div", "sidebar-heading") );
		sidebarHead.appendChild( element("span", "", "Groups") );
		button( sidebarHead, "+", ()=>this.composeGroup(null), "compact" ).title = "New top-level group";
		this.tree = this.sidebar.appendChild( element("div", "group-tree") );
		this.content = main.appendChild( element("section", "forum-content") );
		this.status = this.shell.appendChild( element("footer", "forum-status", "Waiting for login") );
		this.client.onChange( change=>this.changed(change) );
		this.welcome();
	}

	welcome() {
		clear(this.content);
		const panel = this.content.appendChild( element("div", "welcome-panel") );
		panel.appendChild( element("p", "eyebrow", "GRAPH-BACKED CONVERSATIONS") );
		panel.appendChild( element("h1", "", "Choose a group and start a thread." ) );
		panel.appendChild( element("p", "lede", "Groups can nest to any depth. Messages are stored in paged slabs and rendered as safe Markdown with KaTeX math." ) );
	}

	async initialize() {
		this.identity.textContent = this.client.user.name;
		this.status.textContent = "Connected";
		const bootstrap = await this.client.request( "bootstrap", {limit:GROUP_PAGE} );
		this.renderTree( bootstrap.groups.items );
	}

	renderTree(groups) {
		clear(this.tree);
		if( !groups.length ) this.tree.appendChild( element("p", "empty-note", "No groups yet.") );
		for( const group of groups ) this.tree.appendChild( this.groupNode(group) );
	}

	groupNode(group) {
		const wrapper = element("div", "tree-group");
		const row = wrapper.appendChild( element("div", "tree-row") );
		const toggle = row.appendChild( element("button", "tree-toggle", group.childCount ? "›" : "·") );
		toggle.disabled = !group.childCount;
		const label = row.appendChild( element("button", "tree-label", group.name) );
		label.onclick = ()=>this.openGroup(group.id).catch(showError);
		if( group.childCount ) toggle.onclick = async ()=>{
			const existing = wrapper.querySelector(":scope > .tree-children");
			if( existing ) { existing.remove(); toggle.textContent = "›"; return; }
			const children = wrapper.appendChild( element("div", "tree-children") );
			toggle.textContent = "⌄";
			const data = await this.client.request( "listGroups", {parentId:group.id, limit:100} );
			for( const child of data.items ) children.appendChild( this.groupNode(child) );
		};
		return wrapper;
	}

	async refreshTree() {
		const roots = await this.client.request( "listGroups", {limit:GROUP_PAGE} );
		this.renderTree(roots.items);
	}

	async openGroup(groupId, offset = 0) {
		this.currentTopic = null;
		this.postOffset = 0;
		this.groupOffset = Math.max(0, offset);
		const [topics, children] = await Promise.all([
			this.client.request( "listTopics", {groupId, offset:this.groupOffset, limit:GROUP_PAGE} ),
			this.client.request( "listGroups", {parentId:groupId, limit:100} ),
		]);
		this.currentGroup = topics.group;
		await this.client.request( "subscribe", {ids:[groupId]} );
		this.renderGroup(topics, children.items);
	}

	breadcrumbs(path, topic = null) {
		const crumbs = element("nav", "breadcrumbs");
		const home = crumbs.appendChild( element("button", "", "Forum") );
		home.onclick = ()=>{ this.currentGroup = this.currentTopic = null; this.welcome(); };
		for( const group of path ) {
			crumbs.appendChild( element("span", "", "/") );
			const link = crumbs.appendChild( element("button", "", group.name) );
			link.onclick = ()=>this.openGroup(group.id).catch(showError);
		}
		if( topic ) {
			crumbs.appendChild( element("span", "", "/") );
			crumbs.appendChild( element("strong", "", topic.subject) );
		}
		return crumbs;
	}

	renderGroup(data, children) {
		clear(this.content);
		this.content.appendChild( this.breadcrumbs(data.path) );
		const titlebar = this.content.appendChild( element("div", "content-titlebar") );
		const title = titlebar.appendChild( element("div") );
		title.appendChild( element("p", "eyebrow", `${data.total} topic${data.total === 1 ? "" : "s"}`) );
		title.appendChild( element("h1", "", data.group.name) );
		if( data.group.description ) title.appendChild( element("p", "lede", data.group.description) );
		const actions = titlebar.appendChild( element("div", "title-actions") );
		button( actions, "New subgroup", ()=>this.composeGroup(data.group.id), "secondary" );
		button( actions, "New topic", ()=>this.composeTopic(data.group.id), "primary" );

		if( children.length ) {
			this.content.appendChild( element("h2", "section-title", "Subgroups") );
			const grid = this.content.appendChild( element("div", "group-cards") );
			for( const group of children ) {
				const card = grid.appendChild( element("button", "group-card") );
				card.appendChild( element("strong", "", group.name) );
				card.appendChild( element("span", "", group.description || "No description") );
				card.appendChild( element("small", "", `${group.childCount} subgroups · ${group.topicCount} topics`) );
				card.onclick = ()=>this.openGroup(group.id).catch(showError);
			}
		}

		this.content.appendChild( element("h2", "section-title", "Topics") );
		const list = this.content.appendChild( element("div", "topic-list") );
		if( !data.items.length ) list.appendChild( element("p", "empty-note", "No topics in this group yet.") );
		for( const topic of data.items ) {
			const row = list.appendChild( element("button", "topic-row") );
			const copy = row.appendChild( element("span", "topic-copy") );
			copy.appendChild( element("strong", "", topic.subject) );
			copy.appendChild( element("small", "", `Started by ${topic.createdBy.name} · updated ${displayDate(topic.updated)}`) );
			row.appendChild( element("span", "topic-count", `${topic.postCount}\nposts`) );
			row.onclick = ()=>this.openTopic(topic.id).catch(showError);
		}
		this.pager( data.offset, data.limit, data.total, next=>this.openGroup(data.group.id, next) );
	}

	async openTopic(topicId, offset = 0) {
		this.postOffset = Math.max(0, offset);
		const data = await this.client.request( "readTopic", {topicId, offset:this.postOffset, limit:POST_PAGE} );
		this.currentTopic = data.topic;
		this.currentGroup = data.group;
		await this.client.request( "subscribe", {ids:[data.group.id, topicId]} );
		this.renderTopic(data);
	}

	renderTopic(data) {
		clear(this.content);
		this.content.appendChild( this.breadcrumbs(data.path, data.topic) );
		const titlebar = this.content.appendChild( element("div", "content-titlebar") );
		const title = titlebar.appendChild( element("div") );
		title.appendChild( element("p", "eyebrow", `${data.total} message${data.total === 1 ? "" : "s"}`) );
		title.appendChild( element("h1", "", data.topic.subject) );
		button( titlebar, "Reply", ()=>this.composeReply(data.topic.id), "primary" );
		const messages = this.content.appendChild( element("div", "message-list") );
		for( const post of data.items ) messages.appendChild( this.postCard(post) );
		this.pager( data.offset, data.limit, data.total, next=>this.openTopic(data.topic.id, next) );
	}

	postCard(post) {
		const card = element("article", "message-card");
		card.id = `post-${post.id}`;
		const head = card.appendChild( element("header", "message-head") );
		const author = head.appendChild( element("div", "message-author") );
		author.appendChild( element("span", "avatar", post.author.name.slice(0, 1).toUpperCase()) );
		const detail = author.appendChild( element("div") );
		detail.appendChild( element("strong", "", post.author.name) );
		detail.appendChild( element("small", "", `${displayDate(post.created)}${post.edited ? ` · edited ${displayDate(post.edited)}` : ""}`) );
		const actions = head.appendChild( element("div", "message-actions") );
		if( post.parentId ) {
			const parent = actions.appendChild( element("button", "text-action", "↩ parent") );
			parent.onclick = ()=>document.getElementById(`post-${post.parentId}`)?.scrollIntoView({behavior:"smooth"});
		}
		button( actions, "Reply", ()=>this.composeReply(post.topicId, post.id), "compact" );
		if( post.canEdit ) button( actions, "Edit", ()=>this.composeEdit(post), "compact" );
		const body = card.appendChild( element("div", "markdown-body") );
		renderMarkdown(body, post.markdown);
		return card;
	}

	pager(offset, limit, total, navigate) {
		if( total <= limit ) return;
		const pager = this.content.appendChild( element("div", "pager") );
		const previous = button( pager, "Previous", ()=>navigate(Math.max(0, offset - limit)), "secondary" );
		previous.disabled = offset === 0;
		pager.appendChild( element("span", "", `${offset + 1}–${Math.min(total, offset + limit)} of ${total}`) );
		const next = button( pager, "Next", ()=>navigate(offset + limit), "secondary" );
		next.disabled = offset + limit >= total;
	}

	composeGroup(parentId) {
		this.editor({title:parentId ? "New subgroup" : "New top-level group", subjectLabel:"Group name", description:true},
			async values=>{
				const group = await this.client.request( "createGroup", {parentId, name:values.subject, description:values.markdown} );
				await this.refreshTree();
				await this.openGroup(group.id);
			} );
	}

	composeTopic(groupId) {
		this.editor({title:"Start a topic", subjectLabel:"Subject"}, async values=>{
			const result = await this.client.request( "createTopic", {groupId, subject:values.subject, markdown:values.markdown} );
			await this.openTopic(result.topic.id);
		} );
	}

	composeReply(topicId, parentId = null) {
		this.editor({title:parentId ? "Reply to message" : "Reply to topic", noSubject:true}, async values=>{
			await this.client.request( "reply", {topicId, parentId, markdown:values.markdown} );
			const topic = await this.client.request( "readTopic", {topicId, offset:0, limit:1} );
			const lastPage = Math.floor((topic.total - 1) / POST_PAGE) * POST_PAGE;
			await this.openTopic(topicId, lastPage);
		} );
	}

	composeEdit(post) {
		this.editor({title:"Edit message", noSubject:true, markdown:post.markdown}, async values=>{
			await this.client.request( "editPost", {postId:post.id, markdown:values.markdown} );
			await this.openTopic(post.topicId, this.postOffset);
		} );
	}

	editor(options, submit) {
		const form = new Popup( options.title, document.body, {modal:true, enableClose:true, suffix:"forum"} );
		form.divContent.classList.add("editor-form");
		let subject = null;
		if( !options.noSubject ) {
			const label = form.appendChild( element("label", "field-label", options.subjectLabel || "Subject") );
			subject = label.appendChild( element("input", "editor-subject") );
			subject.maxLength = 200;
		}
		const tabs = form.appendChild( element("div", "editor-tabs") );
		const write = tabs.appendChild( element("button", "active", "Write") );
		const previewTab = tabs.appendChild( element("button", "", "Preview") );
		const textarea = form.appendChild( element("textarea", "editor-text") );
		textarea.placeholder = options.description ? "Describe this group…" : "Write Markdown or LaTeX: $E=mc^2$";
		textarea.value = options.markdown || "";
		textarea.maxLength = options.description ? 2000 : 100000;
		const preview = form.appendChild( element("div", "editor-preview markdown-body") );
		preview.hidden = true;
		write.onclick = ()=>{ textarea.hidden = false; preview.hidden = true; write.classList.add("active"); previewTab.classList.remove("active"); };
		previewTab.onclick = ()=>{ textarea.hidden = true; preview.hidden = false; write.classList.remove("active"); previewTab.classList.add("active"); renderMarkdown(preview, textarea.value); };
		const hint = form.appendChild( element("p", "editor-hint", "Markdown supported · inline math $…$ · display math $$…$$") );
		const actions = form.appendChild( element("div", "editor-actions") );
		button( actions, "Cancel", ()=>form.hide(), "secondary" );
		button( actions, "Save", async ()=>{
			const values = {subject:subject?.value.trim() || "", markdown:textarea.value.trim()};
			if( subject && !values.subject ) throw new Error(`${options.subjectLabel || "Subject"} is required.`);
			if( !options.description && !values.markdown ) throw new Error("Message is required.");
			await submit(values);
			form.hide();
			form.remove();
		}, "primary" );
		form.show();
		subject?.focus();
		if( !subject ) textarea.focus();
	}

	changed(change) {
		if( this.currentTopic && change.topicId === this.currentTopic.id
			|| !this.currentTopic && this.currentGroup && change.groupId === this.currentGroup.id ) {
			clearTimeout(this.refreshTimer);
			this.refreshTimer = setTimeout( ()=>{
				const refresh = this.currentTopic
					? this.openTopic(this.currentTopic.id, this.postOffset)
					: this.openGroup(this.currentGroup.id, this.groupOffset);
				refresh.catch(showError);
			}, 120 );
		}
		if( change.type === "group" ) this.refreshTree().catch(showError);
	}
}

const client = new ForumClient();
const app = new ForumApplication(client);
const params = new URLSearchParams(location.search);

async function connected(key, guestName = null) {
	app.status.textContent = "Opening forum connection…";
	await client.connect(key, guestName);
	await app.initialize();
}

if( params.has("noauth") ) {
	const name = params.get("noauth") || "Local guest";
	connected( `guest-${crypto.randomUUID()}`, name ).catch(showError);
} else {
	app.status.textContent = "Waiting for user-database login…";
	import( "/node_modules/@d3x0r/user-database-remote/requestService.js" )
		.then(login=>login.requestService( "d3x0r.org", "Sack Forum", token=>
			connected(token.svc.key[0]).catch(showError) ))
		.catch(error=>{
			app.status.textContent = "Login service unavailable";
			showError(error);
		} );
}
