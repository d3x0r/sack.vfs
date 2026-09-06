rem Only the .mjs sources: events.js is a CommonJS shim with no static exports,
rem so generating from it would emit an empty events.d.ts over the hand-written
rem one. See the comment at the top of events.d.ts.
call ..\makeTypes.bat ./events.mjs
call ..\makeTypes.bat ./events2.mjs
