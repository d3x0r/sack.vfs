# Sack Forum

This is a small forum service built from the local projects in this workspace:

- `sack.vfs` supplies object storage, the HTTP/WebSocket server, JSOX, IDs, and the paged/hash structures.
- `@d3x0r/user-database-remote` supplies the one-shot service login token.
- `@d3x0r/popups2` supplies the modal group/topic/reply/edit forms.
- KaTeX is the only third-party runtime dependency; Markdown parsing is local and escapes raw HTML.

## Storage graph

The fixed `sack-forum-root-v1` object owns BloomNHash indexes for groups, topics, and posts, plus a SlabArray of root groups. Every group has paged child-group and topic arrays. Every topic has a paged chronological post array. Replies refer to a parent post ID, so reply depth does not require rewriting the topic timeline.

## Local setup

From this directory:

```powershell
npm install --ignore-scripts
npm link --ignore-scripts M:\javascript\vfs\native M:\javascript\d3x0r\popups2 M:\javascript\d3x0r\user-database-remote
```

For normal login-backed use:

```powershell
npm start
```

The service registers as `d3x0r.org / Sack Forum` using `service.jsox`. `service.badges.jsox` is the current badge definition; `badges.jsox` and the empty `config.jsox` keep compatibility with login towers that still send the older service bootstrap. The start script loads `sack.vfs/import` so those JSOX modules work on Node. Set `LOGIN_TOWERS` if the defaults in `user-database-remote` are not appropriate. The default port is 8082; `PORT` and `FORUM_STORAGE` override the listener and object-storage filename.

For isolated development without a login tower:

```powershell
$env:FORUM_NO_AUTH="1"
$env:FORUM_ALLOW_GUESTS="1"
npm start
```

Then open `http://localhost:8082/?noauth=Alice`. Guest mode is deliberately opt-in and should not be enabled on a public service.

## Features

- Arbitrarily nested groups and breadcrumb navigation.
- Paged topic and post reads, newest topic first and chronological posts.
- Topic creation, replies to a topic or a particular post, and author-only edits.
- Safe Markdown preview/rendering and KaTeX for `$inline$` and `$$display$$` math.
- WebSocket change subscriptions for live refresh.
- User identity consumed exactly once from `getUser()` when the service socket opens.

Run the focused storage/paging and Markdown safety checks with `npm test`.
