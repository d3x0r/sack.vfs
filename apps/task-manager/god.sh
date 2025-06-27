export DEBUG=*
export NVM_DIR=$HOME/.nvm
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm
nvm use 17
#call npm run start
gdb --args node  --experimental-loader=sack.vfs/import.mjs  src/main.mjs

#exec $*
