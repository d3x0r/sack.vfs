import assert from "node:assert/strict";
import {sack} from "sack.vfs";
import {ObjectStorage} from "sack.vfs/object-storage";
import {SlabArray} from "sack.vfs/slab-array";
import {ForumDatabase} from "./forumDb.mjs";

const suffix = `${process.pid}-${Date.now()}`;
const arrayFile = `forum-array-${suffix}.os`;
const arrayStorage = new ObjectStorage( arrayFile );
SlabArray.hook( arrayStorage );
const array = new SlabArray( arrayStorage );
for( let index = 0; index < 405; index++ ) await array.push( `item-${index}` );
assert.equal( array.length, 405 );
assert.equal( await array.get(0), "item-0" );
assert.equal( await array.get(199), "item-199" );
assert.equal( await array.get(200), "item-200" );
assert.equal( await array.get(404), "item-404" );
assert.equal( await array.get(-1), "item-404" );
assert.deepEqual( await array.slice(1, 3, {reverse:true}), ["item-403", "item-402", "item-401"] );
let visited = 0;
await array.forEach( (value, index)=>{
	assert.equal( value, `item-${index}` );
	visited++;
} );
assert.equal( visited, 405 );

const forumFile = `forum-db-${suffix}.os`;
const forumStorage = new ObjectStorage( forumFile );
const forum = await new ForumDatabase().hook( forumStorage );
const alice = {UID:"alice-id", name:"Alice"};
const bob = {UID:"bob-id", name:"Bob"};
const root = await forum.createGroup( alice, {name:"Root", description:"Top level"} );
const child = await forum.createGroup( alice, {name:"Child", parentId:root.id} );
const grandchild = await forum.createGroup( bob, {name:"Grandchild", parentId:child.id} );
assert.deepEqual( (await forum.listGroups(null)).items.map(g=>g.name), ["Root"] );
assert.deepEqual( (await forum.listGroups(root.id)).items.map(g=>g.name), ["Child"] );
assert.deepEqual( (await forum.listGroups(child.id)).path.map(g=>g.name), ["Root", "Child"] );
assert.equal( grandchild.parentId, child.id );

const created = await forum.createTopic( alice, {
	groupId:grandchild.id,
	subject:"Markdown and math",
	markdown:"Hello **world**. $E=mc^2$",
} );
const reply = await forum.reply( bob, {
	 topicId:created.topic.id,
	 parentId:created.post.id,
	 markdown:"A nested reply",
} );
assert.equal( reply.parentId, created.post.id );
assert.equal( (await forum.listTopics(grandchild.id)).total, 1 );
const topic = await forum.readTopic( created.topic.id, 0, 50, bob );
assert.equal( topic.total, 2 );
assert.equal( topic.items[1].canEdit, true );
assert.equal( topic.items[0].canEdit, false );

await assert.rejects(
	forum.editPost( bob, {postId:created.post.id, markdown:"Not mine"} ),
	/Only the author/,
);
const edited = await forum.editPost( alice, {postId:created.post.id, markdown:"Updated"} );
assert.equal( edited.markdown, "Updated" );
assert.ok( edited.edited instanceof Date );

console.log( "forum object-storage tests passed", {arrayItems:array.length, groups:3, posts:2} );
sack.SaltyRNG.setSigningThreads( 0 );
