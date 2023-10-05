

import {sack} from "sack.vfs"


const pch = sack.Config();

			pch.add( "Auto Checkpoint=%b", SetAutoCheckpoint );
			pch.add( "Option DSN=%m", SetOptionDSN );
			pch.add( "Primary DSN=%m", SetPrimaryDSN );
			pch.add( "Primary User=%m", SetUser );
			pch.add( "Primary Connection String=%m", SetConnString );
			pch.add( "Primary Password=%m", SetPassword );
			pch.add( "Backup DSN=%m", SetBackupDSN );
			pch.add( "Backup User=%m", SetBackupUser );
			pch.add( "Backup Password=%m", SetBackupPassword );
			pch.add( "Log enable=%b", SetLoggingEnabled );
			pch.add( "LogFile enable=%b", SetLoggingEnabled2 );
			pch.add( "LogFile enable dump data=%b", SetLoggingEnabled3 );
			pch.add( "Log Option Connection=%b", SetLogOptions );
			pch.add( "Fallback on failure=%b", SetFallback );
			pch.add( "Require Connection=%b", SetRequireConnection );
			pch.add( "Require Primary Connection=%b", SetRequirePrimaryConnection );
			pch.add( "Require Backup Connection=%b", SetRequireBackupConnection );
			pch.add( "Database Init SQL=%m", AddDatabaseInit );
			pch.add( "Option Database Init SQL=%m", AddOptionDatabaseInit );

pch.go( "sql.config" );

function SetAutoCheckpoint( yesno ) {
	console.log( "SetAutoCheckpoint", yesno );
}

function SetOptionDSN( dsn ) {
	console.log( "Option DSN", dsn );
}

function SetPrimaryDSN( dsn ) {
	console.log( "Primary DSN", dsn );
}

function SetUser( dsn ) {
	console.log( "DSN User", dsn );
}

function SetConnString( dsn ) {
	console.log( "DSN Conn String", dsn );
}

function SetPassword( dsn ) {
	console.log( "DSN Password", dsn );
}

function SetBackupDSN( dsn ) {
	console.log( "Backup DSN", dsn );
}

function SetBackupUser( dsn ) {
	console.log( "Backup User", dsn );
}

function SetBackupPassword( dsn ) {
	console.log( "Backup Password", dsn );
}

function SetLoggingEnabled( yesno ) {
	console.log( "SetLoggingEnabled", yesno );
}

function SetLoggingEnabled2( yesno ) {
	console.log( "SetLoggingEnabled2", yesno );
}

function SetLoggingEnabled3( yesno ) {
	console.log( "SetLoggingEnabled3", yesno );
}

function SetLogOptions( yesno ) {
	console.log( "SetLogOptions", yesno );
}

function SetFallback( yesno ) {
	console.log( "SetFallback", yesno );
}

function SetRequireConnection( yesno ) {
	console.log( "SetRequireConnection", yesno );
}

function SetRequirePrimaryConnection( yesno ) {
	console.log( "SetRequirePrimaryConnection", yesno );
}

function SetRequireBackupConnection( yesno ) {
	console.log( "SetRequireBackupConnection", yesno );
}

function AddDatabaseInit( init ) {
	console.log( "SetDatabaseInit", init );
}

function AddOptionDatabaseInit( init ) {
	console.log( "SetOptionDatabaseInit", init );
}
