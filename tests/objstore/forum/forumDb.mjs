import {StoredObject} from "sack.vfs/object-storage-object";
import {BloomNHash} from "sack.vfs/bloom-n-hash";
import {SlabArray} from "sack.vfs/slab-array";

const ROOT_ID = "sack-forum-root-v1";
const DEFAULT_LIMIT = 25;
const MAX_LIMIT = 100;

class ForumRoot extends StoredObject {
	version = 1;
	created = new Date();
	rootGroups = null;
	groups = null;
	topics = null;
	posts = null;
}

class ForumGroup extends StoredObject {
	name = "";
	description = "";
	parentId = null;
	createdBy = null;
	created = new Date();
	updated = new Date();
	children = null;
	topics = null;
	childCount = 0;
	topicCount = 0;
	moderators = [];
}

class ForumTopic extends StoredObject {
	groupId = null;
	subject = "";
	createdBy = null;
	created = new Date();
	updated = new Date();
	posts = null;
	postCount = 0;
}

class ForumPost extends StoredObject {
	topicId = null;
	parentId = null;
	author = null;
	markdown = "";
	created = new Date();
	edited = null;
}

function boundedText(value, field, maximum, required = true) {
	const text = String( value ?? "" ).trim();
	if( required && !text ) throw new Error( `${field} is required.` );
	if( text.length > maximum ) throw new Error( `${field} cannot exceed ${maximum} characters.` );
	return text;
}

function pageArgs(offset, limit) {
	return {
		offset: Math.max( 0, Number(offset) || 0 ),
		limit: Math.min( MAX_LIMIT, Math.max( 1, Number(limit) || DEFAULT_LIMIT ) ),
	};
}

function userSummary(user) {
	if( !user ) throw new Error( "Authentication is required." );
	const id = user.UID ?? user.id ?? user.userId ?? user.clientId;
	if( id === undefined || id === null ) throw new Error( "The login token has no user identifier." );
	return {
		id:String(id),
		name:boundedText( user.name ?? user.displayName ?? "User", "Display name", 120 ),
	};
}

function groupSummary(group) {
	return {
		id:group.id,
		name:group.name,
		description:group.description,
		parentId:group.parentId,
		createdBy:group.createdBy,
		created:group.created,
		updated:group.updated,
		childCount:group.childCount,
		topicCount:group.topicCount,
	};
}

function topicSummary(topic) {
	return {
		id:topic.id,
		groupId:topic.groupId,
		subject:topic.subject,
		createdBy:topic.createdBy,
		created:topic.created,
		updated:topic.updated,
		postCount:topic.postCount,
	};
}

function postSummary(post, viewer) {
	return {
		id:post.id,
		topicId:post.topicId,
		parentId:post.parentId,
		author:post.author,
		markdown:post.markdown,
		created:post.created,
		edited:post.edited,
		canEdit:!!viewer && viewer.id === post.author?.id,
	};
}

export class ForumDatabase {
	storage = null;
	root = null;
	#writeTail = Promise.resolve();

	async hook(storage) {
		this.storage = storage;
		await BloomNHash.hook( storage );
		SlabArray.hook( storage );
		storage.addEncoders( [
			{ tag:"~Fr", p:ForumRoot, f:null },
			{ tag:"~Fg", p:ForumGroup, f:null },
			{ tag:"~Ft", p:ForumTopic, f:null },
			{ tag:"~Fp", p:ForumPost, f:null },
		] );
		storage.addDecoders( [
			{ tag:"~Fr", p:ForumRoot, f:null },
			{ tag:"~Fg", p:ForumGroup, f:null },
			{ tag:"~Ft", p:ForumTopic, f:null },
			{ tag:"~Fp", p:ForumPost, f:null },
		] );

		this.root = await storage.get( {id:ROOT_ID} );
		if( !this.root ) await this.#createRoot();
		await Promise.all( ["rootGroups", "groups", "topics", "posts"].map(
			field=>this.#resolveField( this.root, field )
		) );
		return this;
	}

	async #createRoot() {
		const root = new ForumRoot( this.storage );
		root.rootGroups = new SlabArray( this.storage );
		root.groups = new BloomNHash( this.storage );
		root.topics = new BloomNHash( this.storage );
		root.posts = new BloomNHash( this.storage );
		await root.rootGroups.store();
		await root.groups.store();
		await root.topics.store();
		await root.posts.store();
		await root.store( {id:ROOT_ID} );
		this.root = root;
	}

	async #resolveField(owner, field) {
		if( owner[field] instanceof Promise )
			owner[field] = await this.storage.map( owner[field] );
		return owner[field];
	}

	#write(operation) {
		const result = this.#writeTail.then( operation, operation );
		this.#writeTail = result.catch( ()=>undefined );
		return result;
	}

	async getGroup(id) {
		if( !id ) return null;
		return await this.root.groups.get( String(id) ) || null;
	}

	async getTopicRecord(id) {
		if( !id ) return null;
		return await this.root.topics.get( String(id) ) || null;
	}

	async getPost(id) {
		if( !id ) return null;
		return await this.root.posts.get( String(id) ) || null;
	}

	async groupPath(group) {
		const path = [];
		let cursor = group;
		for( let depth = 0; cursor && depth < 100; depth++ ) {
			path.unshift( groupSummary(cursor) );
			cursor = cursor.parentId ? await this.getGroup( cursor.parentId ) : null;
		}
		return path;
	}

	async listGroups(parentId = null, offset = 0, limit = DEFAULT_LIMIT) {
		const page = pageArgs( offset, limit );
		let list;
		let parent = null;
		if( parentId ) {
			parent = await this.getGroup( parentId );
			if( !parent ) throw new Error( "Group not found." );
			list = await this.#resolveField( parent, "children" );
		} else list = this.root.rootGroups;
		const groups = await list.slice( page.offset, page.limit );
		return {
			parent:parent ? groupSummary(parent) : null,
			path:parent ? await this.groupPath(parent) : [],
			offset:page.offset,
			limit:page.limit,
			total:list.length,
			items:groups.map( groupSummary ),
		};
	}

	createGroup(user, input) {
		return this.#write( async ()=>{
			const author = userSummary(user);
			const parent = input.parentId ? await this.getGroup( input.parentId ) : null;
			if( input.parentId && !parent ) throw new Error( "Parent group not found." );
			const group = new ForumGroup( this.storage );
			group.name = boundedText( input.name, "Group name", 120 );
			group.description = boundedText( input.description, "Description", 2000, false );
			group.parentId = parent?.id || null;
			group.createdBy = author;
			group.children = new SlabArray( this.storage );
			group.topics = new SlabArray( this.storage );
			await group.children.store();
			await group.topics.store();
			await group.store();
			await this.root.groups.set( group.id, group );
			const list = parent ? await this.#resolveField( parent, "children" ) : this.root.rootGroups;
			await list.push( group );
			if( parent ) {
				parent.childCount++;
				parent.updated = new Date();
				await parent.store();
			}
			return groupSummary(group);
		} );
	}

	async listTopics(groupId, offset = 0, limit = DEFAULT_LIMIT) {
		const group = await this.getGroup( groupId );
		if( !group ) throw new Error( "Group not found." );
		const page = pageArgs( offset, limit );
		const topics = await (await this.#resolveField(group, "topics"))
			.slice( page.offset, page.limit, {reverse:true} );
		return {
			group:groupSummary(group),
			path:await this.groupPath(group),
			offset:page.offset,
			limit:page.limit,
			total:group.topicCount,
			items:topics.map( topicSummary ),
		};
	}

	createTopic(user, input) {
		return this.#write( async ()=>{
			const author = userSummary(user);
			const group = await this.getGroup( input.groupId );
			if( !group ) throw new Error( "Group not found." );
			const topic = new ForumTopic( this.storage );
			topic.groupId = group.id;
			topic.subject = boundedText( input.subject, "Subject", 200 );
			topic.createdBy = author;
			topic.posts = new SlabArray( this.storage );
			await topic.posts.store();
			await topic.store();

			const post = new ForumPost( this.storage );
			post.topicId = topic.id;
			post.author = author;
			post.markdown = boundedText( input.markdown, "Message", 100000 );
			await post.store();
			await topic.posts.push( post );
			topic.postCount = 1;
			await topic.store();

			await this.root.topics.set( topic.id, topic );
			await this.root.posts.set( post.id, post );
			await (await this.#resolveField(group, "topics")).push( topic );
			group.topicCount++;
			group.updated = topic.updated = new Date();
			await group.store();
			await topic.store();
			return {topic:topicSummary(topic), post:postSummary(post, author)};
		} );
	}

	async readTopic(topicId, offset = 0, limit = 50, user = null) {
		const topic = await this.getTopicRecord( topicId );
		if( !topic ) throw new Error( "Topic not found." );
		const group = await this.getGroup( topic.groupId );
		const page = pageArgs( offset, limit );
		const posts = await (await this.#resolveField(topic, "posts"))
			.slice( page.offset, page.limit );
		const viewer = user ? userSummary(user) : null;
		return {
			topic:topicSummary(topic),
			group:groupSummary(group),
			path:await this.groupPath(group),
			offset:page.offset,
			limit:page.limit,
			total:topic.postCount,
			items:posts.map( post=>postSummary(post, viewer) ),
		};
	}

	reply(user, input) {
		return this.#write( async ()=>{
			const author = userSummary(user);
			const topic = await this.getTopicRecord( input.topicId );
			if( !topic ) throw new Error( "Topic not found." );
			let parent = null;
			if( input.parentId ) {
				parent = await this.getPost( input.parentId );
				if( !parent || parent.topicId !== topic.id )
					throw new Error( "Reply target is not in this topic." );
			}
			const post = new ForumPost( this.storage );
			post.topicId = topic.id;
			post.parentId = parent?.id || null;
			post.author = author;
			post.markdown = boundedText( input.markdown, "Message", 100000 );
			await post.store();
			await (await this.#resolveField(topic, "posts")).push( post );
			topic.postCount++;
			topic.updated = new Date();
			await topic.store();
			await this.root.posts.set( post.id, post );
			return postSummary( post, author );
		} );
	}

	editPost(user, input) {
		return this.#write( async ()=>{
			const editor = userSummary(user);
			const post = await this.getPost( input.postId );
			if( !post ) throw new Error( "Post not found." );
			if( post.author?.id !== editor.id ) throw new Error( "Only the author can edit this post." );
			post.markdown = boundedText( input.markdown, "Message", 100000 );
			post.edited = new Date();
			await post.store();
			return postSummary( post, editor );
		} );
	}
}

export {ForumRoot, ForumGroup, ForumTopic, ForumPost, ROOT_ID};
