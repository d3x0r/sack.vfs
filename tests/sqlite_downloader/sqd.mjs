import {sack} from "sack.vfs"


// path is always absolute...
sack.HTTPS.get( { hostname:"sqlite.org", path:"/download.html", onReply(res) {
		if( res.statusCode === 200 ) {
			const filePath = findSqliteAmalgamation( res.content, "/" );
			sack.HTTPS.get( {hostname:"sqlite.org", path:filePath.relativeUrl, onReply(res) {
				if( res.statusCode === 200 ) {
					const fne = filePath.relativeUrl.split('/' );
					const fn = fne[fne.length-1].split( '.' );
					if (res.bytes?.length && Number(filePath.sizeBytes) !== res.bytes.length) {
					  console.log("size mismatch:", res.bytes.length, "expected", filePath.sizeBytes);
					  return;
					}
					const hash = sha3_256(res.bytes);
					if (filePath.sha3 && hash !== filePath.sha3) {
					  console.log("sha3 mismatch:", hash, "expected", filePath.sha3);
					  return;
					}
 	 				sack.Volume().write( "sqlite.zip", res.bytes );				
					//sack.Volume().write( "sqlite-name.txt", fn[0] );
					if( process.platform === "win32" ) {
						sack.Task( { bin:"pkzip25.exe", args:["-ext","-dir","sqlite.zip"]
							, end() {
								console.log( "unzipped..." );
								sack.Task( { bin:"node.exe", args:["apply-table-alias.mjs",fn[0]]
									, end() {
										console.log( "updated." );
										sack.Volume().rm( "sqlite.zip" );
									} } );
   
						} } );
					} else {
						console.log( "attempting unzip (untested, non win32)" );
						sack.Task( { bin:"unzip", args:["sqlite.zip"]
							, end() {
								console.log( "unzipped..." );
								sack.Task( { bin:"node", args:["apply-table-alias.mjs",fn[0]]
									, end() {
										console.log( "updated." );
										sack.Volume().rm( "sqlite.zip" );
									} } );
   
						} } );
					}
				}
			} } )
		}
	} } );

function findSqliteAmalgamation(html, baseUrl = "https://www.sqlite.org/") {
  const marker = "Download product data for scripts to read";
  const commentRe = /<!--([\s\S]*?)-->/g;

  for (const match of html.matchAll(commentRe)) {
    const body = match[1].trim();
    if (!body.startsWith(marker)) continue;

    const lines = body.split(/\r?\n/).map(line => line.trim()).filter(Boolean);
    const headerIndex = lines.findIndex(line =>
      line.startsWith("PRODUCT,VERSION,RELATIVE-URL,SIZE-IN-BYTES,SHA3-HASH")
    );

    if (headerIndex < 0) throw new Error("SQLite product CSV header not found");

    for (const line of lines.slice(headerIndex + 1)) {
      const [kind, version, relativeUrl, sizeBytes, sha3] = line.split(",", 5);

      if (
        kind === "PRODUCT" &&
        /^sqlite-amalgamation-\d+\.zip$/.test(relativeUrl.split("/").pop())
      ) {
        return {
          version,
          relativeUrl,
          //url: new URL(relativeUrl, baseUrl).href,
          sizeBytes: Number(sizeBytes),
          sha3
        };
      }
    }
  }

  throw new Error("SQLite amalgamation zip not found");
}


import { createHash } from "node:crypto";

function sha3_256(bytes) {
  return createHash("sha3-256").update(Buffer.from(bytes)).digest("hex");
}


//setTimeout( ()=>{}, 5000 );