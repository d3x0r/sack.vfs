// https://tools.ietf.org/html/rfc5280
//   §4.2.1.1 and §4.2.1.2. 
//  https://tools.ietf.org/html/rfc5280#section-4.2.1.2
//

// http://fm4dd.com/openssl/manual-apps/x509v3_config.htm (extension configuration docs)


#include "global.h"

const int serverKBits = 4096;
const int kBits = 1024;// 4096;
const int kExp = RSA_F4;

/*
  C
  ST
  L
  O
  OU
  CN
  GN  // given name
  SN  // surname
  initials 
  friendlyName
  name

  associatedName     associatedName
  associatedDomain     associatedDomain
  friendlyCountryName     friendlyCountryName
  buildingName     buildingName


  msUPN     Microsoft Universal Principal Name
  nameConstraints     X509v3 Name Constraints
  physicalDeliveryOfficeName     physicalDeliveryOfficeName
  distinguishedName     distinguishedName
  houseIdentifier     houseIdentifier

  jurisdictionL     jurisdictionLocalityName
  jurisdictionST     jurisdictionStateOrProvinceName
  jurisdictionC     jurisdictionCountryName

  unstructuredName
  emailAddress

  subjectAltName     X509v3 Subject Alternative Name
  issuerAltName     X509v3 Issuer Alternative Name

  crlNumber     X509v3 CRL Number

  nsSslServerName
*/

Persistent<Function> TLSObject::constructor;

struct optionStrings {
	Isolate *isolate;

	Persistent<String> *keyString;
	Persistent<String> *certString;
	Persistent<String> *passString;
	Persistent<String> *serialString;
	Persistent<String> *orgString;
	Persistent<String> *unitString;
	Persistent<String> *locString;
	Persistent<String> *countryString;
	Persistent<String> *stateString;
	Persistent<String> *commonString;
	Persistent<String> *pubkeyString;
	Persistent<String> *issuerString;
	Persistent<String> *expireString;
	Persistent<String> *requestString;
	Persistent<String> *signerString;
};

PLIST strings;

struct info_params {
	Isolate *isolate;
	char *country;
	int countrylen;
	char *state;
	int statelen;
	char *locality;
	int localitylen;
	char *org;
	int orglen;
	char *orgUnit;
	int orgUnitlen;
	char *common;
	int commonlen;
	char *key;
	int keylen;
	char *pubkey;
	int pubkeylen;
	int expire;
	char *issuer;
	int issuerlen;
	char *subject;
	int subjectlen;

	int64_t serial;
	char *password;
	int passlen;

	char *ca;
	int ca_len;

	char *cert;
	int certlen;
	char *chain;
	char *signingCert;
	char *signReq;
};


struct optionStrings *getStrings( Isolate *isolate ) {
	INDEX idx;
	struct optionStrings * check;
	LIST_FORALL( strings, idx, struct optionStrings *, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		check->isolate = isolate;
		check->keyString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "key" ) );
		check->certString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "cert" ) );
		check->passString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "password" ) );
		check->serialString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "serial" ) );
		check->orgString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "org" ) );
		check->unitString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "unit" ) );
		check->locString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "locality" ) );
		check->countryString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "country" ) );
		check->stateString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "state" ) );
		check->commonString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "name" ) );
		check->pubkeyString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "pubkey" ) );
		check->issuerString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "issuer" ) );
		check->expireString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "expire" ) );
		check->requestString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "request" ) );
		check->signerString = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "signer" ) );
		//check->String = new Persistent<String>( isolate, String::NewFromUtf8( isolate, "" ) );
		AddLink( &strings, check );
	}
	return check;
}

void TLSObject::Init( Isolate *isolate, Handle<Object> exports )
{
	Local<FunctionTemplate> tlsTemplate;

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();

	tlsTemplate = FunctionTemplate::New( isolate, New );
	tlsTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Volume" ) );
	tlsTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	//NODE_SET_PROTOTYPE_METHOD( tlsTemplate, "genkey", genKey );
	Local<Object> i = Object::New( isolate );
	//Local<Function> i = tlsTemplate->GetFunction();
	i->Set( String::NewFromUtf8( isolate, "seed" ), Function::New( isolate, seed ) );
	i->Set( String::NewFromUtf8( isolate, "genkey" ), Function::New( isolate, genKey ) );
	i->Set( String::NewFromUtf8( isolate, "gencert" ), Function::New( isolate, genCert ) ); // root cert
	i->Set( String::NewFromUtf8( isolate, "genreq" ), Function::New( isolate, genReq ) );  // cert signing req
	i->Set( String::NewFromUtf8( isolate, "pubkey" ), Function::New( isolate, pubKey ) ); // get public key of private
	i->Set( String::NewFromUtf8( isolate, "signreq" ), Function::New( isolate, signReq ) ); // sign cert
	i->Set( String::NewFromUtf8( isolate, "validate" ), Function::New( isolate, validate ) ); // validate a cert chain (returns true, or throws exception)
	i->Set( String::NewFromUtf8( isolate, "expiration" ), Function::New( isolate, expiration ) ); // get certificate expiration date
	//i->Set( String::NewFromUtf8( isolate, "certToString" ), Function::New( isolate, toString ) ); // dump cert to text string

	//constructor.Reset( isolate, tlsTemplate->GetFunction() );
	exports->Set( String::NewFromUtf8( isolate, "TLS" ),
		i );//tlsTemplate->GetFunction() );

}

TLSObject::TLSObject()
{
}


TLSObject::~TLSObject()
{
}

static void flushErrors(void) {
	int err;
	char buf[256];
	//lprintf( "----- FLUSH ERRORS ------" );
	while( err = ERR_get_error() ) {
		ERR_error_string_n( err, buf, 256 );
		lprintf( "OUtstanding SSL Error:%s", buf );
	}
}

static void _throwError( struct info_params *params, const char *message ) {
	PVARTEXT pvt = VarTextCreate();
	char buf[256];
	int err;
	VarTextAddData( pvt, message, VARTEXT_ADD_DATA_NULTERM );
	VarTextAddData( pvt, "\n", 1 );
	while( err = ERR_get_error() ) {
		ERR_error_string_n( err, buf, 256 );
		//lprintf( "...%s", buf );
		VarTextAddData( pvt, buf, VARTEXT_ADD_DATA_NULTERM );
		VarTextAddData( pvt, "\n", 1 );
	}
	PTEXT result = VarTextPeek( pvt );
	params->isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( params->isolate, GetText( result ) ) ) );
	VarTextDestroy( &pvt );
}

static void throwError( struct info_params *params, const char *message ) {
	_throwError( params, TranslateText( message ) );
}

void TLSObject::seed( const v8::FunctionCallbackInfo<Value>& args ) {
	int argc = args.Length();
	if( argc > 0 ) {
		if( args[0]->IsUint8Array() )
		{
			Local<Uint8Array> arr = args[0].As<Uint8Array>();
			RAND_seed( *arr, (int)arr->Length() );
		}
		else {
			String::Utf8Value val( args[0]->ToString() );
			RAND_seed( *val, val.length() );
		}
	}
}

static EVP_PKEY *genKey( int kBits ) {
	EVP_PKEY *keypair = EVP_PKEY_new();
	//int keylen;
	//char *pem_key;
	//BN_GENCB cb = { }
	BIGNUM          *bne = BN_new();
	RSA *rsa = RSA_new();
	int ret;
	ret = BN_set_word( bne, kExp );
	if( ret != 1 ) {
		BN_free( bne );
		RSA_free( rsa );
		EVP_PKEY_free( keypair );
		return NULL;
	}
	RSA_generate_key_ex( rsa, kBits, bne, NULL );
	EVP_PKEY_set1_RSA( keypair, rsa );

	BN_free( bne );
	RSA_free( rsa );

	return keypair;
}

void TLSObject::New( const v8::FunctionCallbackInfo<Value>& args  ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( args.IsConstructCall() ) {
		TLSObject* obj;
		obj = new TLSObject( );
		obj->Wrap( args.This() );

		args.GetReturnValue().Set( args.This() );
	} else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		Local<Value> *argv = new Local<Value>[0];
		Local<Function> cons = Local<Function>::New( isolate, constructor );
		MaybeLocal<Object> mo = Nan::NewInstance( cons, 0, argv );
		if( !mo.IsEmpty() )
			args.GetReturnValue().Set( mo.ToLocalChecked() );
		delete[] argv;
	}
}

void TLSObject::genKey( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	int keylen = 1024;
	char *keypass = NULL;
	size_t keypasslen = 0;
	String::Utf8Value *pass = NULL;
	if( argc ) {
		keylen = (int)args[0]->IntegerValue();
		if( argc > 1 ) {
  			pass = new String::Utf8Value( args[1]->ToString() );
			keypass = *pass[0];
			keypasslen = pass->length();
		}
	}
	EVP_PKEY *key = ::genKey( keylen );
	if( !key ) return;
	{
		BIO *keybuf = BIO_new( BIO_s_mem() );
		size_t keybuflen;
		//int r2 = PEM_write_bio_PUBKEY( keybuf, key );
		int r = PEM_write_bio_PKCS8PrivateKey( keybuf, key, keypass?EVP_aes_256_cbc():NULL, keypass, (int)keypasslen, NULL, NULL );
		//int r = PEM_write_bio_PrivateKey( keybuf, key, EVP_aes_256_cbc(), (unsigned char*)keypass, (int)keypasslen, NULL, NULL );
		if( !r )
		{	
			struct info_params params;
			params.isolate = isolate;
			throwError( &params, "genkey: error encoding private key" );
			if( keypass ) Deallocate( char*, keypass );
			return;
		}
		keybuflen = BIO_pending( keybuf );
		char *keyoutbuf = NewArray( char, keybuflen + 1 );
		BIO_read( keybuf, keyoutbuf, (int)keybuflen );
		((char*)keyoutbuf)[keybuflen] = 0;

		MaybeLocal<String> retval = String::NewFromUtf8( isolate, keyoutbuf, NewStringType::kNormal, (int)keybuflen );
		args.GetReturnValue().Set( retval.ToLocalChecked() );

		Deallocate( char*, keyoutbuf );
		BIO_free_all(keybuf);
	}
}

int pem_password(char *buf, int size, int rwflag, void *u)
{
// rwflag = 0  = used for decryption
// rwflag = 1  = used for encryption
	struct info_params *params = (struct info_params*)u;
	int len;
	memcpy( buf, params->password, len = (size<params->passlen?size:params->passlen) );

	return len;
}

void MakeCert(  struct info_params *params )
{ 
	X509 * x509=NULL;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY *pubkey = NULL;
	params->ca = NULL;  // init no result

	BIO *keybuf = BIO_new( BIO_s_mem() );

	// Create evp obj to hold our rsakey 
	// 
	BIO_write( keybuf, params->key, params->keylen );
	PEM_read_bio_PrivateKey( keybuf, &pkey, params->password?pem_password:NULL, params->password?params:NULL );
	if( !pkey ) {
		throwError( params, "gencert : bad key or password." );
		goto free_all;
	}

	if( params->pubkey ) {
		BIO_ctrl(keybuf,BIO_CTRL_RESET,0,NULL);
		//BIO_reset( keybuf );
		BIO_write( keybuf, params->pubkey, params->pubkeylen );
		PEM_read_bio_PUBKEY( keybuf, &pubkey, NULL, NULL );
		if( !pubkey ) {
			throwError( params, "gencert : bad pubkey specified." );
			goto free_all;
		}
	}
	else
		pubkey = pkey;
	{
		x509 = X509_new();
		X509_set_version( x509, 2 ); // wikipedia says 0 is what is normal. (0=1, 1=2, 2=v3! )

		ASN1_INTEGER_set( X509_get_serialNumber( x509 ), (long)params->serial );
		X509_gmtime_adj( X509_get_notBefore( x509 ), 0 );
		// (60 seconds * 60 minutes * 24 hours * 365 days) = 31536000.
		X509_gmtime_adj( X509_get_notAfter( x509 ), params->expire?params->expire*(60*60*24):31536000 );
		X509_set_pubkey( x509, pubkey );
		{
			X509_NAME *name = X509_get_subject_name( x509 );

			X509_NAME_add_entry_by_txt( name, "C", MBSTRING_UTF8,
				(unsigned char *)params->country, params->countrylen, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "ST", MBSTRING_UTF8,
				(unsigned char *)params->state, params->statelen, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "L", MBSTRING_UTF8,
				(unsigned char *)params->locality, params->localitylen, -1, 0 );
			if( params->orgUnit )
				X509_NAME_add_entry_by_txt( name, "OU", MBSTRING_UTF8,
					(unsigned char *)params->orgUnit, params->orgUnitlen, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "O", MBSTRING_UTF8,
				(unsigned char *)params->org, params->orglen, -1, 0 );

			X509_NAME_add_entry_by_txt( name, "CN", MBSTRING_UTF8,
				(unsigned char *)params->common, params->commonlen, -1, 0 );

			// this is a self-signing part.
			X509_set_issuer_name( x509, name );

			//X509v3_add_ext( );
			//X509V3_EXT_add
				//X509V3_string_free
			/*
			[v3_ca]
			# Extensions for a typical CA( `man x509v3_config`).
				subjectKeyIdentifier = hash
				authorityKeyIdentifier = keyid:always, issuer
				basicConstraints = [critical,] CA : true
				keyUsage = [critical,] digitalSignature, cRLSign, keyCertSign

				NID_netscape_cert_type, // 71 
				NID_key_usage,		// 83 
				NID_subject_alt_name,	// 85 
				NID_basic_constraints,	// 87 
				NID_certificate_policies, // 89 
				NID_ext_key_usage,	// 126 
				NID_policy_constraints,	// 401 
				NID_proxyCertInfo,	// 663 
				NID_name_constraints,	// 666 
				NID_policy_mappings,	// 747 
				NID_inhibit_any_policy	// 748 

				# define KU_DIGITAL_SIGNATURE    0x0080
				# define KU_NON_REPUDIATION      0x0040
				# define KU_KEY_ENCIPHERMENT     0x0020
				# define KU_DATA_ENCIPHERMENT    0x0010
				# define KU_KEY_AGREEMENT        0x0008
				# define KU_KEY_CERT_SIGN        0x0004
				# define KU_CRL_SIGN             0x0002
				# define KU_ENCIPHER_ONLY        0x0001
				# define KU_DECIPHER_ONLY        0x8000
				*/
			{
				X509_EXTENSION *ext = NULL;// X509_get_ext();
				//ASN1_OCTET_STRING *data;
				{
					ASN1_STRING *data = ASN1_STRING_new();
					char *r = params->issuer?params->issuer:SRG_ID_Generator();
					ASN1_STRING_set( data, r, (int)strlen( r ) );
					X509_add1_ext_i2d( x509, NID_subject_key_identifier, data, 0, X509V3_ADD_DEFAULT );

					AUTHORITY_KEYID kid;
					kid.keyid = data;
					kid.serial = NULL;
					kid.issuer = NULL;
					X509_add1_ext_i2d( x509, NID_authority_key_identifier, &kid, 0, X509V3_ADD_DEFAULT );
				}
				{
					BASIC_CONSTRAINTS bc;
					bc.ca = 1;
					bc.pathlen = NULL;
					X509_add1_ext_i2d( x509, NID_basic_constraints, &bc, 1, X509V3_ADD_DEFAULT );
				}
				{
					int _usage = KU_CRL_SIGN | KU_KEY_CERT_SIGN | KU_DIGITAL_SIGNATURE;
					ASN1_INTEGER *usage = ASN1_INTEGER_new();
					ASN1_INTEGER_set( usage, _usage );
					X509_add1_ext_i2d( x509, NID_key_usage, usage, 1, X509V3_ADD_DEFAULT );
				}
			}

			if( X509_sign( x509, pkey, EVP_sha512() ) < 1 )
			{
				throwError( params, "gencert: signing failed." );
				goto free_all;
			}

			{
				PEM_write_bio_X509( keybuf, x509 );
				params->ca_len = BIO_pending( keybuf );
				params->ca = NewArray( char, params->ca_len + 1 );
				BIO_read( keybuf, params->ca, params->ca_len );
				params->ca[params->ca_len] = 0;
			}
		}
	}
free_all:
	if( pubkey != pkey )
		EVP_PKEY_free( pubkey );
	EVP_PKEY_free( pkey );
	X509_free( x509 );
	BIO_free_all(keybuf);
} 


void TLSObject::genCert( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;
	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter.") ) ) );
		return;
	}
	params.isolate = isolate;

	struct optionStrings *strings = getStrings( isolate );

	Local<Object> opts = args[0]->ToObject();

	Local<String> optName;
	if( !opts->Has( optName = strings->countryString->Get(isolate) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'country'.") ) ) );
		return;
	}
	Local<Object> country = opts->Get( optName )->ToObject();
	String::Utf8Value _country( country->ToString() );
	params.country = *_country;
	params.countrylen = _country.length();

	if( !opts->Has( optName = strings->stateString->Get(isolate) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'state' or province.") ) ) );
		return;
	}
	Local<Object> state = opts->Get( optName )->ToObject();
	String::Utf8Value _state( state->ToString() );
	params.state = *_state;
	params.statelen = _state.length();

	if( !opts->Has( optName = strings->locString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'locality'(city).") ) ) );
		return;
	}
	Local<Object> locality = opts->Get( optName )->ToObject();
	String::Utf8Value _locality( locality->ToString() );
	params.locality = *_locality;
	params.localitylen = _locality.length();

	if( !opts->Has( optName = strings->orgString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'org'.") ) ) );
		return;
	}
	Local<Object> org = opts->Get( optName )->ToObject();
	String::Utf8Value _org( org->ToString() );
	params.org = *_org;
	params.orglen = _org.length();

	Local<String> unitString = strings->unitString->Get( isolate );
	String::Utf8Value *_unit;
	if( !opts->Has( unitString ) ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'.") ) ) );
		//return;
		_unit = NULL;
		params.orgUnit = NULL;
	}else {
		Local<Object> unit = opts->Get( unitString )->ToObject();
		_unit = new String::Utf8Value( unit->ToString() );
		params.orgUnit = *_unit[0];
		params.orgUnitlen = _unit[0].length();
	}

	if( !opts->Has( optName = strings->commonString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'name'.") ) ) );
		return;
	}
	Local<Object> common = opts->Get( optName )->ToObject();
	String::Utf8Value _common( common->ToString() );
	params.common = *_common;
	params.commonlen = _common.length();

	if( !opts->Has( optName = strings->keyString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'key'.") ) ) );
		return;
	}
	Local<Object> key = opts->Get( optName )->ToObject();
	String::Utf8Value _key( key->ToString() );
	params.key = *_key;
	params.keylen = _key.length();

	Local<String> pubkeyString = strings->pubkeyString->Get( isolate );// String::NewFromUtf8( isolate, "pubkey" );
	String::Utf8Value *_pubkey = NULL;

	if( opts->Has( pubkeyString ) ) {
		Local<Object> key = opts->Get( pubkeyString )->ToObject();
		_pubkey = new String::Utf8Value( key->ToString() );
		params.pubkey = *_pubkey[0];
		params.pubkeylen = _pubkey[0].length();
	}
	else
		params.pubkey = NULL;


	Local<String> serialString = strings->serialString->Get( isolate );// String::NewFromUtf8( isolate, "serial" );
	if( !opts->Has( serialString ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'serial'.") ) ) );
		return;
	}
	else {
		Local<Integer> serial = opts->Get( serialString )->ToInteger();
		params.serial = serial->Value();
		if( params.serial < 1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial' must be more than 0." ) ) ) );
			return;
		}
	}

	Local<String> issuerString = strings->issuerString->Get( isolate );//String::NewFromUtf8( isolate, "issuer" );
	String::Utf8Value *_issuer;
	if( !opts->Has( issuerString ) ) {
		_issuer = NULL;
		params.issuer = NULL;
	}else {
		Local<Object> issuer = opts->Get( issuerString )->ToObject();
		_issuer = new String::Utf8Value( issuer->ToString() );
		params.issuer = *_issuer[0];
		params.issuerlen = _issuer[0].length();
	}

	Local<String> expireString = strings->expireString->Get( isolate );
	if( !opts->Has( expireString ) ) {
		params.expire = 0;
	}
	else {
		Local<Integer> expire = opts->Get( expireString )->ToInteger();
		params.expire = (int)expire->Value();
	}

	
	Local<String> passString = strings->passString->Get( isolate );;

	String::Utf8Value *_password = NULL;
	if( !opts->Has( passString ) ) {
		params.password = NULL;
	}
	else {
		Local<Object> password = opts->Get( passString )->ToObject();
		_password = new String::Utf8Value( password->ToString() );
		params.password = *_password[0];
		params.passlen = _password[0].length();
	}

	MakeCert( &params );
	if( params.ca ) {
		MaybeLocal<String> retval = String::NewFromUtf8( isolate, params.ca, NewStringType::kNormal, (int)params.ca_len );
		args.GetReturnValue().Set( retval.ToLocalChecked() );
		Deallocate( char *, params.ca );
	}

	if( _pubkey ) delete _pubkey;
	if( _issuer ) delete _issuer;
	if( _unit ) delete _unit;
	if( _password ) delete _password;

}


void MakeReq( struct info_params *params )
{ 
	int ca_len;
	char* ca;

	X509_REQ *req = NULL;
	//X509 * x509;
	EVP_PKEY *pkey = NULL;
	params->ca = NULL;

	BIO *keybuf = BIO_new( BIO_s_mem() );

	BIO_write( keybuf, params->key, params->keylen );
	PEM_read_bio_PrivateKey( keybuf, &pkey, params->password?pem_password:NULL, params->password?params:NULL );
	if( !pkey ) {
		throwError( params, "genreq : bad key or password." );
		goto free_all;
	}

	// seed openssl's prng 
	// 
	/* 
	if (RAND_load_file("/dev/random", -1)) 
		  fatal("Could not seed prng"); 
	*/ 
	// Generate the RSA key; we don't assign a callback to monitor progress 
	// since generating keys is fast enough these days 
	// 

	{
		req = X509_REQ_new();
		X509_REQ_set_version(req, 2); // wikipedia says 0 is what is normal. (0=1, 1=2, 2=v3! )
		//ASN1_INTEGER_set( X509_REQ_get_serialNumber( req ), params->serial );
		//X509_gmtime_adj( X509_get_notBefore( req ), 0 );
		// (60 seconds * 60 minutes * 24 hours * 365 days) = 31536000.
		//X509_REQ_gmtime_adj( X509_get_notAfter( req ), 31536000L );


		X509_REQ_set_pubkey( req, pkey );
		{
			X509_NAME *name = X509_REQ_get_subject_name( req );

			X509_NAME_add_entry_by_txt( name, "C", MBSTRING_UTF8,
				(unsigned char *)params->country, params->countrylen, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "ST", MBSTRING_UTF8,
				(unsigned char *)params->state, params->statelen, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "L", MBSTRING_UTF8,
				(unsigned char *)params->locality, params->localitylen, -1, 0 );
			if( params->orgUnit )
				X509_NAME_add_entry_by_txt( name, "OU", MBSTRING_UTF8,
					(unsigned char *)params->orgUnit, params->orgUnitlen, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "O", MBSTRING_UTF8,
				(unsigned char *)params->org, params->orglen, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "CN", MBSTRING_UTF8,
				(unsigned char *)params->common, params->commonlen, -1, 0 );

			if( X509_REQ_sign( req, pkey, EVP_sha256() ) < 0 )
			{
				throwError( params, "genreq:Signing failed." );
				goto free_all;
			}

			{
				PEM_write_bio_X509_REQ_NEW( keybuf, req );
				ca_len = BIO_pending( keybuf );
				ca = NewArray( char, ca_len + 1 );

				BIO_read( keybuf, ca, ca_len );
				ca[ca_len] = 0;

				params->ca = ca;
				params->ca_len = ca_len;
			}

		}
	}
free_all:
	EVP_PKEY_free( pkey );
	X509_REQ_free( req );
	BIO_free_all(keybuf);
} 


void TLSObject::genReq( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;
	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter.") ) ) );
		return;
	}
	params.isolate = isolate;

	struct optionStrings *strings = getStrings( isolate );
	Local<Object> opts = args[0]->ToObject();

	Local<String> optName;

	if( !opts->Has( optName = strings->countryString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'country'.") ) ) );
		return;
	}
	Local<Object> country = opts->Get( optName )->ToObject();
	String::Utf8Value _country( country->ToString() );
	params.country = *_country;
	params.countrylen = _country.length();

	if( !opts->Has( optName = strings->stateString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'state' or province.") ) ) );
		return;
	}
	String::Utf8Value _state( opts->Get( optName )->ToString() );
	params.state = *_state;
	params.statelen = _state.length();

	if( !opts->Has( optName = strings->locString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'locality'(city).") ) ) );
		return;
	}
	String::Utf8Value _locality( opts->Get( optName )->ToString() );
	params.locality = *_locality;
	params.localitylen = _locality.length();

	if( !opts->Has( optName = strings->orgString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'org'.") ) ) );
		return;
	}
	String::Utf8Value _org( opts->Get( optName )->ToString() );
	params.org = *_org;
	params.orglen = _org.length();

	String::Utf8Value *_unit;
	if( !opts->Has( optName = strings->unitString->Get( isolate ) ) ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'.") ) ) );
		//return;
		_unit = NULL;
		params.orgUnit = NULL;
	}else {
		_unit = new String::Utf8Value( opts->Get( optName )->ToString() );
		params.orgUnit = *_unit[0];
		params.orgUnitlen = _unit[0].length();
	}

	if( !opts->Has( optName = strings->commonString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'name'.") ) ) );
		return;
	}
	String::Utf8Value _common( opts->Get( optName )->ToString() );
	params.common = *_common;
	params.commonlen = _common.length();

	/*
	Local<String> serialString = String::NewFromUtf8( isolate, "serial" );
	if( !opts->Has( serialString ) ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial'." ) ) ) );
		return;
	}
	else {
		Local<Integer> serial = opts->Get( serialString )->ToInteger();
		params.serial = serial->Value();
		if( params.serial < 1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial' must be more than 0." ) ) ) );
			return;
		}
	}
	*/

	Local<String> keyString = strings->keyString->Get( isolate );
	if( !opts->Has( keyString ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'key'.") ) ) );
		return;
	}
	Local<Object> key = opts->Get( keyString )->ToObject();
	String::Utf8Value _key( key->ToString() );
	params.key = *_key;
	params.keylen = _key.length();

	Local<String> passString = strings->passString->Get( isolate );

	String::Utf8Value *_password = NULL;
	if( !opts->Has( passString ) ) {
		params.password = NULL;
	}
	else {
		Local<Object> password = opts->Get( passString )->ToObject();
		_password = new String::Utf8Value( password->ToString() );
		params.password = *_password[0];
		params.passlen = _password[0].length();
	}

	MakeReq( &params );
	if( params.ca ) {
		MaybeLocal<String> retval = String::NewFromUtf8( isolate, params.ca, NewStringType::kNormal, (int)params.ca_len );
		args.GetReturnValue().Set( retval.ToLocalChecked() );		
		Deallocate( char *, params.ca );
	}
	if( _unit ) delete _unit;	
	if( _password ) delete _password;
}


static void SignReq( struct info_params *params )
{
	int ret;

	X509_REQ *req = NULL;
	X509 * x509 = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY *pubkey = NULL;
	params->ca = NULL;

	BIO *keybuf = BIO_new( BIO_s_mem() );

	BIO_write( keybuf, params->key, (int)strlen( params->key ) );
	PEM_read_bio_PrivateKey( keybuf, &pkey, params->password?pem_password:NULL, params->password?params:NULL );
	if( !pkey ){
		throwError( params, "signreq: bad key or password" );
		goto free_all;
	}

	BIO_write( keybuf, params->signReq, (int)strlen( params->signReq ) );
	if( !PEM_read_bio_X509_REQ( keybuf, &req, NULL, NULL ) ) {
		throwError( params, "signreq: failed to parse request" );
		goto free_all;
	}

	BIO_write( keybuf, params->signingCert, (int)strlen( params->signingCert ) );
	if( !PEM_read_bio_X509( keybuf, &x509, NULL, NULL )) {
		throwError( params, "signreq: failed to parse signing cert" );
		goto free_all;
	}

	if( params->pubkey ) {
		BIO_ctrl(keybuf,BIO_CTRL_RESET,0,NULL);
		//BIO_reset( keybuf );
		BIO_write( keybuf, params->pubkey, params->pubkeylen );
		PEM_read_bio_PUBKEY( keybuf, &pubkey, NULL, NULL );
		if( !pubkey ) {
			throwError( params, "signreq : bad pubkey specified." );
			goto free_all;
		}
	}
	else
		pubkey = X509_REQ_get_pubkey( req );;

	X509* cert;
	cert = X509_new();
	// set version to X509 v3 certificate
	if (!X509_set_version(cert,2)) {
		throwError( params, "signreq: failed set version" );
		goto free_all;
 
	}

	// set serial
	ASN1_INTEGER_set(X509_get_serialNumber(cert), (long)params->serial);

	// set issuer name frome ca
	if (!X509_set_issuer_name(cert, X509_get_subject_name(x509))){
		throwError( params, "signreq: failed to set issuer" );
		goto free_all;
	}
	
	// set time
	X509_gmtime_adj(X509_get_notBefore(cert), 0);
	// (60 seconds * 60 minutes * 24 hours * 365 days) = 31536000.
	X509_gmtime_adj(X509_get_notAfter(cert), params->expire?params->expire*(60*60*24):31536000);
 
	// set subject from req
	X509_NAME *subject, *tmpname;
	tmpname = X509_REQ_get_subject_name(req);
	subject = X509_NAME_dup(tmpname);
	if (!X509_set_subject_name(cert, subject)){
		throwError( params, "signreq: failed to set subject" );
		goto free_all;
	}
 
	ret = X509_set_pubkey(cert, pubkey);
	//EVP_PKEY_free(pktmp);
	if (!ret)  {
		throwError( params, "signreq: Failed to set public key" );
		goto free_all;
 	}

	{
		{
			ASN1_STRING *data = ASN1_STRING_new();
			char *r = params->subject?params->subject:SRG_ID_Generator();
			ASN1_STRING_set( data, r, (int)strlen( r ) );
			X509_add1_ext_i2d( cert, NID_subject_key_identifier, data, 0, X509V3_ADD_DEFAULT );

			AUTHORITY_KEYID kid;
			ASN1_STRING *subj;
			if( !params->issuer )
				subj = (ASN1_STRING *)X509_get_ext_d2i( x509, NID_subject_key_identifier, NULL, NULL );
			else {
				subj = ASN1_STRING_new();
				ASN1_STRING_set( subj, params->issuer, params->issuerlen );
			}
			kid.keyid = subj;
			kid.serial = X509_get_serialNumber( x509 );
			kid.issuer = NULL;
			X509_add1_ext_i2d( cert, NID_authority_key_identifier, &kid, 0, X509V3_ADD_DEFAULT );
		}
		{
			BASIC_CONSTRAINTS *sbc = (BASIC_CONSTRAINTS *)X509_get_ext_d2i( x509, NID_basic_constraints, NULL, NULL );
			if( !sbc ) {
				throwError( params, "signcert: signing cert is not a CA" );
				goto free_all;
			}
			long pathlen;
			pathlen = sbc->pathlen ? ASN1_INTEGER_get( sbc->pathlen ) : 1;
			if( pathlen > 0 ) {
				BASIC_CONSTRAINTS bc;
				bc.ca = 1;
				bc.pathlen = ASN1_INTEGER_new();;
				ASN1_INTEGER_set( bc.pathlen, pathlen - 1 );
				X509_add1_ext_i2d( cert, NID_basic_constraints, &bc, 1, X509V3_ADD_DEFAULT );
				{
					int _usage = KU_CRL_SIGN | KU_KEY_CERT_SIGN | KU_DIGITAL_SIGNATURE;
					ASN1_INTEGER *usage = ASN1_INTEGER_new();
					ASN1_INTEGER_set( usage, _usage );
					X509_add1_ext_i2d( cert, NID_key_usage, usage, 1, X509V3_ADD_DEFAULT );
				}
			}
		}
	}


	// sign cert
	if (!X509_sign(cert, pkey, EVP_sha256())) {
		throwError( params, "signreq: signing failed" );
		goto free_all;
	}
 
	PEM_write_bio_X509(keybuf, cert);
	params->ca_len = BIO_pending( keybuf );
	params->ca = NewArray( char, params->ca_len + 1 );
	BIO_read( keybuf, params->ca, params->ca_len );
	params->ca[params->ca_len] = 0;


free_all:
	X509_free(cert);
	BIO_free_all(keybuf);
 
	X509_REQ_free(req);
	X509_free(x509);
	if( pubkey != pkey )
		EVP_PKEY_free( pubkey );
	EVP_PKEY_free(pkey);

}

void TLSObject::signReq( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;
	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter.") ) ) );
		return;
	}
	params.isolate = isolate;

	struct optionStrings *strings = getStrings( isolate );

	Local<Object> opts = args[0]->ToObject();
	Local<String> optName;

	if( !opts->Has( optName = strings->signerString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'signer'.") ) ) );
		return;
	}
	String::Utf8Value _signingCert( opts->Get( optName )->ToString() );
	params.signingCert = *_signingCert;

	if( !opts->Has( optName = strings->requestString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'request'.") ) ) );
		return;
	}
	String::Utf8Value _request( opts->Get( optName )->ToString() );
	params.signReq = *_request;

	Local<String> expireString = strings->expireString->Get( isolate );
	if( !opts->Has( expireString ) ) {
		params.expire = 0;
	}
	else {
		Local<Integer> expire = opts->Get( expireString )->ToInteger();
		params.expire = (int)expire->Value();
	}

	if( !opts->Has( optName = strings->keyString->Get( isolate ) ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'key'.") ) ) );
		return;
	}
	String::Utf8Value _key( opts->Get( optName )->ToString() );
	params.key = *_key;
	params.keylen = _key.length();

	Local<String> serialString = strings->serialString->Get( isolate );

	if( !opts->Has( serialString ) ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial'." ) ) ) );
		return;
	}
	else {
		Local<Integer> serial = opts->Get( serialString )->ToInteger();
		params.serial = serial->Value();
		if( params.serial < 1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial' must be more than 0." ) ) ) );
			return;
		}
	}

	Local<String> issuerString = String::NewFromUtf8( isolate, "issuer" );
	Local<String> subjectString = String::NewFromUtf8( isolate, "subject" );

	String::Utf8Value *_issuer;
	if( !opts->Has( issuerString ) ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'.") ) ) );
		//return;
		_issuer = NULL;
		params.issuer = NULL;
	}
	else {
		Local<Object> issuer = opts->Get( issuerString )->ToObject();
		_issuer = new String::Utf8Value( issuer->ToString() );
		params.issuer = *_issuer[0];
		params.issuerlen = _issuer[0].length();
	}

	String::Utf8Value *_subject;
	if( !opts->Has( subjectString ) ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'.") ) ) );
		//return;
		_subject = NULL;
		params.subject = NULL;
	}
	else {
		Local<Object> subject = opts->Get( String::NewFromUtf8( isolate, "subject" ) )->ToObject();
		_subject = new String::Utf8Value( subject->ToString() );
		params.subject = *_subject[0];
		params.subjectlen = _subject[0].length();
	}

	Local<String> pubkeyString = strings->pubkeyString->Get( isolate );// String::NewFromUtf8( isolate, "pubkey" );
	String::Utf8Value *_pubkey = NULL;

	if( opts->Has( pubkeyString ) ) {
		lprintf( "using custom public key..." );
		Local<Object> key = opts->Get( pubkeyString )->ToObject();
		_pubkey = new String::Utf8Value( key->ToString() );
		params.pubkey = *_pubkey[0];
		params.pubkeylen = _pubkey[0].length();
	}
	else
		params.pubkey = NULL;

	Local<String> passString = strings->passString->Get( isolate );;

	String::Utf8Value *_password = NULL;
	if( !opts->Has( passString ) ) {
		params.password = NULL;
	}
	else {
		Local<Object> password = opts->Get( passString )->ToObject();
		_password = new String::Utf8Value( password->ToString() );
		params.password = *_password[0];
		params.passlen = _password[0].length();
	}

	SignReq( &params );
	if( params.ca ) {
		MaybeLocal<String> retval = String::NewFromUtf8( isolate, params.ca, NewStringType::kNormal, (int)params.ca_len );
		args.GetReturnValue().Set( retval.ToLocalChecked() );
		Deallocate( char *, params.ca );
		
	}
	if( _issuer ) delete _issuer;
	if( _subject ) delete _subject;
	if( _password ) delete _password;
}



static void PubKey( struct info_params *params ) {

	params->ca = NULL;

	BIO *keybuf = BIO_new( BIO_s_mem() );

	EVP_PKEY *pkey;
	X509 *x509 = NULL;
	if( params->key ) {
		BIO_write( keybuf, params->key, (int)strlen( params->key ) );
		//lprintf( "key:%s, pass:%s", params->key, params->password );
		pkey = PEM_read_bio_PrivateKey( keybuf, NULL, params->password?pem_password:NULL, params->password?params:NULL );
		if( !pkey ){
			throwError( params, "pubkey: bad key or password" );
			goto free_all;
		}
	} else {
		BIO_write( keybuf, params->cert, (int)strlen( params->cert ) );
		x509 = PEM_read_bio_X509( keybuf, &x509, NULL, NULL );
		if( !x509 ) {
			throwError( params, "pubkey: bad certificate" );
			goto free_all;
		}
		pkey = X509_get_pubkey( x509 );
		if( !pkey ) {
			throwError( params, "pubkey: certificate fialed to get pubkey" );
				goto free_all;
		}
	}
/*
	int r2 = PEM_write_bio_PKCS8PrivateKey( keybuf, pkey, NULL, NULL, 0, NULL, NULL );
	if( !r2 )
	{	
		lprintf( "no private key?" );
	}
*/	
	int r;
	r = PEM_write_bio_PUBKEY( keybuf, pkey );
	if( !r )
	{	
		throwError( params, "genkey: error writing public key" );
		goto free_all;
	}
	
	params->ca_len = BIO_pending( keybuf );
	params->ca = NewArray( char, params->ca_len + 1 );
	BIO_read( keybuf, params->ca, params->ca_len );
	params->ca[params->ca_len] = 0;

free_all:
	EVP_PKEY_free( pkey );
	X509_free( x509 );
	BIO_free( keybuf );
}

void TLSObject::pubKey( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;


	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter.") ) ) );
		return;
	}
	params.isolate = isolate;
	struct optionStrings *strings = getStrings( isolate );
	Local<Object> opts = args[0]->ToObject();
	
	Local<String> keyString = strings->keyString->Get( isolate );
	Local<String> certString = strings->certString->Get( isolate );
	Local<String> passString = strings->passString->Get( isolate );

	String::Utf8Value *_key;
	if( !opts->Has( keyString ) ) {
		if( !opts->Has( certString ) ) {
			isolate->ThrowException( Exception::Error(
						String::NewFromUtf8( isolate, TranslateText("Missing required option 'key' or 'cert'.") ) ) );
			return;
		}
		MaybeLocal<Value> cert = opts->Get( context, certString );
		_key = new String::Utf8Value( cert.ToLocalChecked()->ToString() );
		params.cert = *_key[0];
		//params.certlen = _key[0].length();
		params.key = NULL;
	}
	else{
		MaybeLocal<Value> keyObj = opts->Get( context, keyString );
		_key = new String::Utf8Value( keyObj.ToLocalChecked()->ToString() );
		params.key = *_key[0];
		params.keylen = _key[0].length();
		params.cert = NULL;
	}

	String::Utf8Value *_password = NULL;
	if( !opts->Has( passString ) ) {
		params.password = NULL;
	}
	else {
		MaybeLocal<Value> password = opts->Get( context, passString );
		_password = new String::Utf8Value( password.ToLocalChecked()->ToString() );
		params.password = *_password[0];
		params.passlen = _password[0].length();
	}

	PubKey( &params );

	if( params.ca ) {
		MaybeLocal<String> retval = String::NewFromUtf8( isolate, params.ca, NewStringType::kNormal, (int)params.ca_len );
		args.GetReturnValue().Set( retval.ToLocalChecked() );
		Deallocate( char *, params.ca );
	}
	
	if( _key ) delete _key;
	if( _password ) delete _password;


}

static void DumpCert( X509 *x509 ) {
	return;
	char *subj;
	subj = X509_NAME_oneline( X509_get_subject_name( x509 ), NULL, 0 );
	char *issuer;
	issuer = X509_NAME_oneline( X509_get_issuer_name( x509 ), NULL, 0 );

	lprintf( "OneLiners (cert to verify):\n%s\n%s", subj, issuer );


	ASN1_INTEGER *val;
	val = X509_get_serialNumber( x509 );
	long serial;
	serial = ASN1_INTEGER_get( val );

	lprintf( "serial : %d", serial );
	
	lprintf( "Subject Identity -------------- " );
	{
		X509_NAME *name = X509_get_subject_name( x509 );
		int n;
		int count = X509_NAME_entry_count( name );

		if( 0 )  // 957 as of the time this was written... lots of strings.
			for( n = 0; n < 960; n++ ) {
				lprintf( "NID: %d  = %32s     %s",  n, OBJ_nid2sn( n ), OBJ_nid2ln( n ) );
			}


		int pkey_nid = OBJ_obj2nid( x509->cert_info->key->algor->algorithm );

		lprintf( "algo nid = %d", pkey_nid );

		int valid = X509_check_ca( x509 );

		lprintf( "valid ca:%d", valid );

		for( n = 0; n < count; n++ )
		{
			X509_NAME_ENTRY *entry = X509_NAME_get_entry( name, n );
			ASN1_OBJECT *o = X509_NAME_ENTRY_get_object( entry );
			ASN1_STRING *v = X509_NAME_ENTRY_get_data( entry );
			int nid = OBJ_obj2nid( o );
			lprintf( "Field: %d  %s %s %*.*s"
						, nid, OBJ_nid2ln( nid ), OBJ_nid2sn( nid )
						, v->length, v->length, v->data );
			LogBinary( v->data, v->length );
		}
		//stack_st_X509_NAME_ENTRY 
		//name->entries
	}

	lprintf( "Issuer Identity -------------- " );
	{
		X509_NAME *name = X509_get_issuer_name( x509 );
		int n;
		int count = X509_NAME_entry_count( name );
		for( n = 0; n < count; n++ )
		{
			X509_NAME_ENTRY *entry = X509_NAME_get_entry( name, n );
			ASN1_OBJECT *o = X509_NAME_ENTRY_get_object( entry );
			ASN1_STRING *v = X509_NAME_ENTRY_get_data( entry );
			int nid = OBJ_obj2nid( o );
			lprintf( "Field: %d  %s %s %*.*s"
					, nid, OBJ_nid2ln( nid ), OBJ_nid2sn( nid )
					, v->length, v->length, v->data );
			LogBinary( v->data, v->length );
			//X509_NAME_oneline
			//X509_NAME_add_entry_by_txt
			//name->
		}
		//stack_st_X509_NAME_ENTRY 
		//name->entries
	}

	lprintf( "Extensions ------------ " );
	{
		int extCount = X509_get_ext_count( x509 );
		int n;
		for( n = 0; n < extCount; n++ )
		{
			X509_EXTENSION *ext = X509_get_ext( x509, n );
			ASN1_OBJECT *o = ext->object;
			ASN1_STRING *v = (ASN1_STRING *)X509V3_EXT_d2i( ext );
			int nid = OBJ_obj2nid( o );
			//V_ASN1_OCTET_STRING
			lprintf( "extension: %d %d  %s  %s  %d", ext->critical>0 ? "critical" : "",
				nid, OBJ_nid2ln( nid ), OBJ_nid2sn( nid ), v->type );
			if( nid == NID_authority_key_identifier ) {
				AUTHORITY_KEYID *akid = (AUTHORITY_KEYID *)v;
				//akid->serial
				GENERAL_NAMES *names = akid->issuer;
				ASN1_OCTET_STRING *key = akid->keyid;
				ASN1_INTEGER *ser = akid->serial;
				LogBinary( key->data, key->length );
				long val = ASN1_INTEGER_get( ser );
				lprintf( "Serial is:%d", val );
			}
			else if( nid == NID_subject_key_identifier ) {
				ASN1_OCTET_STRING *skid = (ASN1_OCTET_STRING *)v;
				LogBinary( skid->data, skid->length );
			} else if( nid == NID_basic_constraints ) {
				BASIC_CONSTRAINTS *bc = (BASIC_CONSTRAINTS *)v;
				long pathlen = bc->pathlen?ASN1_INTEGER_get( bc->pathlen ):0;
				lprintf( "Basic Constraint: CA? %d   %s%d", bc->ca, bc->pathlen?"pathlen:":"ignore:", pathlen );
				//X509_get_ext_d2i( x, NID_subject_key_identifier, NULL, NULL );
				//X509V3_get_d2i( x->cert_info->extensions, nid, crit, idx );
				// 0x55,0x1D,0x13
			} else if( nid == NID_key_usage ) {
				//ASN1_BIT_STRING *s = (ASN1_BIT_STRING *)X509_get_ext_d2i( x509, NID_key_usage, NULL, NULL );
				int usage = v->data[0];
				LogBinary( v->data, v->length );
				if( v->length > 1 )
					usage |= v->data[1];
				lprintf( "USAGE: %08x", usage );
			}
			else
				if( v->data )
					LogBinary( v->data, v->length );
		}
	}

}

static LOGICAL Validate( struct info_params *params ) {
//   some good documentation.
//  https://zakird.com/2013/10/13/certificate-parsing-with-openssl
	LOGICAL valid = FALSE;
	BIO *keybuf = BIO_new( BIO_s_mem() );

	//EVP_PKEY *pkey;
	X509 *x509 = NULL;
	STACK_OF( X509 ) *chain = sk_X509_new_null();

	if( params->chain ) {
		LOGICAL goodRead;
		BIO_write( keybuf, params->chain, (int)strlen( params->chain ) );
		while( ( BIO_pending( keybuf )?(goodRead=TRUE):(goodRead=FALSE) ) && 
			PEM_read_bio_X509( keybuf, &x509, NULL, NULL ) ) {
			if( X509_check_ca( x509 ) ) {
				/* Function return 0, if it is not CA certificate, 1 if it is proper
				X509v3 CA certificate with basicConstraints extension CA : TRUE,
				3, if it is self - signed X509 v1 certificate, 4, if it is
				certificate with keyUsage extension with bit keyCertSign set,
				but without basicConstraints, and 5 if it has outdated Netscape
				Certificate Type extension telling that it is CA certificate. */
				//lprintf( "add cert...  %d", X509_check_ca( x509 ) );
				sk_X509_push( chain, x509 );
				x509 = NULL;
			}
			else {
				throwError( params, "validate : failed to read cert chain" );
				goto free_all;
			}
		}
		if( goodRead ) {
			throwError( params, "validate : failed to read cert chain" );
			goto free_all;
		}
		// the last read
		//flushErrors();
	}

	BIO_write( keybuf, params->cert, (int)strlen( params->cert ) );
	PEM_read_bio_X509( keybuf, &x509, NULL, NULL );
	if( !x509 )
	{
		throwError( params, "validate: bad certificate" );
		goto free_all;
	}

	{
		X509_STORE_CTX *x509_store = X509_STORE_CTX_new();
		//X509_STORE_load_locations( )
		X509_STORE_CTX_init( x509_store, NULL, x509, NULL );
		X509_STORE_CTX_trusted_stack( x509_store, chain );
		// 0x80000 X509_V_FLAG_PARTIAL_CHAIN

		//X509 *chain_ss = sk_X509_pop( x509_store->chain );
		//x509_store->check_issued( x509_store, x509, chain_ss );
		//lprintf( "before verify:" ); flushErrors();
		int verified = X509_verify_cert( x509_store );
		//lprintf( "verified:%d", verified );
		if( verified <= 0 ) {
			char subbuf[256];
			char issbuf[256];
			int err = X509_STORE_CTX_get_error( x509_store );

			X509 *badCert = X509_STORE_CTX_get_current_cert( x509_store );
			char *subj;
			subj = X509_NAME_oneline( X509_get_subject_name( badCert ), subbuf, 256 );
			char *issuer;
			issuer = X509_NAME_oneline( X509_get_issuer_name( badCert ), issbuf, 256 );
			//lprintf( "Error OneLiners (cert to verify):\n%s\n%s", subj, issuer );
			PVARTEXT pvt = VarTextCreate();
			switch( err ) {
			case X509_V_ERR_CERT_SIGNATURE_FAILURE:
				VarTextAddData( pvt, TranslateText( "validate: certificate signature failed:" ), VARTEXT_ADD_DATA_NULTERM );
				VarTextAddData( pvt, TranslateText( " subject:" ), VARTEXT_ADD_DATA_NULTERM );
				VarTextAddData( pvt, subj, VARTEXT_ADD_DATA_NULTERM );
				VarTextAddData( pvt, TranslateText( " issuer:" ), VARTEXT_ADD_DATA_NULTERM );
				VarTextAddData( pvt, issuer, VARTEXT_ADD_DATA_NULTERM );
				_throwError( params, GetText( VarTextPeek( pvt ) ) );
				break;
			case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
				VarTextAddData( pvt, TranslateText( "validate: unable to get issuer cert locally." ), VARTEXT_ADD_DATA_NULTERM );
				_throwError( params, GetText( VarTextPeek( pvt ) ) );
				break;
			default:
				VarTextAddData( pvt, TranslateText( "validate: failed; unhandled error:" ), VARTEXT_ADD_DATA_NULTERM );
				vtprintf( pvt, "%d", err );
				//VarTextAddData( pvt, TranslateText( " depth:" ), VARTEXT_ADD_DATA_NULTERM );
				//vtprintf( pvt, "%d", X509_STORE_CTX_get_error_depth( x509_store ) );
				_throwError( params, GetText( VarTextPeek( pvt ) ) );
				break;
			}
			DumpCert( badCert );
			//flushErrors();
			VarTextDestroy( &pvt );
			
		}
		else
			valid = TRUE;
		X509_STORE_CTX_free( x509_store );
	}

free_all:
	X509_free( x509 );
	sk_X509_pop_free( chain, X509_free );
	BIO_free( keybuf );
	return valid;
}

void TLSObject::validate( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;

	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter.") ) ) );
		return;
	}
	params.isolate = isolate;

	Local<Object> opts = args[0]->ToObject();
	
	Local<String> certString = String::NewFromUtf8( isolate, "cert" );
	Local<String> chainString = String::NewFromUtf8( isolate, "chain" );

	String::Utf8Value *_key;
	if( !opts->Has( certString ) ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'cert'.") ) ) );
		return;
	}
	MaybeLocal<Value> cert = opts->Get( context, certString );
	_key = new String::Utf8Value( cert.ToLocalChecked()->ToString() );
	params.cert = *_key[0];
	params.certlen = _key[0].length();
	params.key = NULL;

	String::Utf8Value *chain;
	if( !opts->Has( chainString ) ) {
		//isolate->ThrowException( Exception::Error(
		//	String::NewFromUtf8( isolate, TranslateText( "Missing required option 'cert'." ) ) ) );
		//return;
		chain = NULL;
		params.chain = NULL;
	}
	else {
		MaybeLocal<Value> cert = opts->Get( context, chainString );
		chain = new String::Utf8Value( cert.ToLocalChecked()->ToString() );
		params.chain = *chain[0];
	}
	params.ca = NULL;

	if( Validate( &params ) )
		args.GetReturnValue().Set( true );
	else 
		args.GetReturnValue().Set( false );

	if( params.ca ) {
		MaybeLocal<String> retval = String::NewFromUtf8( isolate, params.ca, NewStringType::kNormal, (int)params.ca_len );
		args.GetReturnValue().Set( retval.ToLocalChecked() );
		Deallocate( char *, params.ca );
	}
	if( chain ) delete chain;
	if( _key ) delete _key;
}

static Local<Value> Expiration( struct info_params *params ) {
	X509 * x509 = NULL;
	params->ca = NULL;

	BIO *keybuf = BIO_new( BIO_s_mem() );

	BIO_write( keybuf, params->cert, params->certlen );
	if( !PEM_read_bio_X509( keybuf, &x509, NULL, NULL )) {
		throwError( params, "expiration : failed to parse cert" );
		BIO_free( keybuf );
		return Undefined(params->isolate);// goto free_all;
	}


	ASN1_TIME *before = x509->cert_info->validity->notAfter;

#if 0
//#ifdef _WIN32
	// the system time
	char *timestring = (char*)before->data;
	//ASN1_TIME_adj
	int val;
	SYSTEMTIME systemTime;
	systemTime.wMilliseconds = 0;
	if( before->type == V_ASN1_UTCTIME )
	if( (before->type == V_ASN1_UTCTIME) /*before->length == 13*/ ) {
		val = 0;
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		//if( year < 70 ) year += 100;
		systemTime.wYear = val + 2000;
	}
	if( (before->type == V_ASN1_GENERALIZEDTIME) /*before->length == 15*/ ) {
		val = 0;
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		systemTime.wYear = val;
	}
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	systemTime.wMonth = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	systemTime.wDay = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	systemTime.wHour = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	systemTime.wMinute = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	systemTime.wSecond = val;
	//GetSystemTime( &systemTime );

	// the current file time
	FILETIME fileTime;
	SystemTimeToFileTime( &systemTime, &fileTime );

	// filetime in 100 nanosecond resolution
	ULONGLONG fileTimeNano100;
	fileTimeNano100 = (((ULONGLONG)fileTime.dwHighDateTime) << 32) + fileTime.dwLowDateTime;

	//to milliseconds and unix windows epoche offset removed
	ULONGLONG posixTime = fileTimeNano100 / 10000 - 11644473600000;

	double dTime = posixTime;// IntCreateFromText( (char*)before->data ) * 1000;

#endif
	struct tm t;
	char * timestring = (char*)before->data;
	int val;
	if( before->type == V_ASN1_UTCTIME )
		if( (before->type == V_ASN1_UTCTIME) /*before->length == 13*/ ) {
			val = 0;
			val = val * 10 + ((timestring++)[0] - '0');
			val = val * 10 + ((timestring++)[0] - '0');
			//if( year < 70 ) year += 100;
			t.tm_year = (val + 2000)-1900;
		}
	if( (before->type == V_ASN1_GENERALIZEDTIME) /*before->length == 15*/ ) {
		val = 0;
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		t.tm_year = (val + 2000) - 1900;
	}
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t.tm_mon = val-1;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t.tm_mday = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t.tm_hour = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t.tm_min = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t.tm_sec = val;

	Local<Value> date = Date::New( params->isolate, (double)mktime( &t ) * 1000.0 );
	
	return date;
}


void TLSObject::expiration( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;

	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required certificate data.") ) ) );
		return;
	}
	params.isolate = isolate;

	String::Utf8Value cert( args[0]->ToString() );
	params.cert = *cert;
	params.certlen = cert.length();

	Local<Value> xx = Expiration( &params );
	args.GetReturnValue().Set( xx );
	
		
}
