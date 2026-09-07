import assert from "node:assert/strict";
import {markdownToHtml} from "./markdown.js";

const rendered = markdownToHtml( "# Safe\n\n<script>alert(1)</script>\n\n[bad](javascript:alert(1)) **strong** `code`" );
assert.match( rendered, /<h1>Safe<\/h1>/ );
assert.match( rendered, /&lt;script&gt;alert\(1\)&lt;\/script&gt;/ );
assert.doesNotMatch( rendered, /<script>/ );
assert.doesNotMatch( rendered, /href="javascript:/ );
assert.match( rendered, /<strong>strong<\/strong>/ );
assert.match( rendered, /<code>code<\/code>/ );
console.log( "safe Markdown tests passed" );
