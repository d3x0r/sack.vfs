{
  "targets": [
    {
      "target_name": "sack_vfs",
      
      "sources": [ "vfs_module.cc",
           "src/sack.cc",
           "src/sqlite3.c",
           "com_interface.cc",
           "sql_module.cc",
           "thread_module.cc",
           "jsonParse.cc",
           "tls_interface.cc",
          ],
	'defines': [
          'TARGETNAME="sack_vfs.node"',
           "__STATIC__","USE_SQLITE","USE_SQLITE_INTERFACE","FORCE_COLOR_MACROS",
           "NO_OPEN_MACRO","NO_FILEOP_ALIAS","JSON_PARSER_MAIN_SOURCE"
        ],
    'conditions': [
          ['OS=="linux"', {
            'defines': [
              '__LINUX__'
            ],
            'cflags_cc': ['-Wno-misleading-indentation','-Wno-parentheses','-Wno-unused-result'
			,'-Wno-char-subscripts'
			,'-Wno-empty-body','-Wno-format', '-Wno-address'
			, '-Wno-strict-aliasing', '-Wno-switch', '-Wno-missing-field-initializers' 
			, '-Wno-unused-variable', '-Wno-unused-function', '-Wno-unused-but-set-variable', '-Wno-maybe-uninitialized'
			, '-Wno-sign-compare', '-Wno-unknown-warning'
			],
            'cflags': ['-Wno-implicit-fallthrough'
			],
            'include_dirs': [
              'include/linux',
            ],
            'libraries':[ '-luuid' ]
          }],
	['node_shared_openssl=="false"', {
	      'include_dirs': [
	        '<(node_root_dir)/deps/openssl/openssl/include'
	      ],
		"conditions" : [
			["target_arch=='ia32'", {
			 "include_dirs": [ "<(node_root_dir)/deps/openssl/config/piii" ]
			}],
			["target_arch=='x64'", {
			 "include_dirs": [ "<(node_root_dir)/deps/openssl/config/k8" ]
			}],
			["target_arch=='arm'", {
			 "include_dirs": [ "<(node_root_dir)/deps/openssl/config/arm" ]
			}]
        	]	
	}],
	['OS=="mac"', {
            'defines': [
              '__LINUX__','__MAC__'
            ],
            'xcode_settings': {
                'OTHER_CFLAGS': [
                       '-Wno-self-assign', '-Wno-null-conversion', '-Wno-parentheses-equality', '-Wno-parentheses'
			,'-Wno-char-subscripts', '-Wno-null-conversion'
			,'-Wno-empty-body','-Wno-format', '-Wno-address'
			, '-Wno-strict-aliasing', '-Wno-switch', '-Wno-missing-field-initializers' 
			, '-Wno-unused-variable', '-Wno-unused-function'
			, '-Wno-sign-compare', '-Wno-null-dereference'
			, '-Wno-address-of-packed-member', '-Wno-unknown-warning-option'
                ],
             },
            'include_dirs': [
              'include/linux',
            ],
          }],
          ['OS=="win"', {
            'defines': [
              "NEED_SHLAPI","NEED_SHLOBJ","_CRT_SECURE_NO_WARNINGS"
            ],
            'sources': [
              # windows-only; exclude on other platforms.
              'reg_access.cc',
            ],
  	        'libraries':[ 'winmm', 'ws2_32', 'iphlpapi', 'rpcrt4', 'odbc32' ]
          }, { # OS != "win",
            'defines': [
              '__LINUX__',
            ],
          }]
        ],

	'otherDefinss': [ '__64__=1',
		"__NO_OPTIONS__","__NO_ODBC__" ],

    }
  ],

  "target_defaults": {
  	'include_dirs': ['src',    "<!(node -e \"require('nan')\")" ]
  }
  
}

