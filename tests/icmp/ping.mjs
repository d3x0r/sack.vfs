import {sack} from "sack.vfs"

sack.Network.ICMP.ping( (obj)=>{console.log( "result:", obj ) }, process.argv[2] || "www.google.com", 0, 10 );
