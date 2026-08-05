

rint = require('readline').createInterface( process.stdin, {} ); 
process.stdin.setRawMode(true);

process.stdin.on('line',function( char, key) {
	console.log( "line?", char );
} )
process.stdin.on('keypress',function( char, key) {
	console.log( "keypress??" );
    //console.log(key);
    if( key == undefined ) {
        process.stdout.write('{'+char+'}')
    } else {
        if( key.name == 'escape' ) {
            process.exit();
        }
        process.stdout.write('['+key.name+']');
    }

}); 

// what to do with a single byte buffer
let processByte = (buff) => {
    console.log(buff);
};

let processBuff = (buff) => {
    let char = buff.toString(),
    hex = buff.toString('hex');
    console.log('control stroke');
    console.log('hex: ' + hex);
    console.log('str: ' + char);
    console.log('data buff length: ' + buff.length);
};
let isRaw = () => {
    if(process.stdin.setRawMode){
       return true;
    }
    return false;
};
// modes for raw use and pipping
let modes = {
    // for raw mode ( $ node app.js)
    raw: (data) => {
        // char and hex strings
        let char = data.toString(),
        hex = data.toString('hex');
        // exit code check (press q,Q, or ctrl+c aka '03' in hex)
        if(char.toLowerCase() === 'q' || hex === '03'){
            onQuit();
        }else{
            if(data.length === 1){
                processByte(data);
            }
            if(data.length > 1){
                processBuff(data);
            }
        }
    },
    // for pipping ( $ echo -n "abcd" | node app.js)
    notRaw: (data) => {
        var i = 0,
        len = data.length;
        while(i < len){
            processByte(data.slice(i, i + 1));
            i += 1;
        }
    }
};
process.stdin.on('data', (data) => {
    if(isRaw()){
        modes['raw'](data);
    }else{
        modes['notRaw'](data);
    }
});

//require('tty').setRawMode(true);
//setTimeout(process.exit, 10000);
