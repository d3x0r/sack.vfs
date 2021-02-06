del data.os
node.exe testUserDb-auto.mjs 1  15000
copy data.os data.good
time /T
:15000
node.exe testUserDb-auto.mjs 1  15000
time /T
:30000
node.exe testUserDb-auto.mjs 1  15000
time /T
:45000
node.exe testUserDb-auto.mjs 1  15000
time /T
:60000
node.exe testUserDb-auto.mjs 1  15000
time /T
:75000
node.exe testUserDb-auto.mjs 1  15000
time /T
:90000
node.exe testUserDb-auto.mjs 1  15000
copy data.os data.os.good.5
time /T
:105000
:M:\node\debug\node.exe --inspect-brk testUserDb-auto.mjs 1  15000
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
time /T
node.exe testUserDb-auto.mjs 1  15000
