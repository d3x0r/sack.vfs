

const l = {
    	ids : {
            forumId : null,
        }
        forums : null

}

class Forum extends StoredObject {
    messages = new SlabArray();
    forums = [];

    moderators = [];

    constructor() {
    }


}


class Posting extends StoredObject {

    content = "messageContent";
    poster = null;
    created = new Date();
    updated = new Date();

    forum = null;
    replies = [];
    attachments = [];


    constructor() {

    }
}

class Attachment extends StoredObject {
    filename = "";
    mimeType = "plain/text";
    poster = null; // user this was posted from
    created = new Date();
    content = null; // probably an arraybuffer
    constructor() {
    }

}

class RootPost extends Posting {
    subject = "messageSummary";

    constructor() {

    }
}

class ReplyPost extends Posting {
    replyTo = null;

    constructor() {

    }
}


// singleton
const ForumDb = {
    hook(storage) {
	// init forum/forumId
        // provide extensions for Objects above
    }
}
