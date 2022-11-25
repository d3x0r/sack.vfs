//const resolve = require('@rollup/plugin-node-resolve')
//const commonjs = require('@rollup/plugin-commonjs')
import {terser} from 'rollup-plugin-terser'
import gzipPlugin  from 'rollup-plugin-gzip'
//const pkg = require('./package.json')

export default [
    // ES6 Modules Non-minified
    {
        input: 'sw.js',
        output: {
            file: '../sw.mjs',
            format: 'esm',
        },
        plugins: [
        ],
    },
    // ES6 Modules Minified
    {
        input: 'sw.js',
        output: {
            file: "../sw.min.mjs",
            format: 'esm',
        },
        plugins: [
            terser(),
				gzipPlugin() ,
        ],
    }
]
