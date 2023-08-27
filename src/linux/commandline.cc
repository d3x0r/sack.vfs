
#include "../global.h"

struct addArgs {
	const char *match;
	int matchlen;
	PLIST pArgs;
};

void addProc( uintptr_t psvUser, CTEXTSTR name, ScanFileProcessFlags flags ){
	static char buf[256];
	static char data[4096];
	struct addArgs* params = (struct addArgs*)psvUser;
	struct stat statbuf;
	if( lstat( name, &statbuf ) == 0 ) {
		if( S_ISLNK( statbuf.st_mode ) ) return;
	} else {
		int err = errno;
		lprintf( "Stat errno:%d", err );
	}

	snprintf( buf, 256, "%s/cmdline", name );
	//lprintf( "Trying:%s", buf );
	FILE *file = fopen( buf, "rb" );
	if( file ) {
		int bytes = fread( data, 1, 4096, file );
		if( !bytes ) {
			int err = errno;
			if( err == ENOTDIR ) {
				fclose( file );
				return;
			}else
				lprintf( "Read error? %d", err );
		}
		if( bytes == 4096 ) {
			lprintf( "long command line...");

		} else {
			if( !params->matchlen || ( StrCaseCmpEx( data, params->match, params->matchlen ) == 0 ) ) {
				int start = 0;
				int ofs = 0;
				int args = 0;
				const char *pid = pathrchr( name );

				pid++;
				while( ofs < bytes ) {
					if( data[ofs] == ' ' ) {
						while( data[ofs] == ' ' ) ofs++;
						args++;
					}
					if( !data[ofs++] ) args++;
				}
				struct command_line_result *cmd = NewArray( struct command_line_result, 1 );
				cmd->dwProcessId = atoi( pid );
				//if( cmd->dwProcessId < 500 )
				//	LogBinary( data, bytes );
				
				cmd->cmd = NewArray( char *, args );
				//lprintf( "------- Found %d %d args", cmd->dwProcessId, args );
				cmd->length = args;
				ofs = 0;
				args = 0;
				while( ofs < bytes ) {
					if( data[ofs] == ' ' || !data[ofs] ) {
						//lprintf( "Adding arg at %d %d %d", start, ofs, ofs-start );
						cmd->cmd[args++] = DupCStrLen( data+start, (ofs-start));
						//lprintf( "Set Arg %d %d %d %s %s", args-1, ofs, cmd->length, data+start, cmd->cmd[args-1] );
						
						if( !data[ofs] ) ofs++;
						else while( data[ofs] == ' ' ) ofs++;
						start = ofs;
					} 
					else ofs++;
				}

				AddLink( &params->pArgs, cmd );
			}
			//LogBinary( data, bytes );

		}
		fclose( file );
	} else {
		int err = errno;
		if( err == ENOTDIR ) {
			return;
		}
		if( err == ENOENT ) {
			return;
		}
		lprintf( "open errno:%s %d", name, err );
	}

}

PLIST GetProcessCommandLines( const char* process ) {
	struct addArgs params = {process, (int)( process?strlen( process ):0), NULL};
	POINTER info = NULL;
	while( ScanFiles( "/proc", "*", &info, addProc, (ScanFileFlags)0, (uintptr_t)&params ));
	return params.pArgs;
}

void ReleaseCommandLineResults( PLIST* ppResults ){
	INDEX idx;
	struct command_line_result *p;
	LIST_FORALL( ppResults[0], idx, struct command_line_result *, p )  {
		int n;
		for( n = 0; n < p->length; n++ )
			ReleaseEx( p->cmd[n] DBG_SRC );
		ReleaseEx( p->cmd DBG_SRC );
		ReleaseEx( p DBG_SRC );
	}
	DeleteList( ppResults );
}