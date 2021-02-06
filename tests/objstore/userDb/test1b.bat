set NODE=M:\node\debug\node.exe
:set NODE=node.exe

del data.os
%NODE% testUserDb-auto.mjs 1  15000
copy data.os data.good
time /T
:15000
%NODE% testUserDb-auto.mjs 1  15000
copy data.os data.good.1
time /T
:30000
%NODE% testUserDb-auto.mjs 1  15000
copy data.os data.good.2
time /T
:45000
%NODE% testUserDb-auto.mjs 1  15000
copy data.os data.good.3
time /T
:60000
%NODE% testUserDb-auto.mjs 1  15000
copy data.os data.good.4
time /T
:75000
%NODE% testUserDb-auto.mjs 1  15000
copy data.os data.good.5
time /T
:90000
%NODE% testUserDb-auto.mjs 1  15000
copy data.os data.good.6
time /T
:105000
:M:\node\debug\node.exe --inspect-brk testUserDb-auto.mjs 1  15000
%NODE% testUserDb-auto.mjs 1  15000
exit
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
time /T
%NODE% testUserDb-auto.mjs 1  15000
