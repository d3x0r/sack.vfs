
import {sack} from "sack.vfs"

//const com10s= sack.DB.so( "COMPORTS.INI", "COM PORTS/com10", "9600,N,8,1,carrier,Rts,rTSflow",  );
//const com8s = sack.DB.so( "comports.ini", "COM PORTS/com8", "9600,N,8,1,carrier,Rts,rTSflow" );

//const com10 = sack.DB.op( "COM PORTS", "com10", "9600,N,8,1,carrier,Rts,rTSflow", "COMPORTS.INI" );
//const com8 = sack.DB.op( "COM PORTS", "com8", "9600,N,8,1,carrier,Rts,rTSflow", "COMPORTS.INI" );
//console.log( "oldsettings:", com10, com8 );
///comports.ini/COM PORTS/&lt;comName&gt; = 57600,N,8,1,carrier,RTS,rTSflow



const decoder = new TextDecoder();
const encoder = new TextEncoder();

class GPS {
	good = false;
	warning = false;

	satViews = [];
	inView = 0;
	time = { hr:0, mn:0, sc:0 };
	time2 = { hr:0, mn:0, sc:0 };
	time3 = { hr:0, mn:0, sc:0 };
	time4 = { hr:0, mn:0, sc:0 };
	localZone_hr = 0;
	localZone_mn = 0;

	date2 = 0;
	date = {yr:0,mn:0,dy:0};

	lat = { deg: 0 }
	long = { deg: 0 }
	lat2 = { deg: 0 }
	long2 = { deg: 0 }
	lat3 = { deg: 0 }
	long3 = { deg: 0 }
	ll3Valid = false;
	heading_mag = 0;
	heading_mag2 = 0;
	heading2 = 0;
	heading_true = 0;

	qual = 0;

	speed = { knots: 0, knots2: 0, km:0 }

	modeDop = {
		auto : false,
		manual : false,
		mode : 0,
		SV : [],
		PDOP: 0,
		HDOP: 0,
		VDOP: 0,
	};


	handleData( data){

	/*
	data: 2 $GPRMC,194341.00,A,3034.22750,N,08132.14664,W,0.028,,200725,,,A*6F
	$GPVTG,,T,,M,0.028,N,0.052,K,A*2E
	$GPGGA,194341.00,3034.22750,N,08132.14664,W,1,07,1.49,3.6,M,-31.4,M,,*6E
	$GPGSA,A,3,24,15,18,10,32,23,27,,,,,,2.60,1.49,2.13*05
	$GPGSV,3,1,10,05,01,104,,10,34,314,16,12,00,129,,15,26,050,38*7B
	$GPGSV,3,2,10,18,69,177,35,23,60,011,24,24,55,078,38,27,13,292,18*7C
	$GPGSV,3,3,10,29,01,182,,32,32,252,32*7D
	$GPGLL,3034.22750,N,08132.14664,W,194341.00,A,A*7E
	$GPZDA,194341.00,20,07,2025,00,00*6C


	*/
		const packet = decoder.decode(data);
		const lines = packet.split('\r\n' ).reduce( (acc,l)=>(l[0] === '$'?(acc.push(l),acc):acc), [] );
		const chksums = lines.map( l=>l.split('*') );
		//console.log( "chks:", chksums );
		const chksum_checks = chksums.reduce( (acc,l)=>{ 
			if( l.length != 2 ) return acc;
			const bytes = encoder.encode( l[0]  );//+'*'+l[1]+'\r\n' );
			let sum = 0;
			for( let i = 1; i < bytes.length ;i++ ) {
				sum ^= bytes[i];
			}
			const c = parseInt( l[1], 16 );
			if( c === sum ) acc.push( l[0] );
			//console.log( "got:", sum.toString(16), l[1], c );
			return acc }, [] );

		// at this point chksum_checks has only lines that matched the checksum
		//console.log( "chks:", chksum_checks );
		const argMap = chksum_checks.map( l=>l.split(',') );

		//console.log( "data:", port, argMap  );
	/*
	$GPBOD - Bearing, origin to destination
	$GPBWC - Bearing and distance to waypoint, great circle
	$GPGGA - Global Positioning System Fix Data
	$GPGLL - Geographic position, latitude / longitude
	$GPGSA - GPS DOP and active satellites 
	$GPGSV - GPS Satellites in view
	$GPHDT - Heading, True
	$GPR00 - List of waypoints in currently active route
	$GPRMA - Recommended minimum specific Loran-C data
	$GPRMB - Recommended minimum navigation info
	$GPRMC - Recommended minimum specific GPS/Transit data
	$GPRTE - Routes
	$GPTRF - Transit Fix Data
	$GPSTN - Multiple Data ID
	$GPVBW - Dual Ground / Water Speed
	$GPVTG - Track made good and ground speed
	$GPWPL - Waypoint location
	$GPXTE - Cross-track error, Measured
	$GPZDA - Date & Time
	*/
		for( let line of argMap ) {
			let arg = 1;
			//console.log( "msg:", line.join(',' ))
			switch( line[0] ) {
			case "$GPZDA":
				// time and date only
				{
				const time_bcd = Number( line[arg++] );
				this. time3.hr= Math.floor( time_bcd/10000  );
				this. time3. mn= Math.floor( (time_bcd/100)%100  );
				this. time3. sc= time_bcd%100;
				this. date.dy = Number( line[arg++] );
				this. date.mn = Number( line[arg++] );
				this.date.yr = Number( line[arg++] );
				this.localZone_hr = Number( line[arg++] );
				this.localZone_mn = Number( line[arg++] );
				//console.log( "Specific time:", time, date_year, date_month, date_day, localZone_hr, localZone_mn );
				}

				break;
			case "$GPGLL":
				// $GPGLL,3034.22750,N,08132.14664,W,194341.00,A,A*7E	
				{
				this. lat3.deg = Number( line[arg++] ) * ( line[arg++]==='W'?-1:1)
				this.long3.deg = Number( line[arg++] ) * (line[arg++]==='N'?1:-1);
				const time_bcd = Number( line[arg++] );
				this.time4.hr= Math.floor( time_bcd/10000  );
				this.time4. mn= Math.floor( (time_bcd/100)%100  );
				this.time4. sc=  time_bcd%100 ;
				this.ll3Valid = line[arg++] === "A";
				}
				break;
			case "$GPGSA":
				this.modeDop.auto = line[1] === 'A',
				this.modeDop.manual = line[1] === 'M',
				this.modeDop.mode = Number(line[2] ),
				this.modeDop.SV = line.slice( 3, 14 ).map( v=>v?Number(v):null );
				this.modeDop.PDOP= Number(line[15]);
				this.modeDop.HDOP= Number(line[16]);
				this.modeDop.VDOP= Number(line[17]);
				//console.log( "mode:", this.modeDop );
				break;
			case "$GPGSV":
				// satellites in view
				{
					//const totalMsgs = Number(line[1] );
					const msg = Number(line[2] );
					this.inView = Number( line[3] );
					const sats = [];
					if( msg === 1 ) {
						this.satViews.forEach( sat=>sat.active = false );
					}
					for( let n = 0; n < 4 && 4+n*4 < line.length; n++ ) {
						if( !line[4+n*4] ) continue;
						const PRN = Number( line[4+n*4] );
						const oldRec = this.satViews.find( sat=>sat.PRN===PRN );
						if( oldRec ) {
							oldRec.active = true;
							oldRec.elev = Number(line[5+n*4] )
							oldRec.azi = Number(line[6+n*4] )
							oldRec.SNR = Number(line[7+n*4] );
						}else {
							const rec={ 
								active : true
								, PRN : Number( line[4+n*4] )
								, elev : Number(line[5+n*4] )
								, azi : Number(line[6+n*4] )
								, SNR : Number(line[7+n*4] ) } ;
							sats.push(rec);
						}
					}
					
					this.satViews.push( ...sats );
					//console.log( "SV:", line );
					//console.log( "sats:", sats );
				}
				break;
			case "$GPGGA":
				{
				const time_bcd = Number( line[1] );
				this.time.hr= Math.floor( time_bcd/10000  );
				this.time.mn= Math.floor( (time_bcd/100)%100  );
				this.time.sc= Math.floor( time_bcd%100  );

				this. lat.deg = Number( line[2] ) * (line[3]==='S'?-1:1);
				this. long .deg = Number( line[4] ) * (line[5]==='W'?-1:1) ;
				this. qual = Number(line[6] );
				this.inUse = Number( line[7] );
				this.horrizDil = Number(line[8] );
				this.alt = Number(line[9] ); // M (meters)
				this.geoidalSep = Number( line[11] ); // M(meters)
				this.age = Number( line[13] );
				this.diffStation = Number( line[14] );
		
				//const meters = Number(1
				}
				break;
			case "$GPVTG":
				//   [ '$GPVTG,,T,,M,0.033,N,0.062,K,A', '27' ],
				{
					if( line[1] ) {
						// true degrees
						this.heading_true = Number( line[1] );
					}
					if( line[3] ) {
						// magnetic degrees
						this.heading_mag = Number( line[2] );
					} 
					if( line[5] ) {
						// spd knots
						this.speed.knots = Number( line[5] );
					}
					if( line[7] ) {
						// spd km/hr
						this.speed.km = Number( line[7] );
					}
				}
				break;
			case "$GPRMC":
				// ,194341.00,A,3034.22750,N,08132.14664,W,0.028,,200725,,,A*6F
				//Time, date, latitude, longitude, speed over ground, course over ground, and a validity fla	
				{
				const time_bcd = Number( line[1] );
				this.time2.hr = Math.floor( time_bcd/10000  )
				this.time2. mn= Math.floor( (time_bcd/100)%100  )
				this.time2.sc= time_bcd%100 ;
				this. good = (line[2] =='A');
				this. warning = (line[2] =='V');

				this. lat2.deg = Number( line[3] ) * (line[4]==='S'?-1:1);
				this. long2 .deg = Number( line[5] ) * (line[6]==='W'?-1:1) ;

				this. speed.knots2 = Number( line[7] );
				this. heading2 = Number( line[8] );
				this. date2 = Number( line[9] );
				this. heading_mag2 = Number( line[10] ) * ( line[11]==='E'?1:-1);

				}
				break;
			default:
				console.log( "Unhandled line:", line );
				break;
			}
		}
	//console.log( "sats:", this );
	} 

}

const GPS1 = sack.ComPort( "\\\\.\\com10" );
const GPS1data = new GPS(1);
GPS1.onRead( (data)=>GPS1data.handleData(data ) );

const GPS2 = sack.ComPort( "com8" );
const GPS2data = new GPS(2);
GPS2.onRead( (data)=>GPS2data.handleData(data ) );


function tick() {
	console.log( [ "\x1b[H"
			, "GPS1:", GPS1data.time.hr.toString().padStart(2, '0')+":"+GPS1data.time.mn.toString().padStart(2,'0')+":"+GPS1data.time.sc.toString().padStart(2,'0')
			,  " Alt:" + GPS1data.alt, JSON.stringify(GPS1data.speed), "\x1b[K\n"
			, "GPS2:", GPS2data.time.hr.toString().padStart(2, '0')+":"+GPS2data.time.mn.toString().padStart(2,'0')+":"+GPS2data.time.sc.toString().padStart(2,'0'), " Alt:"+GPS2data.alt, JSON.stringify(GPS2data.speed), "\x1b[K\n"
	] .join('')
	)
	setTimeout( tick, 1000 );
}

console.log( "\x1b[2J")
setTimeout( tick, 1000 );