:set NODE=M:\node\debug\node.exe
set NODE=node.exe
del data.os
%NODE% testUserDb-a.mjs 1  100000
copy data.os data.good
%NODE% testUserDb-a.mjs 2000200 10
%NODE% testUserDb-a.mjs 20001500
%NODE% testUserDb-a.mjs 2000800
%NODE% --inspect-brk testUserDb2.mjs

echo 10000
%NODE% --inspect-brk testUserDb-a.mjs 10000 10000
echo 10000
%NODE% --inspect-brk testUserDb-a.mjs 20000 10000
echo 100000
%NODE% --inspect-brk testUserDb-a.mjs 100000 100000
%NODE% testUserDb2.mjs 3 203 1583 10032 153293
