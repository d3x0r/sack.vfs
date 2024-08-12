import {sack} from "sack.vfs"

const db= sack.DB( "maria-jim" );

db.do( "select ?", undefined );

