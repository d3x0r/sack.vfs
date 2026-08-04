
var stdin = process.openStdin(); 
import tty from "tty";
console.log( "tty?", tty );
tty.setRawMode(true);    

stdin.on('keypress', function (chunk, key) {
  process.stdout.write('Get Chunk: ' + chunk + '\n');
  if (key && key.ctrl && key.name == 'c') process.exit();
});