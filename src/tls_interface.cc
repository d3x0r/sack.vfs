// https://tools.ietf.org/html/rfc5280
//   �4.2.1.1 and �4.2.1.2.
//  https://tools.ietf.org/html/rfc5280#section-4.2.1.2
//

// http://fm4dd.com/openssl/manual-apps/x509v3_config.htm (extension configuration docs)


#include "global.h"

#if WIN32
#define timegm _mkgmtime
#endif
                             
#if OPENSSL_VERSION_NUMBER >= 0x1010008f
#  define USE_TLS_ACCESSORS
#endif

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
  initials friendlyName
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

	Eternal<String> *keyString;
	Eternal<String> *certString;
	Eternal<String> *passString;
	Eternal<String> *serialString;
	Eternal<String> *orgString;
	Eternal<String> *unitString;
	Eternal<String> *locString;
	Eternal<String> *countryString;
	Eternal<String> *stateString;
	Eternal<String> *commonString;
	Eternal<String> *pubkeyString;
	Eternal<String> *issuerString;
	Eternal<String> *expireString;
	Eternal<String> *requestString;
	Eternal<String> *signerString;
	Eternal<String> *subjectString;
	Eternal<String> *DNSString;
	Eternal<String> *IPString;
};

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

	struct alt_subject {
		PLIST DNS;
		PLIST IP;
	} altSubject;

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


static struct optionStrings *getStrings( Isolate *isolate ) {
	static PLIST strings;
	INDEX idx;
	struct optionStrings * check;
	LIST_FORALL( strings, idx, struct optionStrings *, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		check->isolate = isolate;
		check->keyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "key", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->certString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "cert", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->passString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "password", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->serialString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "serial", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->orgString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "org", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->unitString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "unit", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->locString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "locality", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->countryString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "country", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->stateString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "state", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->commonString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "name", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->pubkeyString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "pubkey", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->issuerString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "issuer", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->expireString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "expire", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->requestString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "request", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->signerString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "signer", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->subjectString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "subject", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->DNSString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "DNS", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->IPString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "IP", v8::NewStringType::kNormal ).ToLocalChecked() );
		//check->String = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "", v8::NewStringType::kNormal ).ToLocalChecked() );
		AddLink( &strings, check );
	}
	return check;
}

void TLSObject::Init( Isolate *isolate, Local<Object> exports )
{
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> tlsTemplate;

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();

	tlsTemplate = FunctionTemplate::New( isolate, New );
	tlsTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.vfs.Volume", v8::NewStringType::kNormal ).ToLocalChecked() );
	tlsTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	//NODE_SET_PROTOTYPE_METHOD( tlsTemplate, "genkey", genKey );
	Local<Object> i = Object::New( isolate );
	//Local<Function> i = tlsTemplate->GetFunction(isolate->GetCurrentContext());
	SET( i, "seed", Function::New( context, seed ).ToLocalChecked() );
	SET( i, "genkey", Function::New( context, genKey ).ToLocalChecked() );
	SET( i, "gencert", Function::New( context, genCert ).ToLocalChecked() );
	SET( i, "genreq", Function::New( context, genReq ).ToLocalChecked() );
	SET( i, "pubkey", Function::New( context, pubKey ).ToLocalChecked() );
	SET( i, "signreq", Function::New( context, signReq ).ToLocalChecked() );
	SET( i, "validate", Function::New( context, validate ).ToLocalChecked() );
	SET( i, "expiration", Function::New( context, expiration ).ToLocalChecked() );
	SET( i, "certToString", Function::New( context, certToString ).ToLocalChecked() );

	//constructor.Reset( isolate, tlsTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET( exports, "TLS", i );

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
				String::NewFromUtf8( params->isolate, GetText( result ), v8::NewStringType::kNormal, (int)GetTextSize( result ) ).ToLocalChecked() ) );
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
			String::Utf8Value val( USE_ISOLATE( args.GetIsolate() ) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
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
		MaybeLocal<Object> mo = cons->NewInstance( isolate->GetCurrentContext(), 0, argv );
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
		keylen = (int)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe(0);
		if( argc > 1 ) {
  			pass = new String::Utf8Value( USE_ISOLATE( isolate ) args[1]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
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

static int pem_password(char *buf, int size, int rwflag, void *u)
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
		X509_time_adj_ex( X509_get_notBefore( x509 ), 0, 0, NULL );
		// (60 seconds * 60 minutes * 24 hours * 365 days) = 31536000.
		X509_time_adj_ex( X509_get_notAfter( x509 ), params->expire?params->expire:365, 0, NULL );
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
					//lprintf( "SIGNING ON ROOT CERT" );
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
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	params.isolate = isolate;

	struct optionStrings *strings = getStrings( isolate );

	Local<Object> opts = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;

	Local<String> optName;
	if( !opts->Has( context, optName = strings->countryString->Get(isolate) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'country'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> country = GETV( opts, optName )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	String::Utf8Value _country( USE_ISOLATE( isolate ) country->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.country = *_country;
	params.countrylen = _country.length();

	if( !opts->Has( context, optName = strings->stateString->Get(isolate) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'state' or province."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> state = GETV( opts, optName )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	String::Utf8Value _state( USE_ISOLATE( isolate ) state->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.state = *_state;
	params.statelen = _state.length();

	if( !opts->Has( context, optName = strings->locString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'locality'(city)."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> locality = GETV( opts, optName )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	String::Utf8Value _locality( USE_ISOLATE( isolate ) locality->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.locality = *_locality;
	params.localitylen = _locality.length();

	if( !opts->Has( context, optName = strings->orgString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'org'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> org = GETV( opts, optName )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	String::Utf8Value _org( USE_ISOLATE( isolate ) org->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.org = *_org;
	params.orglen = _org.length();

	Local<String> unitString = strings->unitString->Get( isolate );
	String::Utf8Value *_unit;
	if( !opts->Has( context, unitString ).ToChecked() ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'.") ) ) );
		//return;
		_unit = NULL;
		params.orgUnit = NULL;
	}else {
		Local<Object> unit = GETV( opts, unitString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
		_unit = new String::Utf8Value( USE_ISOLATE( isolate ) unit->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.orgUnit = *_unit[0];
		params.orgUnitlen = _unit[0].length();
	}

	if( !opts->Has( context, optName = strings->commonString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'name'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> common = GETV( opts, optName )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked( );
	String::Utf8Value _common( USE_ISOLATE( isolate ) common->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.common = *_common;
	params.commonlen = _common.length();

	if( !opts->Has( context, optName = strings->keyString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'key'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> key = GETV( opts, optName )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	String::Utf8Value _key( USE_ISOLATE( isolate ) key->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.key = *_key;
	params.keylen = _key.length();

	Local<String> pubkeyString = strings->pubkeyString->Get( isolate );// String::NewFromUtf8( isolate, "pubkey", v8::NewStringType::kNormal ).ToLocalChecked();
	String::Utf8Value *_pubkey = NULL;

	if( opts->Has( context, pubkeyString ).ToChecked() ) {
		Local<Object> key = GETV( opts, pubkeyString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
		_pubkey = new String::Utf8Value( USE_ISOLATE( isolate ) key->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.pubkey = *_pubkey[0];
		params.pubkeylen = _pubkey[0].length();
	}
	else
		params.pubkey = NULL;


	Local<String> serialString = strings->serialString->Get( isolate );// String::NewFromUtf8( isolate, "serial", v8::NewStringType::kNormal ).ToLocalChecked();
	if( !opts->Has( context, serialString ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'serial'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	else {
		//Local<Integer> serial = 
		params.serial = GETV( opts, serialString )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe(0);
		if( params.serial < 1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial' must be more than 0." ), v8::NewStringType::kNormal).ToLocalChecked() ) );
			return;
		}
	}

	Local<String> issuerString = strings->issuerString->Get( isolate );//String::NewFromUtf8( isolate, "issuer" );
	String::Utf8Value *_issuer;
	if( !opts->Has( context, issuerString ).ToChecked() ) {
		_issuer = NULL;
		params.issuer = NULL;
	}else {
		Local<Object> issuer = GETV( opts, issuerString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
		_issuer = new String::Utf8Value( USE_ISOLATE( isolate ) issuer->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.issuer = *_issuer[0];
		params.issuerlen = _issuer[0].length();
	}

	Local<String> expireString = strings->expireString->Get( isolate );
	if( !opts->Has( context, expireString ).ToChecked() ) {
		params.expire = 0;
	}
	else {
		params.expire = (int)GETV( opts, expireString )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe(0);
	}


	Local<String> passString = strings->passString->Get( isolate );;

	String::Utf8Value *_password = NULL;
	if( !opts->Has( context, passString ).ToChecked() ) {
		params.password = NULL;
	}
	else {
		Local<Object> password = GETV(opts, passString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
		_password = new String::Utf8Value( USE_ISOLATE( isolate ) password->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
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
	X509_REQ *req = NULL;
	//X509 * x509;
	EVP_PKEY *pkey = NULL;
	PLIST addresses = NULL;
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

			{
				if( params->altSubject.DNS || params->altSubject.IP ) {
					INDEX idx;
					GENERAL_NAMES *names = GENERAL_NAMES_new();
					char *name;
					LIST_FORALL( params->altSubject.DNS, idx, char *, name ) {
						GENERAL_NAME *gname = GENERAL_NAME_new();
						ASN1_IA5STRING *val = ASN1_IA5STRING_new();
						ASN1_STRING_set( val, name, (int)strlen(name) );
						gname->type = GEN_DNS;
						gname->d.dNSName = val;
						//GENERAL_NAME_set0_value( gname, GEN_DNS, val );
						sk_GENERAL_NAME_push( names, gname );
					}
					LIST_FORALL( params->altSubject.IP, idx, char *, name ) {
						GENERAL_NAME *gname = GENERAL_NAME_new();
						SOCKADDR *addr = CreateRemote( name, 0 );
						if( !addr ) {
							char buf[256];
							snprintf( buf, 256, "%s:%s", TranslateText("Bad address passed"), name );
							params->isolate->ThrowException( Exception::Error(
										  String::NewFromUtf8( params->isolate, buf, v8::NewStringType::kNormal ).ToLocalChecked() ) );
							params->ca = NULL;
							return;
						}
						AddLink( &addresses, addr );
						ASN1_IA5STRING *val = ASN1_IA5STRING_new();
						if( IsAddressV6( addr ) )
							ASN1_STRING_set( val, addr->sa_data+6, (int)16 );
						else
							ASN1_STRING_set( val, addr->sa_data+2, (int)4 );
						gname->type = GEN_IPADD;

						//gname->d.ip = val;
						gname->d.iPAddress = val;

						//GENERAL_NAME_set0_value( gname, GEN_DNS, val );
						sk_GENERAL_NAME_push( names, gname );
					}
					//ASN1_STRING *data = ASN1_STRING_new();
					//char *r = params->subject;
					//ASN1_STRING_set( data, r, (int)params->subjectlen );

					//STACK_OF( X509_EXTENSION ) *extensions = X509_REQ_get_extensions( req );// , NID_subject_alt_name, data, 0, X509V3_ADD_DEFAULT );
					//X509_REQ_set_extension_nids

					STACK_OF( X509_EXTENSION) *exts = sk_X509_EXTENSION_new_null();
					//unsigned char *val;
					//i2d_GENERAL_NAMES( )
					//unsigned char *out = NULL;
					;
					X509_EXTENSION *ex = X509V3_EXT_i2d( NID_subject_alt_name, 0, names );// X509V3_EXT_conf_nid( NULL, NULL, NID_subject_alt_name, (char*)out );
					if( !ex ) {
						throwError( params, "Something" );
					}
					sk_X509_EXTENSION_push( exts, ex );

					const X509V3_EXT_METHOD *method = X509V3_EXT_get_nid( NID_certificate_policies );
					void *ext_struc = (X509_EXTENSION*)method->r2i( method, NULL, "2.5.29.32.0" );
					ex = X509V3_EXT_i2d( NID_certificate_policies, 0, ext_struc );
					if( !ex ) {
						throwError( params, "Failed to make certificate policy extension" );
					}
					sk_X509_EXTENSION_push( exts, ex );

					X509_REQ_add_extensions( req, exts );
					sk_X509_EXTENSION_pop_free( exts, X509_EXTENSION_free );
				}
			}

			if( X509_REQ_sign( req, pkey, EVP_sha256() ) < 0 )
			{
				throwError( params, "genreq:Signing failed." );
				goto free_all;
			}

			{
				PEM_write_bio_X509_REQ_NEW( keybuf, req );
				params->ca_len = BIO_pending( keybuf );
				params->ca = NewArray( char, params->ca_len + 1 );

				BIO_read( keybuf, params->ca, params->ca_len );
				params->ca[params->ca_len] = 0;
			}
		}
	}
free_all:
	if( addresses )
	{
		SOCKADDR *addr;
		INDEX idx;
		LIST_FORALL( addresses, idx, SOCKADDR*, addr )
			ReleaseAddress( addr );
		DeleteList( &addresses );
	}
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
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	params.isolate = isolate;

	struct optionStrings *strings = getStrings( isolate );
	Local<Object> opts = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;

	Local<String> optName;

	if( !opts->Has( context, optName = strings->countryString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'country'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> country = GETV( opts, optName )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	String::Utf8Value _country( USE_ISOLATE( isolate ) country->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.country = *_country;
	params.countrylen = _country.length();

	if( !opts->Has( context, optName = strings->stateString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'state' or province."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value _state( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.state = *_state;
	params.statelen = _state.length();

	//String::Utf8Value *_subject;// (GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ));
	params.altSubject.DNS = NULL;
	params.altSubject.IP = NULL;
	if( opts->Has( context, optName = strings->subjectString->Get( isolate ) ).ToChecked() ) {
		Local<Value> subVal = GETV( opts, optName );
		if( subVal->IsObject() ) {
			Local<Object> subject = subVal->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
			Local<String> dnsString = strings->DNSString->Get( isolate );
			Local<String> ipString = strings->IPString->Get( isolate );
			if( subject->Has( context, dnsString ).ToChecked() ) {
				Local<Object> name = GETV( subject, dnsString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
				do {
					if( name->IsUndefined() || name->IsNull() )
						break;
					if( name->IsArray() ) {
						//String::Utf8Value *entry;
						Local<Array> values = name.As<Array>();
						int count = values->Length();
						int n;
						for( n = 0; n < count; n++ ) {
							String::Utf8Value val( USE_ISOLATE( isolate ) GETN( values, n ) );
							AddLink( &params.altSubject.DNS, StrDup( *val ) );
						}
					}
					else {
						String::Utf8Value val( USE_ISOLATE( isolate ) name->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
						AddLink( &params.altSubject.DNS, StrDup( *val ) );
					}
				} while( 0 );
			}
			if( subject->Has( context, ipString ).ToChecked() ) {
				Local<Object> name = GETV( subject, ipString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
				do {
					if( name->IsUndefined() || name->IsNull() )
						break;
					if( name->IsArray() ) {
						//String::Utf8Value *entry;
						Local<Array> values = name.As<Array>();
						int count = values->Length();
						int n;
						for( n = 0; n < count; n++ ) {
							String::Utf8Value val( USE_ISOLATE( isolate ) GETN( values, n ) );
							AddLink( &params.altSubject.IP, StrDup( *val ) );
						}
					}
					else {
						String::Utf8Value val( USE_ISOLATE( isolate ) name->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
						AddLink( &params.altSubject.IP, StrDup( *val ) );
					}
				} while( 0 );
			}
		}
	}

	if( !opts->Has( context, optName = strings->locString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText("Missing required option 'locality'(city)."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value _locality( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.locality = *_locality;
	params.localitylen = _locality.length();

	if( !opts->Has( context, optName = strings->orgString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'org'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value _org( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.org = *_org;
	params.orglen = _org.length();

	String::Utf8Value *_unit;
	if( !opts->Has( context, optName = strings->unitString->Get( isolate ) ).ToChecked() ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'.") , v8::NewStringType::kNormal ).ToLocalChecked() ) );
		//return;
		_unit = NULL;
		params.orgUnit = NULL;
	}else {
		_unit = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.orgUnit = *_unit[0];
		params.orgUnitlen = _unit[0].length();
	}

	if( !opts->Has( context, optName = strings->commonString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'name'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value _common( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.common = *_common;
	params.commonlen = _common.length();

	/*
	Local<String> serialString = String::NewFromUtf8( isolate, "serial" , v8::NewStringType::kNormal ).ToLocalChecked();
	if( !opts->Has( context, serialString ) ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial'.", v8::NewStringType::kNormal ).ToLocalChecked() ) ) );
		return;
	}
	else {
		Local<Integer> serial = opts->Get( serialString )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe(0);
		params.serial = serial->Value();
		if( params.serial < 1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial' must be more than 0." ) ) ) );
			return;
		}
	}
	*/

	Local<String> keyString = strings->keyString->Get( isolate );
	if( !opts->Has( context, keyString ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'key'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	Local<Object> key = GETV( opts, keyString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	String::Utf8Value _key( USE_ISOLATE( isolate ) key->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.key = *_key;
	params.keylen = _key.length();

	Local<String> passString = strings->passString->Get( isolate );

	String::Utf8Value *_password = NULL;
	if( !opts->Has( context, passString ).ToChecked() ) {
		params.password = NULL;
	}
	else {
		Local<Object> password = GETV( opts, passString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
		_password = new String::Utf8Value( USE_ISOLATE( isolate ) password->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
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
	X509* cert = NULL;
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
	X509_time_adj_ex(X509_get_notBefore(cert), 0, 0, NULL);
	// (60 seconds * 60 minutes * 24 hours * 365 days) = 31536000.
	X509_time_adj_ex(X509_get_notAfter(cert), params->expire?params->expire:365, 0, NULL);

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
			kid.serial = NULL;// X509_get_serialNumber( x509 );
			kid.issuer = NULL;

			kid.issuer = NULL;// GENERAL_NAMES_new();
#if 0
			GENERAL_NAME *gname = GENERAL_NAME_new();
			ASN1_IA5STRING *val = ASN1_IA5STRING_new();
			ASN1_STRING_set( val, name, (int)strlen( name ) );
			gname->type = GEN_DNS;
			gname->d.dNSName = val;
			//GENERAL_NAME_set0_value( gname, GEN_DNS, val );
			sk_GENERAL_NAME_push( kid.issuer, gname );
#endif


			X509_add1_ext_i2d( cert, NID_authority_key_identifier, &kid, 0, X509V3_ADD_DEFAULT );

			// copy request extensions to signed output.
			STACK_OF( X509_EXTENSION ) *reqExts = X509_REQ_get_extensions( req );
			X509_EXTENSION *reqExt;
			while( reqExt = sk_X509_EXTENSION_pop(reqExts) )
				X509_add_ext( cert, reqExt, -1 );
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
				if( sbc->pathlen && pathlen == 1 )
					bc.ca = 0;
				X509_add1_ext_i2d( cert, NID_basic_constraints, &bc, 1, X509V3_ADD_DEFAULT );
				if( sbc->pathlen && pathlen == 1 ) {
					//lprintf( "SET TERMINAL USAGE" );
					int _usage = KU_DIGITAL_SIGNATURE;
					ASN1_INTEGER *usage = ASN1_INTEGER_new();
					ASN1_INTEGER_set( usage, _usage );
					X509_add1_ext_i2d( cert, NID_key_usage, usage, 1, X509V3_ADD_DEFAULT );
				}
				else {
					//lprintf( "SET CA USAGE" );
					int _usage = KU_CRL_SIGN | KU_KEY_CERT_SIGN | KU_DIGITAL_SIGNATURE;
					ASN1_INTEGER *usage = ASN1_INTEGER_new();
					ASN1_INTEGER_set( usage, _usage );
					X509_add1_ext_i2d( cert, NID_key_usage, usage, 1, X509V3_ADD_DEFAULT );
				}
			}
			else {

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
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	params.isolate = isolate;

	struct optionStrings *strings = getStrings( isolate );

	Local<Object> opts = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
	Local<String> optName;

	if( !opts->Has( context, optName = strings->signerString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'signer'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value _signingCert( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.signingCert = *_signingCert;

	if( !opts->Has( context, optName = strings->requestString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'request'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value _request( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.signReq = *_request;

	Local<String> expireString = strings->expireString->Get( isolate );
	if( !opts->Has( context, expireString ).ToChecked() ) {
		params.expire = 0;
	}
	else {
		params.expire = (int)GETV( opts, expireString )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe(0);
	}

	if( !opts->Has( context, optName = strings->keyString->Get( isolate ) ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'key'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	String::Utf8Value _key( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.key = *_key;
	params.keylen = _key.length();

	Local<String> serialString = strings->serialString->Get( isolate );

	if( !opts->Has( context, serialString ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial'." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	else {
		params.serial = GETV( opts, serialString )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe(0);
		if( params.serial < 1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( "Missing required option 'serial' must be more than 0." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			return;
		}
	}

	Local<String> issuerString = strings->issuerString->Get(isolate);
	Local<String> subjectString = strings->subjectString->Get( isolate );

	String::Utf8Value *_issuer;
	if( !opts->Has( context, issuerString ).ToChecked() ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		//return;
		_issuer = NULL;
		params.issuer = NULL;
	}
	else {
		_issuer = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, issuerString )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.issuer = *_issuer[0];
		params.issuerlen = _issuer[0].length();
	}

	String::Utf8Value *_subject;
	if( !opts->Has( context, subjectString ).ToChecked() ) {
		//isolate->ThrowException( Exception::Error(
		//			String::NewFromUtf8( isolate, TranslateText("Missing required option 'unit'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		//return;
		_subject = NULL;
		params.subject = NULL;
	}
	else {
		_subject = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, subjectString )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.subject = *_subject[0];
		params.subjectlen = _subject[0].length();
	}

	Local<String> pubkeyString = strings->pubkeyString->Get( isolate );// String::NewFromUtf8( isolate, "pubkey", v8::NewStringType::kNormal ).ToLocalChecked();
	String::Utf8Value *_pubkey = NULL;

	if( opts->Has( context, pubkeyString ).ToChecked() ) {
		lprintf( "using custom public key..." );
		Local<Object> key = GETV( opts,  pubkeyString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
		_pubkey = new String::Utf8Value( USE_ISOLATE( isolate ) key->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.pubkey = *_pubkey[0];
		params.pubkeylen = _pubkey[0].length();
	}
	else
		params.pubkey = NULL;

	Local<String> passString = strings->passString->Get( isolate );;

	String::Utf8Value *_password = NULL;
	if( !opts->Has( context, passString ).ToChecked() ) {
		params.password = NULL;
	}
	else {
		Local<Object> password = GETV( opts, passString )->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;
		_password = new String::Utf8Value( USE_ISOLATE( isolate ) password->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
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
			pkey = NULL;
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
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	params.isolate = isolate;
	struct optionStrings *strings = getStrings( isolate );
	Local<Object> opts = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;

	Local<String> keyString = strings->keyString->Get( isolate );
	Local<String> certString = strings->certString->Get( isolate );
	Local<String> passString = strings->passString->Get( isolate );

	String::Utf8Value *_key;
	if( !opts->Has( context, keyString ).ToChecked() ) {
		if( !opts->Has( context, certString ).ToChecked() ) {
			isolate->ThrowException( Exception::Error(
						String::NewFromUtf8( isolate, TranslateText("Missing required option 'key' or 'cert'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
			return;
		}
		MaybeLocal<Value> cert = opts->Get( context, certString );
		_key = new String::Utf8Value( USE_ISOLATE( isolate ) cert.ToLocalChecked()->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.cert = *_key[0];
		//params.certlen = _key[0].length();
		params.key = NULL;
	}
	else{
		MaybeLocal<Value> keyObj = opts->Get( context, keyString );
		_key = new String::Utf8Value( USE_ISOLATE( isolate ) keyObj.ToLocalChecked()->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.key = *_key[0];
		params.keylen = _key[0].length();
		params.cert = NULL;
	}

	String::Utf8Value *_password = NULL;
	if( !opts->Has( context, passString ).ToChecked() ) {
		params.password = NULL;
	}
	else {
		MaybeLocal<Value> password = opts->Get( context, passString );
		_password = new String::Utf8Value( USE_ISOLATE( isolate ) password.ToLocalChecked()->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
		params.password = *_password[0];
		params.passlen = _password[0].length();
	}

	PubKey( &params );

	if( params.ca ) {
		MaybeLocal<String> retval = String::NewFromUtf8( isolate, params.ca, v8::NewStringType::kNormal, (int)params.ca_len ).ToLocalChecked();
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
				lprintf( "NID: %d  = %32s     %s", n, OBJ_nid2sn( n ), OBJ_nid2ln( n ) );
			}


		//int pkey_nid = OBJ_obj2nid( x509_get_cert_ x509->cert_info->key->algor->algorithm );

		//lprintf( "algo nid = %d", pkey_nid );

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
#if 1
			ASN1_OBJECT *o = X509_EXTENSION_get_object( ext );
#else
			ASN1_OBJECT *o = ext->object;
#endif
			ASN1_STRING *v = (ASN1_STRING *)X509V3_EXT_d2i( ext );
			int nid = OBJ_obj2nid( o );
			//V_ASN1_OCTET_STRING
			lprintf( "extension: %d %d  %s  %s  %d", X509_EXTENSION_get_critical( ext )>0 ? "critical" : "",
			//lprintf( "extension: %d %d  %s  %s  %d", ext->critical>0 ? "critical" : "",
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
			}
			else if( nid == NID_basic_constraints ) {
				BASIC_CONSTRAINTS *bc = (BASIC_CONSTRAINTS *)v;
				long pathlen = bc->pathlen ? ASN1_INTEGER_get( bc->pathlen ) : 0;
				lprintf( "Basic Constraint: CA? %d   %s%d", bc->ca, bc->pathlen ? "pathlen:" : "ignore:", pathlen );
				//X509_get_ext_d2i( x, NID_subject_key_identifier, NULL, NULL );
				//X509V3_get_d2i( x->cert_info->extensions, nid, crit, idx );
				// 0x55,0x1D,0x13
			}
			else if( nid == NID_key_usage ) {
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
			/*
			X509V3_EXT_METHOD v3_cpols = {
					NID_certificate_policies, 0,ASN1_ITEM_ref(CERTIFICATEPOLICIES),
				0,0,0,0,
				0,0,
				0,0,
				(X509V3_EXT_I2R)i2r_certpol,
				(X509V3_EXT_R2I)r2i_certpol,
				NULL
			};

				https://www.alvestrand.no/objectid/2.5.29.32.0.html
				certificatePolicies = 2.5.29.32.0
				2.5.29.32 - Certificate Policies
				2.5.29 - certificateExtension (id-ce)
				2.5 - X.500 Directory Services
				2 - ISO/ITU-T jointly assigned OIDs

				pol = (struct stack_st *) X509_get_ext_d2i(pCert, NID_certificate_policies, NULL, NULL);

				if(pol != NULL)
				{
					for(j = 0; j < sk_POLICYINFO_num(pol); j++)
					{
						pinfo = sk_POLICYINFO_value(pol, j);
						if(pinfo != NULL)
						{
							char szBuffer[32] = {0};
							OBJ_obj2txt(szBuffer, 64, pinfo->policyid, 0);
							if (0 == strcmp(szBuffer, RRN_OID))// || 0 == strcmp(szBuffer, CERTIPOST_OID))
							{
								iRet = X509_V_OK;
								bFound = TRUE;
								break;
							}
						}
					}
				}

				bool x509v3ext::parse_certpol(QString *, QString *adv) const
				{
					bool retval = true;
					QStringList pols;
					QString myadv;
					int i;
					STACK_OF(POLICYINFO) *pol = (STACK_OF(POLICYINFO) *)d2i();

					if (!pol)
						return false;

					for (i = 0; i < sk_POLICYINFO_num(pol); i++) {
						POLICYINFO *pinfo = sk_POLICYINFO_value(pol, i);
						if (!pinfo->qualifiers) {
							pols << obj2SnOid(pinfo->policyid);
							continue;
						}
						QString tag = QString("certpol%1_sect").arg(i);
						pols << QString("@") + tag;
						if (!gen_cpol_qual_sect(tag, pinfo, &myadv)) {
							retval = false;
							break;
						}
					}
					if (retval && adv)
						*adv = QString("certificatePolicies=%1ia5org,%2\n").
						arg(parse_critical()).arg(pols.join(", ")) + *adv + myadv;
					sk_POLICYINFO_free(pol);
					return retval;
				}
			*/
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
		int n;
		BIO_write( keybuf, params->chain, (int)strlen( params->chain ) );
		while( ( (n=BIO_pending( keybuf ))?(goodRead=TRUE):(goodRead=FALSE) ) &&
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
				throwError( params, "validate : cert chain contains non-authoritative cert" );
				goto free_all;
			}
		}
		if( goodRead && ( n > 1 ) ) {
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
			case X509_V_ERR_CERT_NOT_YET_VALID:
				VarTextAddData( pvt, TranslateText( "validate: certificate is not valid yet." ), VARTEXT_ADD_DATA_NULTERM );
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
					String::NewFromUtf8( isolate, TranslateText("Missing required option object parameter."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	params.isolate = isolate;

	Local<Object> opts = args[0]->ToObject( isolate->GetCurrentContext() ).ToLocalChecked() ;

	Local<String> certString = String::NewFromUtf8( isolate, "cert", v8::NewStringType::kNormal ).ToLocalChecked();
	Local<String> chainString = String::NewFromUtf8( isolate, "chain", v8::NewStringType::kNormal ).ToLocalChecked();

	String::Utf8Value *_key;
	if( !opts->Has( context, certString ).ToChecked() ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required option 'cert'."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	MaybeLocal<Value> cert = opts->Get( context, certString );
	_key = new String::Utf8Value( USE_ISOLATE( isolate ) cert.ToLocalChecked()->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.cert = *_key[0];
	params.certlen = _key[0].length();
	params.key = NULL;

	String::Utf8Value *chain;
	if( !opts->Has( context, chainString ).ToChecked() ) {
		//isolate->ThrowException( Exception::Error(
		//	String::NewFromUtf8( isolate, TranslateText( "Missing required option 'cert'." ) ) ) );
		//return;
		chain = NULL;
		params.chain = NULL;
	}
	else {
		MaybeLocal<Value> cert = opts->Get( context, chainString );
		chain = new String::Utf8Value( USE_ISOLATE( isolate ) cert.ToLocalChecked()->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
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


static void ConvertTimeString( struct tm *t, const ASN1_TIME *time ) {
	char * timestring = (char*)time->data;
	int val;
	if( time->type == V_ASN1_UTCTIME )
		if( (time->type == V_ASN1_UTCTIME) /*before->length == 13*/ ) {
			val = 0;
			val = val * 10 + ((timestring++)[0] - '0');
			val = val * 10 + ((timestring++)[0] - '0');
			//if( year < 70 ) year += 100;
			t->tm_year = (val + 2000)-1900;
		}
	if( (time->type == V_ASN1_GENERALIZEDTIME) /*before->length == 15*/ ) {
		val = 0;
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		val = val * 10 + ((timestring++)[0] - '0');
		t->tm_year = val-1900;
	}
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t->tm_mon = val - 1;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t->tm_mday = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t->tm_hour = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t->tm_min = val;
	val = 0;
	val = val * 10 + ((timestring++)[0] - '0');
	val = val * 10 + ((timestring++)[0] - '0');
	t->tm_sec = val;
	//lprintf( "Converted: %s to  %d %d %d %d %d %d", time->data, t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec );
#if __GNUC__
	t->tm_gmtoff = 0;
	t->tm_zone = NULL;
#endif

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
	BIO_free( keybuf );

	const ASN1_TIME *before = X509_get_notAfter( x509 );
	struct tm t;
	char * timestring = (char*)before->data;

	ConvertTimeString( &t, before );
	X509_free( x509 );
	Local<Value> date = Date::New( params->isolate->GetCurrentContext(), (double)timegm( &t ) * 1000.0 ).ToLocalChecked();
	//lprintf( "Converted: %d %d %d %d %d %d", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec );

	return date;
}


void TLSObject::expiration( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;

	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8( isolate, TranslateText("Missing required certificate data."), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	params.isolate = isolate;

	String::Utf8Value cert( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.cert = *cert;
	params.certlen = cert.length();

	Local<Value> xx = Expiration( &params );
	args.GetReturnValue().Set( xx );


}


void  vtLogBinary( PVARTEXT pvt, uint8_t* buffer, size_t size  )
{
	size_t nOut = size;
	uint8_t* data = buffer;
	// should make this expression something in signed_usigned_comparison...
	while( nOut && !(nOut & (((size_t)1) << ((sizeof( nOut ) * CHAR_BIT) - 1))) )
	{
		size_t x;
		for( x = 0; x<nOut && x<16; x++ )
			vtprintf( pvt, "%02X ", (unsigned char)data[x] );
		// space fill last partial buffer
		for( ; x < 16; x++ )
			vtprintf( pvt, "   " );

		for( x = 0; x<nOut && x<16; x++ )
		{
			if( data[x] >= 32 && data[x] < 127 )
				vtprintf( pvt, "%c", (unsigned char)data[x] );
			else
				vtprintf( pvt, "." );
		}
		vtprintf( pvt, "\n" );
		data += x;
		nOut -= x;
	}
}


static Local<Value> CertToString( struct info_params *params ) {

	X509 * x509 = NULL;
	params->ca = NULL;

	BIO *keybuf = BIO_new( BIO_s_mem() );

	BIO_write( keybuf, params->cert, params->certlen );
	if( !PEM_read_bio_X509( keybuf, &x509, NULL, NULL ) ) {
		throwError( params, "expiration : failed to parse cert" );
		BIO_free( keybuf );
		return Undefined( params->isolate );// goto free_all;
	}

	PVARTEXT pvt = VarTextCreate();

	char *subj;
	subj = X509_NAME_oneline( X509_get_subject_name( x509 ), NULL, 0 );
	char *issuer;
	issuer = X509_NAME_oneline( X509_get_issuer_name( x509 ), NULL, 0 );

	vtprintf( pvt, "OneLiners (cert to verify):\n%s\n%s\n", subj, issuer );


	ASN1_INTEGER *val;
	val = X509_get_serialNumber( x509 );
	long serial;
	serial = ASN1_INTEGER_get( val );

	vtprintf( pvt, "serial : %d\n", serial );

	vtprintf( pvt, "Subject Identity --------------\n " );
	{
		X509_NAME *name = X509_get_subject_name( x509 );
		int n;
		int count = X509_NAME_entry_count( name );

		if( 0 )  // 957 as of the time this was written... lots of strings.
			for( n = 0; n < 960; n++ ) {
				vtprintf( pvt, "NID: %d  = %32s     %s\n", n, OBJ_nid2sn( n ), OBJ_nid2ln( n ) );
			}


		//int pkey_nid = OBJ_obj2nid( EVP_Get X509_PUBKEY_get0( X509_get_X509_PUBKEY(  x509 ) )->algor->algorithm );
		//vtprintf( pvt, "algo nid = %d\n", pkey_nid );

		int valid = X509_check_ca( x509 );

		vtprintf( pvt, "valid ca:%d\n", valid );

		for( n = 0; n < count; n++ )
		{
			X509_NAME_ENTRY *entry = X509_NAME_get_entry( name, n );
			ASN1_OBJECT *o = X509_NAME_ENTRY_get_object( entry );
			ASN1_STRING *v = X509_NAME_ENTRY_get_data( entry );
			int nid = OBJ_obj2nid( o );
			vtprintf( pvt, "Field: %d  %s %s %*.*s\n"
				, nid, OBJ_nid2ln( nid ), OBJ_nid2sn( nid )
				, v->length, v->length, v->data );
			vtLogBinary( pvt, v->data, v->length );
		}
		//stack_st_X509_NAME_ENTRY
		//name->entries
	}

	vtprintf( pvt, "Issuer Identity -------------- \n" );
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
			vtprintf( pvt, "Field: %d  %s %s %*.*s\n"
				, nid, OBJ_nid2ln( nid ), OBJ_nid2sn( nid )
				, v->length, v->length, v->data );
			vtLogBinary( pvt, v->data, v->length );
			//X509_NAME_oneline
			//X509_NAME_add_entry_by_txt
			//name->
		}
		//stack_st_X509_NAME_ENTRY
		//name->entries
	}

	const ASN1_TIME *before = X509_get_notBefore( x509 );
	const ASN1_TIME *after = X509_get_notAfter( x509 );
	if( before ) {
		struct tm t;
		ConvertTimeString( &t, before );
		vtprintf( pvt, "Not Before: %s %4d/%02d/%02d %02d:%02d:%02d\n", before->data, t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec );
	}
	else
		vtprintf( pvt, "Not Before: undefined\n" );
	if( after ) {
		struct tm t;
		ConvertTimeString( &t, after );
		vtprintf( pvt, "Not After: %s %4d/%02d/%02d %02d:%02d:%02d\n", after->data, t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec );
	}
	else
		vtprintf( pvt, "Not After: undefined\n" );

	vtprintf( pvt, "Extensions ------------ \n" );
	{
		int extCount = X509_get_ext_count( x509 );
		int n;
		for( n = 0; n < extCount; n++ )
		{
			X509_EXTENSION *ext = X509_get_ext( x509, n );
			ASN1_OBJECT *o = X509_EXTENSION_get_object( ext );
			ASN1_STRING *v = (ASN1_STRING *)X509V3_EXT_d2i( ext );
			int nid = OBJ_obj2nid( o );
			//V_ASN1_OCTET_STRING
			vtprintf( pvt, "extension: %d %d  %s  %s  %d\n", X509_EXTENSION_get_critical( ext )>0 ? "critical" : "",
				nid, OBJ_nid2ln( nid ), OBJ_nid2sn( nid ), v->type );
			if( nid == NID_authority_key_identifier ) {
				AUTHORITY_KEYID *akid = (AUTHORITY_KEYID *)v;
				//akid->serial
				GENERAL_NAMES *names = akid->issuer;
				ASN1_OCTET_STRING *key = akid->keyid;
				ASN1_INTEGER *ser = akid->serial;
				vtLogBinary( pvt, key->data, key->length );
				long val = ASN1_INTEGER_get( ser );
				vtprintf( pvt, "Serial is:%d\n", val );
			}
			else if( nid == NID_subject_key_identifier ) {
				ASN1_OCTET_STRING *skid = (ASN1_OCTET_STRING *)v;
				vtLogBinary( pvt, skid->data, skid->length );
			}
			else if( nid == NID_basic_constraints ) {
				BASIC_CONSTRAINTS *bc = (BASIC_CONSTRAINTS *)v;
				long pathlen = bc->pathlen ? ASN1_INTEGER_get( bc->pathlen ) : 0;
				vtprintf( pvt, "Basic Constraint: CA? %d   %s%d\n", bc->ca, bc->pathlen ? "pathlen:" : "ignore:", pathlen );
				//X509_get_ext_d2i( x, NID_subject_key_identifier, NULL, NULL );
				//X509V3_get_d2i( x->cert_info->extensions, nid, crit, idx );
				// 0x55,0x1D,0x13
			}
			else if( nid == NID_key_usage ) {
				//ASN1_BIT_STRING *s = (ASN1_BIT_STRING *)X509_get_ext_d2i( x509, NID_key_usage, NULL, NULL );
				int usage = v->data[0];
				vtLogBinary( pvt, v->data, v->length );
				if( v->length > 1 )
					usage |= v->data[1];
				vtprintf( pvt, "USAGE: %08x\n", usage );
			}
			else if( nid == NID_subject_alt_name ) {
				const GENERAL_NAMES *names = (const GENERAL_NAMES*)v;
				int n;
				for( n = 0; n < sk_GENERAL_NAME_num( names ); n++ ) {
					GENERAL_NAME *namePart = sk_GENERAL_NAME_value( names, n );
					switch( namePart->type ) {
					case GEN_DIRNAME:
						{
						X509_NAME *dir = namePart->d.directoryName;
						//int dn;
						//for( dn = 0; dn < dir->entries; dn++ ) {

						//}
						vtprintf( pvt, "unhandled DIRNAME\n" );
						}
						break;
					case GEN_DNS:
						{
							ASN1_IA5STRING *string = namePart->d.dNSName;
							vtLogBinary( pvt, string->data, string->length + 1 );
							vtprintf( pvt, "DNS:%s\n", string->data );
						}
						break;
					case GEN_EDIPARTY:
						vtprintf( pvt, "unhandled EDIPARTY\n" );
						break;
					case GEN_OTHERNAME:
						vtprintf( pvt, "unhandled OTHERNAME\n" );
						break;
					case GEN_RID:
						vtprintf( pvt, "unhandled RID\n" );
						break;
					case GEN_URI:
					{
						ASN1_IA5STRING *name = namePart->d.uniformResourceIdentifier;
						vtLogBinary( pvt, name->data, name->length );
					}
						break;
					case GEN_X400:
						vtprintf( pvt, "unhandled X400\n" );
						break;
					case GEN_EMAIL:
					{
						ASN1_STRING *string = namePart->d.rfc822Name;
						vtLogBinary( pvt, string->data, string->length );
						vtprintf( pvt, "EMAIL:%*.*s", string->length, string->length, string->data );
					}
						break;
					case GEN_IPADD:
						vtprintf( pvt, "unhandled IPADD\n" );
						break;
					}
				}

			}
			else {
				vtprintf( pvt, "unhandled nid...\n" );
				if( v->data )
					vtLogBinary( pvt, v->data, v->length );
			}
		}
	}
	PTEXT out = VarTextPeek( pvt );
	Local<Value> outval = String::NewFromUtf8( params->isolate, GetText( out ), v8::NewStringType::kNormal ).ToLocalChecked();
	return outval;
}


void TLSObject::certToString( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct info_params params;

	int argc = args.Length();
	if( !argc ) {
		isolate->ThrowException( Exception::Error(
			String::NewFromUtf8( isolate, TranslateText( "Missing required certificate data." ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}
	params.isolate = isolate;

	String::Utf8Value cert( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.cert = *cert;
	params.certlen = cert.length();

	Local<Value> xx = CertToString( &params );
	args.GetReturnValue().Set( xx );
}



#ifdef _WIN32

void updateRootCert( BYTE *data, DWORD dataLen ) {
#if 0
   // CA, MY, ROOT, SPC
   HCERTSTORE hRootCertStore = CertOpenSystemStore( NULL, "ROOT" );
   CertAddEncodedCertificateToStore( hRootCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
			, data, dataLen, CERT_STORE_ADD_USE_EXISTING, NULL );
   CertCloseStore( hRootCertStore, 0 );
#endif
   //CertAddEncodedCertificateToSystemStore( "ROOT"
	//		, data, dataLen );
}


#endif
