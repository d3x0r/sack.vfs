
# Import utility

Enable import of .json6 and .jsox files with simple `const config = require( "config.jsox" )` or `import config from "./config.jsox"` sort of syntax.

```
node --import sack.vfs/import
```

environment variable...

```
NODE_OPTIONS="--import sack.vfs/import"
```

Also enables HTTP(S) requests for sources.

(Something like this)

```
import {THREE} from "https://cdn.example.net/threejs.js"
```

Adds some other import interfaces such that one can force loading a module with '.js' extension.


## Methods of sack.import

```
sack.import.force( "script.js" );  // don't include the ./ or ../ part of paths
import whatever from "./script.js"
sack.import.forget( "script.js" );
```

then import.mjs script loaded into node gets the list with

```
const forcedModules = sack.import.modules
```

`modules` is a getter that returns an array of strings that are the names of modules to be forced.