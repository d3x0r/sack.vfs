
rollup --format=cjs --file=swbundle.js --no-esModule --  sw.js
#sed 's/exports\.send \= send;//g' -i swbundle.js
#node "~/.node_modules/bin/rollup"  --format=cjs --file=../swbundle.js --no-esModule --  sw.js
cp swbundle.js ..
rm swbundle.js
