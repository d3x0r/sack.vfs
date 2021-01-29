del data.os
node.exe testUserDb-a.mjs 1  10
copy data.os data.good
node.exe testUserDb-a.mjs 200 10
node.exe testUserDb-a.mjs 1500
node.exe testUserDb-a.mjs 800
node.exe testUserDb2.mjs