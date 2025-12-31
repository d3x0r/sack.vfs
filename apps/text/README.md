
# JS Text Object

This is a text tokenizer, it generates a list of text segments.

A text segment contains a string 'text' and the tabs and spaces before the segment (tabs preceed spaces, and a blank segment will exist for `\t\b\t\b`)


This is primarily a text parser based on ascii puntuation marks and whitespace; whitespace is collected into the text segment to be able to render the text
into the original string.  Some usages may truncate their segments when they split tokens off.


nbsp and sp are both counted as spaces; so I guess the exact string can't be recovered, though it will look the same.

## Text class

Constructor takes an object with `{tabs:<number of tabs>, spaces:<number of spaces>, text:"the text for the segment`.

Static Methods

| name | args | description |
|---|---|---|
| constructor | ( def ) | def is an object like `{tabs:0, spaces: 1, text: "one" }` |
|Parse | (input, punctuation, filter_space, bTabs, bSpaces ) | input is a string that is parsed.  Punctuation is an optional parameter which will use `'"\({[<>]}):@%/,;!?=*&$^~#``` if not specified. filter_space specifies characters to filter out and just ignore; if not specified defaults to carriage return `\r`.  bTabs and bSpaces are booleans that control whether spaces and tabs are counted or are just appened to the text segment. |
|next | (Text) | return the next segment (if there is one, and if the argument is not `null`, otherwise return `null`). |
|prior | (Text) | return the previous segment (if there is one, and if the argument is not `null`, otherwise return `null`). |
|first | (Text) | return the first segment of the list that contains this segment.  (Does not follow up indirects) |
|last | (Text) | return the last segment of the list that contains this segment.  (Does not follow up indirects) |
|append| (a,b) | Append segment A and B, links the last(A) to first(B) |
|subst| (_this, that) | replace _this segment with _that.  returns `that` |
|substRange | (from,to,that) |splice out segments 'from' to 'to' and link first(that) and last(that) in their places, replacing a span of segments potentially |


Methods

| name | args | description |
|---|---|---|
|append| (seg) | appends segment to the end of the list of segments following `this`|
|break| () | breaks the list of segments after `this`, making `this` the last segment, and returns the old next segment of `this` if any.|
| breakBefore|() | breaks the list of segments before `this`, making `this` the first in a list.  It returns the old previous segment of `this` if any.|
| breakAndSpliceTo|( start ) | puts `start` before `this`.  (no sort of check if there was already a previous or next, the two segments are linked together period.) |
| forEach| (cb) | for each segment in the list, call the callback with each segment.  The callback takes one argument which will be a `Text` object. |
| toString | () | Goes to the first segment of the list `this` is in, and returns the whole formatted string as a single result. |
| clone | () | make a copy of this segment which has the same content, but may be linked into a different list. |
| Next | () | get the next segment after `this`, if any. |
| first | () | get the first segment of this list `this` is in. |
| last | getter | returns the last segment of this list. |


Properties 

| name | type | description |
|----|----|----|
|tabs | number | number of tabs before this segment |
| spaces | number | number of spaces before this segment |
| flags | object | TBD.  Includes things like color attributes, foreground, background, blink, underline, bold, invert, extended positioning(specify absolute position to emit, or relative, extended functions like clearing all text that has been output). |
| text | string | content of the segment |
| next | Text | next segment in this string |
| pred | Text | predaccessor of segment in this string.|
| indirect | Text | indirect segments should not have text; they will instead have indirect set to a new segment chain which will be used for the text content of this segment. Moving grouping flags out, things like braces and brackets can be used as a flag in a Text segment around an indirect of a long expression.|


Flags Object (TBD)

| name | type | description | 
|---|---|----|
| op | string | extended operation to perform, this will have custom fields in the flags probably? |
| paren | bool | this segment is bounded by parenthesis `()` before the tabs and spaces and after the text segment, and after the indirect. |
| bracket | bool | this segment is bounded by brackets `[]` before the  tabs and spaces and after the text segment, and after the indirect. |
| brace | bool | this segment is bounded by braces {}` before the  tabs and spaces and after the text segment, and after the indirect. |
| tag | bool | this segment is bounded by tag indicators `<>` befoere the  tabs and spaces and after the text segment, and after the indirect. |
| color | { foreground:, background,  blink, ...} | specify color attributes for this segment.


## VarText

Variable text utility, used to append strings into a buffer, which then is sliced into Text segments retrieving the data.

## TextFlags

A class object that describes extra formatting information for a segment; may include color, or other text styling, positioning,
and even extended format commands like 'clear screen'.


## Changelog

- 1.2.125
   - Added a bunch of static methods; added more getters on the text segment, and hide internal data members.