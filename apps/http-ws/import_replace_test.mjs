import "./banana.mjs"

const str = 'import {sack} from "sack.vfs" \n\
import {local} from "./test.js"  \n\
import {another} from "/node_modules/something"  \n\
import * as foo from "../banana"  \n\
import * as foo from "banana/foo"  \n\
import THREE from "three"\n\
import "foo"\n\
import("foo" )\n\
import( "foo" )\n\
import     ( "foo" )\n\
import     ("foo" )\n\
import     (        "foo" )\n\
';


const str2 = str.replaceAll( /import([^\(]?\s+[^\(]?.*from\s+|)["']((?!\/|.\/|..\/)[^'"]*)["']/g, 'import$1"/$2"'  )
console.log( "out:", str2 );


const str3 = str.replaceAll( /import([^\(]?\s+[^\(]?.*from\s+|)["']((?!\/|.\/|..\/)[^'"]*)["']/g, 'import$1"/$2?🔨=' + "(referer)" + '"' )
console.log( "out:", str3 );
