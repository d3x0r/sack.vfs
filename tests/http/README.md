# tests/http

HTTP client and server tests. Two generations live here: the newer ones cover
the **streaming client** (`sack.HTTP.stream()` — one connection, many requests,
see `../../README_HTTP.md`) and are self-contained and self-checking; the older
ones cover the **one-shot client** (`sack.HTTP.get()`) and are load scripts
pointed at a server you have to supply.

## Running them

Anything that touches `sack.vfs` needs the import hook:

```
node --import sack.vfs/import tests/http/testHttpStream.mjs
```

**Three files are deliberately plain node** — `wirepeer.mjs`, `rawpipe.mjs`
and `exittest.mjs` — and must be run *without* `--import sack.vfs/import`:

```
node tests/http/rawpipe.mjs
```

That is not a style choice. A node `net`/`http` server **listening** in the same
process as sack.vfs dies with `Create view of file for memory access failed` /
`Failed to expand space by <huge>`, so any test that needs a non-sack peer runs
it as its own process. It is also what makes them useful: a failure they
reproduce cannot be blamed on the sack client.

Every test takes `PORT` from the environment. The defaults differ per file so
several can run at once; pick your own `PORT` if something is in the way.

The self-checking ones **exit non-zero on failure**, so they can be run in a
loop or from a script without reading the output.

## Streaming client (`sack.HTTP.stream()`)

| File | PORT | Checks | What it covers |
|----|----|----|----|
| `testHttpStream.mjs` | 8099 | yes | The main suite. Sequential, 20 parallel (verifying each reply lands on *its own* promise), POST, response headers, reject-after-close, reject on a dead port — at pipeline 1 and pipeline 4. |
| `streamsoak.mjs` | 8075 | yes | Volume regression for the `http.c` parser changes: 500 sequential, 500 parallel, 500 pipelined, POST/PUT bodies at 1/10/500/5000/40000 bytes (the server echoes back the byte count it actually received), 20 large responses, and 100 requests through the **old** `HTTP.get()` client, which shares the same parser. `N` sets the count. |
| `testHttpStreamDocs.mjs` | 8061 | yes | Asserts the specific claims `README_HTTP.md` makes — `request({})` defaulting to `GET /`, `bytes` being an ArrayBuffer matching `content.length`, `host:port` overriding `port`, headers/agent/method reaching the server, and 4xx **resolving** rather than rejecting. Keeps the docs from drifting away from the binding. |
| `exittest.mjs` | 8023 | yes | **Plain node.** Spawns `exitrepro.mjs` four ways and asserts the loop-lifetime contract: an open connection pins the process whether idle or not, an in-flight request keeps the loop alive long enough to settle, and only `close()` lets node exit. |
| `exitrepro.mjs` | 8097 | — | The subject of `exittest.mjs`; `STAGE` = `norequest`/`idle`/`pending`/`closed`. Calls no `process.exit()` on purpose — whether node returns to the shell *is* the measurement. Needs `wirepeer.mjs` running. |
| `testHttpStreamPipe.mjs` | 8098 | yes | Pipelining against a chosen server: `SERVER=sack` or `SERVER=node`, `DEPTH`, `COUNT`. Logs what the server actually saw, so dropped or misordered requests are visible. |
| `closerepro.mjs` | 8059 | prints | Bisect harness for connection teardown; `STAGE` = `second` (open/close/**open again**), `embedded`, `404`, `reassign`. `second` is the one that reproduced the loop-thread livelock — a hang here means `Lost client in schedule list ... (Requeuing)` is back. |

## Raw-wire helpers and server-side repros

| File | PORT | Checks | What it covers |
|----|----|----|----|
| `wirepeer.mjs` | 8097 | — | **Plain node.** A raw TCP peer that prints exactly what arrived per read — byte count and request lines — then answers each one. `COALESCE=1` batches all replies for a read into a *single* write, which is what puts several complete responses in one segment. Run it alongside `wireclient.mjs`. |
| `wireclient.mjs` | 8097 | yes | The sack streaming client half; `DEPTH`, `COUNT`. Together with `wirepeer.mjs` this separates "did the client put it on the wire" from "did the server parse it". |
| `rawpipe.mjs` | 8080 | yes | **Plain node.** Writes `COUNT` pipelined requests in one socket write and counts the replies. No sack client anywhere, so it isolates server-side pipelining. This is the repro for the `GatherHttpData` bug where a request swallowed the requests behind it as its own body — 4 in one write used to get 1 reply. |
| `sackserver.mjs` | 8081 | — | Bare `sack.WebSocket.Server` that logs each request it serves; the server half for `rawpipe.mjs`. |
| `wstest.mjs` | 8067 | yes | Websocket upgrade regression. The 101 response is the case the request/response content-state fix in `GatherHttpData` deliberately left alone, so this guards it: `N` echoed messages (default 50) plus a plain HTTP request on the same server. |

## One-shot client (`sack.HTTP.get()` / `sack.HTTPS.get()`)

Older load scripts. They are **not self-contained** — they hard-code a target
(mostly `localhost:7000` and a `/test/test.php` resource) and print rather than
assert, so a "pass" means no errors scrolled by. The `.10.` variants are the
same script aimed at `10.173.0.1:443` instead of localhost.

| File | Shape |
|----|----|
| `testHttpGets.mjs` | 1000 async GETs against `localhost:8084`, collected with `Promise.all`. The most compact async smoke test. |
| `testHttpPost.js` | 500 **synchronous** POSTs — no `onReply`, so each call blocks until its response arrives. |
| `testHttpPost2.js` | 100 POSTs with `onReply`, dumping each response. |
| `testHttpPostAsync.js` / `.10.js` | Async POSTs with status/content-type handling and 301 following. |
| `testHttpsPost.local.js` | 500 TLS **PUT**s with the query in the path. Note it declares two options blocks — `optsxx` (POST) is dead, `opts` (PUT) is the one passed to `get()`. |
| `testHttpsPost.10.js` | The same script — also PUT, also the dead `optsxx` — aimed at `10.173.0.1:443`. |
| `testHttpsRequest.js` | 100 TLS GETs at `/`, with 301 following. |
| `testHttpFastResponse.mjs` | Not a client — a tiny server fixture that answers everything immediately with `"abc"` and CORS headers. Port comes from `argv[2]`, default 4321. |

TLS ones need `SSL_PATH` pointing at a directory holding `fullchain.pem` and
`privkey.pem`.

## Suggested order

`testHttpStream.mjs` first (broadest, fastest), then `streamsoak.mjs` for
volume, `wstest.mjs` for the upgrade path, `exittest.mjs` for process lifetime,
and `rawpipe.mjs` for server-side pipelining. `testHttpStreamDocs.mjs` whenever
`README_HTTP.md` changes.

## Not a test

`install.cmd` is a Claude Code bootstrap script that ended up in this directory
by accident. It has nothing to do with HTTP.
