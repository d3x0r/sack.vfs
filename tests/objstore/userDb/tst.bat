copy data.good.before data.good.before.before
copy data.os data.good.before
node testUserDb-auto.mjs  1 5000
:M:\node\debug\node --inspect-brk testUserDb-auto.mjs  1 2000
:M:\node\debug\node  testUserDb-auto.mjs  1 500
copy data.good.after data.good.before.after
copy data.os data.good.after
