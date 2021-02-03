:set NODE=M:\node\debug\node.exe
set NODE=node.exe
del data.os
%NODE% testUserDb-a.mjs 1  10
copy data.os data.good
%NODE% testUserDb-a.mjs 200 10
%NODE% testUserDb-a.mjs 1500 
%NODE% testUserDb-a.mjs 800 
%NODE%  testUserDb2.mjs

echo 10000
%NODE%  testUserDb-a.mjs 10000 1000
echo 10000
%NODE%  testUserDb-a.mjs 20000 1000
echo 100000
%NODE%  testUserDb-a.mjs 153000 10000
%NODE% testUserDb2.mjs 3 203 1583 10032 153293
