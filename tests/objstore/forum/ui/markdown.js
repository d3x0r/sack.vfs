let renderMath = null;
let mathLoad = null;

function escapeHtml(text) {
	return String(text)
		.replaceAll("&", "&amp;")
		.replaceAll("<", "&lt;")
		.replaceAll(">", "&gt;")
		.replaceAll('"', "&quot;")
		.replaceAll("'", "&#39;");
}

function safeLink(url) {
	const value = String(url).trim();
	return /^(https?:|mailto:|\/|#)/i.test(value) ? escapeHtml(value) : "#";
}

function inline(text) {
	const code = [];
	let output = escapeHtml(text).replace(/`([^`]+)`/g, (_all, value)=>{
		code.push( `<code>${value}</code>` );
		return `\u0000CODE${code.length - 1}\u0000`;
	});
	output = output
		.replace(/!\[([^\]]*)\]\(([^\s)]+)\)/g, (_all, alt, url)=>
			`<img src="${safeLink(url)}" alt="${alt}" loading="lazy">` )
		.replace(/\[([^\]]+)\]\(([^\s)]+)\)/g, (_all, label, url)=>
			`<a href="${safeLink(url)}" target="_blank" rel="noopener noreferrer">${label}</a>` )
		.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
		.replace(/__([^_]+)__/g, "<strong>$1</strong>")
		.replace(/(^|[^*])\*([^*]+)\*/g, "$1<em>$2</em>")
		.replace(/(^|[^_])_([^_]+)_/g, "$1<em>$2</em>")
		.replace(/~~([^~]+)~~/g, "<del>$1</del>");
	return output.replace(/\u0000CODE(\d+)\u0000/g, (_all, index)=>code[Number(index)] );
}

export function markdownToHtml(markdown) {
	const lines = String(markdown ?? "").replaceAll("\r\n", "\n").split("\n");
	const blocks = [];
	let paragraph = [];
	let list = null;

	function flushParagraph() {
		if( paragraph.length ) {
			blocks.push( `<p>${inline(paragraph.join("\n")).replaceAll("\n", "<br>")}</p>` );
			paragraph = [];
		}
	}
	function closeList() {
		if( list ) {
			blocks.push( `<${list.type}>${list.items.join("")}</${list.type}>` );
			list = null;
		}
	}

	for( let index = 0; index < lines.length; index++ ) {
		const line = lines[index];
		if( /^```/.test(line) ) {
			flushParagraph(); closeList();
			const language = line.slice(3).trim().replace(/[^a-z0-9_-]/gi, "");
			const code = [];
			while( ++index < lines.length && !/^```/.test(lines[index]) ) code.push( lines[index] );
			blocks.push( `<pre><code${language ? ` class="language-${language}"` : ""}>${escapeHtml(code.join("\n"))}</code></pre>` );
			continue;
		}
		const heading = line.match(/^(#{1,6})\s+(.+)$/);
		if( heading ) {
			flushParagraph(); closeList();
			blocks.push( `<h${heading[1].length}>${inline(heading[2])}</h${heading[1].length}>` );
			continue;
		}
		const item = line.match(/^\s*([-+*]|\d+\.)\s+(.+)$/);
		if( item ) {
			flushParagraph();
			const type = /\d/.test(item[1]) ? "ol" : "ul";
			if( list && list.type !== type ) closeList();
			if( !list ) list = {type, items:[]};
			list.items.push( `<li>${inline(item[2])}</li>` );
			continue;
		}
		const quote = line.match(/^>\s?(.*)$/);
		if( quote ) {
			flushParagraph(); closeList();
			blocks.push( `<blockquote>${inline(quote[1])}</blockquote>` );
			continue;
		}
		if( /^\s*([-*_])(?:\s*\1){2,}\s*$/.test(line) ) {
			flushParagraph(); closeList(); blocks.push("<hr>"); continue;
		}
		if( !line.trim() ) {
			flushParagraph(); closeList();
		} else paragraph.push(line);
	}
	flushParagraph(); closeList();
	return blocks.join("\n");
}

async function loadMath() {
	if( renderMath ) return renderMath;
	if( !mathLoad ) mathLoad = import("/node_modules/katex/dist/contrib/auto-render.mjs")
		.then(module=>renderMath = module.default)
		.catch(()=>null);
	return mathLoad;
}

export async function renderMarkdown(element, markdown) {
	element.innerHTML = markdownToHtml(markdown);
	const renderer = await loadMath();
	if( renderer ) renderer( element, {
		throwOnError:false,
		delimiters:[
			{left:"$$", right:"$$", display:true},
			{left:"\\[", right:"\\]", display:true},
			{left:"\\(", right:"\\)", display:false},
			{left:"$", right:"$", display:false},
		],
	} );
}
