import {sack} from "sack.vfs"

const addr = process.argv[2] || "1.1.1.1";
const names = sack.Network.ICMP.nameOf( addr );
console.log( "nameof", addr, ":", names );
