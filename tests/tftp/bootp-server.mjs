

	static  message67( msg, rinfo ) {
		msg = new Uint8Array( msg );
		console.log( "67 got message:", msg, rinfo );
		const fmsg = {
			op:msg[0],
			htype : msg[1],
			hlen : msg[2],
			hops : msg[3],
			xid : (msg[4]<< 24)|(msg[5]<< 16)|(msg[6]<< 8)|(msg[7]<< 0),
			secs : (msg[8]<<8) | msg[9],
			unused : (msg[10]<<8) | msg[11],
			ciaddr : (msg[12]<< 24)|(msg[13]<< 16)|(msg[14]<< 8)|(msg[15]<< 0),
			yiaddr : (msg[16]<< 24)|(msg[17]<< 16)|(msg[18]<< 8)|(msg[19]<< 0),
			siaddr : (msg[20]<< 24)|(msg[21]<< 16)|(msg[22]<< 8)|(msg[23]<< 0),
			giaddr : (msg[24]<< 24)|(msg[25]<< 16)|(msg[26]<< 8)|(msg[27]<< 0),
			chaddr : msg.slice( 28, 28+16 ),
			sname : '',  // 44 - 44+64
			file : '',  // 108 - 108+128
			vend : msg.slice( 236, 236+64 ),	// 236 - 236+64
			vend : {
				magic : (msg[236]<<24)|(msg[237]<<16)|(msg[238]<<8)|(msg[239]<<0),  // 0x63825363
				flags : (msg[240]<<24)|(msg[241]<<16)|(msg[242]<<8)|(msg[243]<<0),  // 
				pad : [...msg.slice( 244, 244+56 )].map(n=>n.toString(16)),
			}
		};
		{
			const s = [];
				for( let i = 44; i < msg.byteLength; i++ ) {
					const c = msg[i];
					if( c ) {
						s.push( String.fromCodePoint( c ) );
					} else break;
				}
			fmsg.sname = s.join('');
		}
		{
			const s = [];
				for( let i = 44; i < msg.byteLength; i++ ) {
					const c = msg[i];
					if( c ) {
						s.push( String.fromCodePoint( c ) );
					} else break;
				}
			fmsg.file = s.join('');
		}
		

		if( fmsg.op === 1 ) {
			/*
request: {
  op: 1,
  htype: 1,
  hlen: 6,
  hops: 0,
  xid: -53538292,
  secs: 10,
  unused: 0,
  ciaddr: 0,
  yiaddr: 0,
  siaddr: 0,
  giaddr: 0,
  chaddr: [
    '8', '0',  '27', '24',
    '6', '42', '0',  '0',
    '0', '0',  '0',  '0',
    '0', '0',  '0',  '0'
  ],
  sname: '',
  file: '',
  vend: [
    '63', '82', '53', '63', '35', '1',  '1',  '39',
    '2',  '5',  'c0', '5d', '2',  '0',  '0',  '5e',
    '3',  '1',  '2',  '1',  '3c', '20', '50', '58',
    '45', '43', '6c', '69', '65', '6e', '74', '3a',
    '41', '72', '63', '68', '3a', '30', '30', '30',
    '30', '30', '3a', '55', '4e', '44', '49', '3a',
    '30', '30', '32', '30', '30', '31', '4d', '4',
    '69', '50', '58', '45', '37', '17', '1',  '3'
  ],
  vend_text: [
    'c',    '\x82', 'S',    'c',    
'5',    '\x01', '\x01',   '9',    


'\x02', '\x05', 'À',    ']',    '\x02', '\x00',
    '\x00', '^',    '\x03', '\x01', '\x02', '\x01', '<',
    ' ',    'P',    'X',    'E',    'C',    'l',    'i',
    'e',    'n',    't',    ':',    'A',    'r',    'c',
    'h',    ':',    '0',    '0',    '0',    '0',    '0',
    ':',    'U',    'N',    'D',    'I',    ':',    '0',
    '0',    '2',    '0',    '0',    '1',    'M',    '\x04',
    'i',    'P',    'X',    'E',    '7',    '\x17', '\x01',
    '\x03'
  ]
}

/*
			// 1024 max data
                 UINT8_t d[BOOTP_DHCPVEND]; /**< DHCP options * /
                 /** DHCP options * /
                 struct bootph_vendor_v {
                         /** DHCP magic cookie
                          *
                          * Should have the value #VM_RFC1048.
                          * /
                         UINT8_t magic[4];
                         UINT32_t flags; /**< BOOTP flags/opcodes * /
                         /** "End of BOOTP vendor extensions"
                          *
                          * Abandon hope, all ye who consider the
                          * purpose of this field.
                          * /
                         UINT8_t pad[56];
                 } v;

*/
			const yiaddr = 0x0aac0101;
			const op = 2;
			const file = "request_this_file";
			const siaddr = "me or tftp?";
			const giaddr = "gateway to get back to me";
						
			const reply = new Uint8Array( 300 );
			reply[0] = op;
			for( let n = 1; n < 10; n++ ) reply[n] = msg[n];

			msg[16] = (yiaddr >> 24)&0xFF;
			msg[17] = (yiaddr >> 16)&0xFF;
			msg[18] = (yiaddr >> 8)&0xFF;
			msg[19] = (yiaddr >> 0)&0xFF;

			

		} 
		console.log( "request:", fmsg );

	}
