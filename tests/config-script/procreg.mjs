



import {sack} from "sack.vfs"

const pch = sack.Config();

		pch.add( "alias service %w %w", HandleAlias );

		pch.add( "Producer=%m", SetProducerName );
		pch.add( "Application=%m", SetApplicationName );
		pch.add( "enable trace=%b", SetTrace );
		pch.add( "option default %m=%m", SetOptionDefault );
		pch.add( "option set %m=%m", SetOptionSet );
		pch.add( "default option %m=%m", SetOptionDefault );
		pch.add( "set option %m=%m", SetOptionSet );
		pch.add( "start directory \"%m\"", SetDefaultDirectory );
		pch.add( "include \"%m\"", IncludeAdditional );
		pch.add( "if %m==%m", TestOption );
		pch.add( "endif", EndTestOption );
		pch.add( "else", ElseTestOption );

		pch.add( "service=%w library=%w load=%w unload=%w", HandleLibrary );
		pch.add( "alias service '%m' = '%m'", HandleAlias );
		pch.add( "module %w", HandleModule );
		pch.add( "pmodule %w", HandlePrivateModule );
		pch.add( "modulepath %m", HandleModulePath );

//sack.memDump();
//for( let i = 0; i < 100; i++ )
pch.go( "interface.conf" );
//sack.memDump();

function SetProducerName( name ) {
	console.log( "SetProducerName", name );
}

function SetApplicationName( name ) {
	console.log( "SetApplicationName", name );
}

function SetTrace( name ) {
	console.log( "SetTrace", name );
}

function SetOptionDefault( name, val ) {
	console.log( "SetOptionDefault", name, val );
}

function SetOptionSet( name, val ) {
	console.log( "SetOptionSet", name, val );
}

function IncludeAdditional( name ) {
	console.log( "IncludeAdditional", name );
}

function SetDefaultDirectory( name ) {
	console.log( "SetDefaultDirectory", name );
}

function TestOption( name, val ) {
	console.log( "TestOption", name, val );
}

function EndTestOption( ) {
	console.log( "EndTestOption" );
}

function ElseTestOption( ) {
	console.log( "ElseTestOption" );
}

function HandleLibrary( name, name2, name3, name4 ) {
	console.log( "HandleLibrary", name, name2, name3, name4 );
}

function HandleAlias( name, alias ) {
	console.log( "HandleAlias", name , alias );
}

function HandleModule( name ) {
	console.log( "HandleModule", name );
}

function HandlePrivateModule( name ) {
	console.log( "HandlePrivateModule", name );
}

function HandleModulePath( name ) {
	console.log( "HandleModulePath", name );
}

