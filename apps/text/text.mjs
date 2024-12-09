/*jslint
    white:true, maxerr: 100, node
*/
"use strict";

/* copyright d3x0r; part of javascript/org.d3x0r.common
  ported from SACK ( github.com/d3x0r/SACK/src/typelib/text.c )
*/

/* usage
 *   var text = require( 'text.js' );
 *   var someText = Text( "some sort of text string" );
 *   var words = text.Parse( [Text object or String that gets converted to Text] [,punctuation [, filter_space [, bTabs,[  bSpaces]]]] ) );
 *      punctuation is a string of punctuation type characters (except . which is always treated as elipses ) *
 *      filter_Space is a string of space type characters
 *      bTabs is a boolean whether to keep tabs or count them.
 *      bSpaces is a boolean wheter to keep spaces or count them. // unimplmented
 *
 *
 *   String( someText ) === "some sort of text string"
 *   String( text.Parse( someText ) ) === "some sort of text string"
 */


// In this final implementation - it was decided that for a general
// library, that expressions, escapes of expressions, apostrophes
// were of no consequence, and without expressions, there is no excess
// so this simply is text stream in, text stream out.

// these are just shortcuts - these bits of code were used repeatedly....


const normal_punctuation=("\'\"\\({[<>]}):@%/,;!?=*&$^~#`");
//static CTEXTSTR not_punctuation;
//console.log( normal_punctuation )

const tab = "\t";
const space = " ";

function textString(self) {
	if (self) {
		//console.log(tab.repeat(self.tabs) + space.repeat(self.spaces) + self.text
		//                 + (self.next?textString( self.next ):"") );
		return tab.repeat(self.tabs) + space.repeat(self.spaces) + self.text
			+ (self.indirect ? textString(self.indirect) : "")
			+ (self.next ? textString(self.next) : "");
	}
	return "";
}


export class TextFlags {

	static OPS = {
      NOOP : 0
			/* this segment clears to the end of the line.  Its content is then added to the output */
			CLEAR_END_OF_LINE : 1
        ,CLEAR_START_OF_LINE: 2/* clear from the current cursor to the start of line 2*/

        ,/* clear the current line; 3 */
		 CLEAR_LINE:3
		,/* clear to the end of the page from this line;4 */
		 CLEAR_END_OF_PAGE:4
        ,/* clear from this line to the start of the page;5 */
		 CLEAR_START_OF_PAGE:5
		,/* clear the entire vieable page (pushes all content to history)
         set cursor home ;6*/
		 CLEAR_PAGE:6
		,/* sets option to not show text at all until next color. ;7*/
		 CONCEAL:7
        ,/* background is how many to delete. ;8*/
		 DELETE_CHARS:8
        ,/* format.x, y are start/end of region -1,-1 clears. ;9*/
		 SET_SCROLL_REGION:9
        ,/* this works as a transaction...;10 */
		 GET_CURSOR:10
		,/* responce to getcursor...;11 */
		 SET_CURSOR:11
		,/* clear page, home page... result in page break...;12 */
		 PAGE_BREAK:12
		,/* break between paragraphs - kinda same as lines...
		 since lines are as long as possible... ;13 */
		PARAGRAPH_BREAK:13
		,/* Justify line(s if wrapped) to the right
		  This attribute should be passed through to renderer;14*/
        JUSTIFY_RIGHT:14
		,/* Justify line(s if wrapped) to the center
		This attribute should be passed through to renderer;15*/
        JUSTIFY_CENTER:15
	};

	is_quote = false;
	is_squote = false;
	is_bracket = false;
	is_brace = false;
	is_tag = false;

	format_rel = true;   // uses relative (tabs, spaces) positioning
	format_abs = false;  // uses absolute (x,y) positioning
	
	binary = false;  // content is a Uint8Array instead?
	no_return = false; // a segment by default does a newline before itself....

	flags = {

	   prior_foreground : true,
		prior_background : true,
		default_foreground : false,
		default_background : false,
	

      /* the foreground color of this segment (0-16 standard console text [ANSI text]) */
		 foreground : 0,
      /* the background color of this segment (0-16 standard console text [ANSI text]) */
		 background : 0,
      /* a bit indicating the text should blink if supported */
		 blink : false,
      /* a bit indicating the foreground and background color should be reversed */
		reverse : false,
		// usually highly is bolder, perhaps it's
      // a highlighter effect and changes the background
		 highlight : false,
		// this is double height modifications to the font...
		 tall : false,
      // this is thicker characters...
		 bold : false,
      // draw a line under the text...
		 underline : false,
		// strike through - if able, draw a line right
		// through the middle of the text... maybe
		// it's a wiggly scribble line?  maybe that
      // could be extended again?
		 strike : false,
      // text is drawn wide (printer kinda font?)
		 wide : false,
       // this is pretty common......
		 italic : false,
		// --
		// these flags are free, but since we already have text segments
		// and I'm bringing in consoles, perhaps we should consider using
		// this to describe captions, but provide the api layer for CTEXTSTR
		// --
		// position data remains constant.
		// text is mounted at the top/left of the
		// first character... (unless center, then
		// the position specifies the middle of the text
		// draw vertical instead of horizontal
		 bVertical:false,
		// draw opposite/upside down from normal
		// vertical/down, right/left upside down if not centered
		// if centered, the text pivots around position.
		 bInvert:false,
		// 0 = default alignment 1 = left, 2 = center 3 = right
		// 0 is not set, the flag set in the lower 32 bit flags
		// is not needed any longer.... anything non zero
		// is that operation to apply.
		 bAlign:0,
      /* format op indicates one of the enum FORMAT_OPS applies to this segment */
		 format_op : TextFlags.OPS.NOOP,

	};
	
	position = {};

	constructor( flags ) {
		const coords = {};
		const offset = {};
		Object.defineProperty( coords, "x_", { writable:true, enumerable:false, value: 0 } );
		Object.defineProperty( coords, "y_", { writable:true, enumerable:false, value: 0 } );
		Object.defineProperty( coords, "x", { set(v) { coords.x_ = v; }, get(){ return coords.x_ }, enumerable:true } );
		Object.defineProperty( coords, "y", { set(v) { coords.y_ = v; }, get(){ return coords.y_ }, enumerable:true } );
		Object.defineProperty( offset, "tabs", { set(v) { coords.x_ = v; }, get(){ return coords.x_ }, enumerable:true } );
		Object.defineProperty( offset, "spaces", { set(v) { coords.y_ = v; }, get(){ return coords.y_ }, enumerable:true } );
	
		Object.defineProperty( this.position, "coords", { value:coords, enumerable:true } );
		Object.defineProperty( this.position, "offset", { value:offset, enumerable:true } );


		["binary", "no_return", "is_quote","is_squote","is_bracket","is_brace","is_tag"].forEach( f=>this[f] = flags[f] );
		if( flags.format_abs ) {
			this.format_abs = true;
			this.format_rel = false;
			this.position.coords.x = flags.x;
			this.position.coords.y = flags.y;
		} else if( flags.format_rel ) {
			this.format_abs = false;
			this.format_rel = true;
			this.position.offset.tabs = flags.tabs||0;
			this.position.offset.spaces = flags.spaces||0;
		}
		for( let f in this.flags ) this.flags[f] = flags[f];
	}
}

export class Text {

	tabs= 0;
	spaces = 0;
	flags= null; // this is actually a whole object if we want to get technical.
	text= "";
	next= null;
	pred= null;
	indirect= null;

		 append(seg) { if (seg) { var end = this; while (end.next) end = end.next; seg.pred = this; end.next = seg; } return seg; }
		 break() { var result; if (result = this.next) { this.next = null; result.pred = null; return result } return null; }
		 breakBefore() { var result; if (result = this.pred) { this.pred = null; result.next = null; return result } return null; }
		 breakAndSpliceTo (start) { var result; if (result = this.pred) { this.pred = start; result.next = null; start.next = this; return result } return null; }
		 forEach(callback) { var cur = this; while (cur) { callback(cur); cur = cur.next; } }
		 toString() {
			var t = this;
			while (t && t.pred) t = t.pred;
			return textString(t);
		}
		 clone() {
			return new Text( { spaces:this.spaces, tabs:this.tabs, text:this.text, flags:this.flags } );
		}
		 Next() { if (!this) return null; return this.next }
		 first() { var cur = this; while (cur.pred) cur = cur.pred; return cur; }
		 get last() { var cur = this; while (cur.next) cur = cur.next; return cur; }

	constructor(def) {
		this.tabs= def && def.tabs || 0
		this.spaces= def && def.spaces || 0;
		this.text= (def && def.text === null) ? null : def && def.text || def || ""
		if( "flags" in def )
			this.flags = new TextFlags( def.flags );
	}
        
        
        static Parse(input, punctuation, filter_space, bTabs, bSpaces)
// returns a TEXT list of parsed data
{
	if (!filter_space) filter_space = "\r";// " \t\r"
	if (!punctuation) punctuation = normal_punctuation;
	if (!input)	// if nothing new to process- return nothing processed.
		return null;
	if (typeof (input) === 'string') { input = Text(input); }
	if (Object.getPrototypeOf(input).constructor.name === 'Buffer') { input = Text(input.toString()); }

	var out = {
		collect: Text()
		, getText: () => {
			if (!out.collect || (out.collect.tabs === 0 && out.collect.spaces === 0 && out.collect.text === ""))
				return null;
			var tmp = out.collect; out.collect = Text(); return tmp;
		}
	};
	var outdata = null,
		word;
	var has_minus = -1;
	var has_plus = -1;

	var index;
	var codePoint;

	var elipses = false;
	var spaces = 0;
	var tabs = 0;

	//console.log( out );
	//console.log( input );

	function SET_SPACES(word) {
		word.tabs = tabs;
		word.spaces = spaces;
		//console.log( `set spaces ${word.tabs} ${word.spaces}  '${word.text}'`)
		tabs = 0;
		spaces = 0;
		return word;
	}
        
	function SegAppend(_this, that) { if (_this === null) return that; return _this.append(that); }

	function collapse() {
		//console.log( "Collapsing:", out.collect.text.length );
		if (out.collect.text.length > 0) {
			//console.log( "New word - set spaces..." );
			outdata = SegAppend(outdata, SET_SPACES(out.getText()));
		}
	}
	function defaultChar() {
		if (elipses) {
			if ((word = out.getText()))
				outdata = SegAppend(outdata, SET_SPACES(word));
			elipses = FALSE;
		}
		out.collect.text += character;
		// characters are added at this point.
		//console.log( `out collect is '${out.collect.text}'`);
	}


	function NextChar() {
		//console.log( `NextChar and... ${index} ${input.text} ${input.text.length} ` );
		if (index < (input.text.length - 1)) {
			var codePoint = input.text.codePointAt(index + 1);
			var character = String.fromCodePoint(codePoint);
			return character;
		} else {
			return "";
		}
	}

	function normalPunctuation() {
		if ((word = out.getText())) {
			outdata = SegAppend(outdata, SET_SPACES(word));
			out.collect.text += character;
			outdata = SegAppend(outdata, out.getText());
		}
		else {
			out.collect.text += character;
			outdata = SegAppend(outdata, SET_SPACES(out.getText()));
		}
	}
	while (input) {
		//Log1( ("Assuming %d spaces... "), spaces );
		//console.log( "input is : ",input, typeof input, Object.getPrototypeOf(input).constructor.name );
		for( var character of  input.text )
		{
			if (elipses && character != '.') {
				outdata = SegAppend(outdata, SET_SPACES(out.getText()));
				elipses = false;
			}
			else if (elipses) // elipses and character is . - continue
			{
				out.collect.text += character;
				continue;
			}
			if (filter_space.includes(character)) {
				if ((word = out.getText())) {
					outdata = SegAppend(outdata, SET_SPACES(word));
				}
				spaces++;
			}
			else if (punctuation.includes(character)) {
				normalPunctuation();
			}
			else switch (character) {
				case '\n':
					if ((word = out.getText())) {
						outdata = SegAppend(outdata, SET_SPACES(word));
					}
					outdata = SegAppend(outdata, Text()); // add a line-break packet
					break;
				case ' ':
				case '\u00a0': // nbsp
					//console.log( "Why wasn't this switch a ' ' ?", ' '.codePointAt(0) );
					collapse();
					spaces++;
					break;
				case '\t':
					if (bTabs) {
						collapse();
						tabs++;
					}
					else {
						defaultChar();
					}
					break;
				case '\r': // a space space character...
					if ((word = out.getText())) {
						outdata = SegAppend(outdata, SET_SPACES(word));
					}
					break;
				case '.': // handle multiple periods grouped (elipses)
					//goto NormalPunctuation;
					{
						let c;
						if ((!elipses &&
							(c = NextChar()) &&
							(c === '.'))) {
					    ON    	if ((word = out.getText())) {
								outdata = SegAppend(outdata, SET_SPACES(word));
							}
							out.collect.text += '.';
							elipses = true;
							break;
						}
						if ((c = NextChar()) &&
							(c >= '0' && c <= '9')) {
							// gather together as a floating point number...
							out.collect.text += character;
							break;
						}
					}
					normalPunctuation();
					break;
				case '-':  // work seperations flaming-long-sword
					if (has_minus == -1)
						if (!punctuation || punctuation.includes('-'))
							has_minus = 1;
						else
							has_minus = 0;
					if (!has_minus) {
						out.collect.text += '-';
						break;
					}
				// fall through...
				case '+':
					{
						let c;
						if (has_plus == -1)
							if (!punctuation || punctuation.includes('+'))
								has_plus = 1;
							else
								has_plus = 0;
						if (!has_plus) {
							out.collect.text += '+';
							break;
						}
						if ((c = NextChar()) &&
							(c >= '0' && c <= '9')) {
							if ((word = out.getText())) {
								outdata = SegAppend(outdata, SET_SPACES(word));
								// gather together as a sign indication on a number.
							}
							out.collect.text += character;
							break;
						}
					}
					if ((word = out.getText())) {
						outdata = SegAppend(outdata, SET_SPACES(word));
						out.collect.text += character;
						word = out.getText();
						outdata = SegAppend(outdata, word);
					}
					else {
						out.collect.text += character;
						word = out.getText();
						outdata = SegAppend(outdata, SET_SPACES(word));
					}
					break;
				default:
					//console.log( "add characater normal...", JSON.stringify( character ), character.codePointAt(0) );
					defaultChar();
					break;
			}
		}
		input = input.next;
	}

	if ((word = out.getText())) // any generic outstanding data?
	{
		outdata = SegAppend(outdata, SET_SPACES(word));
	}

	while (outdata && outdata.pred) outdata = outdata.pred;
	return (outdata);
}
}

Object.seal( TextFlags.OPS );
Object.freeze( TextFlags.OPS );