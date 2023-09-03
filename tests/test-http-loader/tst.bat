
set FORCE_IMPORT_MODULE=true


node --experimental-loader ../../import.mjs test.js

:node --experimental-loader ./import.mjs https://localhost:8084/test.mjs
