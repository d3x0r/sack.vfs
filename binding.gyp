{
  "targets": [
    {
      "target_name": "sack_vfs",
      
      "sources": [ "vfs_module.cc",
           "src/sack.cc",
           "src/sqlite3.c",
           "com_interface.cc",
           "sql_module.cc",
           "reg_access.cc",
           "thread_module.cc",
          ],
	'defines': [
          '__64__=1',
          'TARGETNAME="sack_vfs.node"',
           "__STATIC__","USE_SQLITE","USE_SQLITE_INTERFACE","FORCE_COLOR_MACROS","_CRT_SECURE_NO_WARNINGS","NEED_SHLAPI","NEED_SHLOBJ" 
                

        ],
	'otherDefinss': [ "__NO_OPTIONS__","__NO_INTERFACE_SUPPORT__","__NO_ODBC__" ],

	'libraries':[ 'winmm', 'ws2_32', 'iphlpapi', 'rpcrt4', 'odbc32' ]
    }
  ],
  "target_defaults": {
  	'include_dirs': ['src',    "<!(node -e \"require('nan')\")" ]
  }
  
}

