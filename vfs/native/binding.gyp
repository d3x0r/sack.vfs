{
  "targets": [
    {
      "target_name": "sack_vfs",
      
      "sources": [ "vfs_module.cc",
           "<(sack_base)/src/utils/virtual_file_system/vfs.c"
          ]
    }
  ],
  'variables' : { 'sack_base%': 'M:/sack' },
  "target_defaults": {
  	'include_dirs': ['<(sack_base)/include']
  }
  
}
