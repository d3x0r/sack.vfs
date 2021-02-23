
# Object Storage


## Get some storage....

The factory of ObjectStorage takes the name of a file.  The file may be a full path name.  (TODO: Filenames may reference virtual filesystems).

```
	const storage = sack.ObjectStorage( "data.os" );
```

## Application configuration

Somewhere to save the IDs and actual objects of things is required.

An application's configuration state can be created like this...

``` js

const configValues = {
	accountId : null,
	emailId : null,

	commit : null, // this is filled in later to save configuration information
}

const config = {
	ids: configValues,
        
	accountId : null,
	emailId : null,
}



async function loadStorage() {
	const root = await storage.getRoot();
	try {
		const file = await root.open( "config.jsox" );
		const obj = await file.read()
		//console.log( "reloadConfig:", obj );
		config.commit = ()=>file.write(config);
		Object.assign( configValues, obj );
                
		config.accountId   = await storage.get( configValues.accountId );
                
                
	} catch(err){
		console.log( "Error is:", err );
		const file = await root.create( "test.config.jsox" );
                
                // create some stuff...
                config.accountId = new BloomNHash()
                configValues.accountId = await storage.put( config.accountId );
                
                // write initial values.
		file.write( configValues );
		config.commit = ()=>file.write(configValues);
	} ;
}


```


## Have some data to store...

```
    const user = { user: "user", password: "passhash" };
```



## Set the user data in a index...

```

	storage.put( user ); // you don't have to care about the ID... 
        
	config.accountId.set( user.userId, user );

```




## store something.

Store the object.

```

   const putPromise = storage.put( user );
   putPromise.then(  (id)=>{
   	// the ID parameter here is what the object was stored as.
   } );

```




## User types stored as data


### Define a type

### Add encoder/decoder tags for type


```
		storage.addEncoders( [ { tag:"~U", p:User, f: null },  { tag:"~D", p:Device, f: null },  { tag:"~I", p:UniqueIdentifier, f: null } ] );
		storage.addDecoders( [ { tag:"~U", p:User, f: null },  { tag:"~D", p:Device, f: null },  { tag:"~I", p:UniqueIdentifier, f: null } ] );
```


