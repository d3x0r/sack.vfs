:set NODE=M:\node\debug\node.exe
set NODE=node.exe

del data.os
%NODE%  testUserDb-a.mjs 1  
copy data.os data.good
%NODE%   testUserDb-a.mjs 100 
%NODE%   testUserDb-a.mjs 1500
%NODE%   testUserDb-a.mjs 800
%NODE% testUserDb2.mjs