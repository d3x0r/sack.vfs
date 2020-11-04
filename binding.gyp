{
  "targets": [
    {
      "target_name": "sack_vfs",
      'win_delay_load_hook': 'false',      
      "sources": [ "src/vfs_module.cc",
           "src/sack/sack.cc",
           "src/sack/sqlite3.c",
           "src/com_interface.cc",
           "src/sql_module.cc",
           "src/thread_module.cc",
           "src/jsonParse.cc",
           "src/jsoxParse.cc",
           "src/tls_interface.cc",
           "src/srg_module.cc",
           "src/websocket_module.cc",
           "src/network_module.cc",
           "src/task_module.cc",
           "src/config_module.cc",
           "src/objStore_module.cc",
           "src/fileMonitor_module.cc",
           "src/system_module.cc",
           "src/text_object.cc",
          ],
	'defines': [ "BUILDING_NODE_EXTENSION","BUILD_NODE_ADDON",
          'TARGETNAME="sack_vfs.node"'
           , "__STATIC__","__STATIC_GLOBALS__"
           , "DEFAULT_OUTPUT_STDERR"
           , "USE_SQLITE","USE_SQLITE_INTERFACE","FORCE_COLOR_MACROS",
           "NO_OPEN_MACRO","NO_FILEOP_ALIAS","JSON_PARSER_MAIN_SOURCE", "SQLITE_ENABLE_LOCKING_STYLE=0","SQLITE_THREADSAFE=0","SQLITE_OMIT_UTF16","SQLITE_ENABLE_COLUMN_METADATA=1", "SQLITE_DEFAULT_FOREIGN_KEYS=1", "NO_MIN_MAX_MACROS",
           "BUILD_NODE_ADDON"
        ],
    'conditions': [
          ['OS=="linux"', {
            'defines': [
              '__LINUX__', '__NO_ODBC__','__MANUAL_PRELOAD__','__INTERNAL_UUID__','UUID_SOURCE'
            ],
            'cflags_cc': ['-Wno-misleading-indentation','-Wno-parentheses','-Wno-unused-result'
			,'-Wno-char-subscripts'
			,'-Wno-empty-body','-Wno-format', '-Wno-address'
			, '-Wno-strict-aliasing', '-Wno-switch', '-Wno-missing-field-initializers' 
			, '-Wno-unused-variable', '-Wno-unused-function', '-Wno-unused-but-set-variable', '-Wno-maybe-uninitialized'
			, '-Wno-sign-compare', '-Wno-unknown-warning', '-fexceptions', '-Wno-cast-function-type'
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
              '__LINUX__','__MAC__', '__NO_ODBC__',"__NO_OPTIONS__",'_DARWIN_C_SOURCE'
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
			, '-Wno-unused-result', '-fexceptions', '-Wno-unknown-pragma'
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
            'configurations': {
              'Debug': {
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'BufferSecurityCheck': 'false',
                    'RuntimeTypeInfo': 'true',
                    'MultiProcessorCompilation' : 'true',
                    'InlineFunctionExpansion': 2,
                    'OmitFramePointers': 'true',
                    'ExceptionHandling':2

                  }
                }
              },
              'Release': {                            
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'BufferSecurityCheck': 'false',
                    'RuntimeTypeInfo': 'true',
                    'MultiProcessorCompilation' : 'true',
                    'InlineFunctionExpansion': 2,
                    'OmitFramePointers': 'true',
                    'ExceptionHandling':2
                  }
                }
              }
            },
            'sources': [
              # windows-only; exclude on other platforms.
              'src/hid_module.cc',
              'src/reg_access.cc',
              "src/playSound_module.cc",
             ],
  	        'libraries':[ 'winmm', 'ws2_32', 'iphlpapi', 'rpcrt4', 'odbc32', 'crypt32', 'cryptui' ]
          }, { # OS != "win",
            'defines': [
              '__LINUX__',
            ],
          }]
        ],

	# 'otherDefinss': [ "__NO_OPTIONS__" ],

    }
  ],

  "target_defaults": {
  	'include_dirs': ['src/sack']
  }
  
}

