// Canned responses for splitfuzz.mjs / splitpeer.mjs.  Shared so the two
// processes agree on the bytes; imported by a plain-node process, so nothing
// in here may touch sack.vfs.
export const RESPONSES = {
	// sqlite.org's rejection: status line + one header, no blank line, then FIN.
	bare503:    "HTTP/1.1 503 Service Unavailable\r\nConnection: close\r\n",
	// the same, properly terminated
	err503:     "HTTP/1.1 503 Service Unavailable\r\nConnection: close\r\n\r\n",
	// a status line and nothing else at all
	justline:   "HTTP/1.1 503 Service Unavailable\r\n",
	// counted body
	ok200:      "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nhello world",
	// body delimited by the close
	ok200close: "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nbody-bytes",
	nocontent:  "HTTP/1.1 204 No Content\r\n\r\n",
	chunked:    "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n",
};
