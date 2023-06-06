// Usage: 
//  import {isTopLevel} from "sack.vfs/isTopLevel"
//     isTopLevel( import.meta.url)

import path from 'path';
import { fileURLToPath } from 'url'

const nodePath = path.resolve(process.argv[1]);
export const isTopLevel = ((testPath)=>(nodePath === path.resolve(fileURLToPath(testPath))));

