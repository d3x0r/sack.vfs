
# Object Storage


## Get some storage....

The factory of ObjectStorage takes the name of a file.  The file may be a full path name.  (TODO: Filenames may reference virtual filesystems).

```
	const storage = sack.ObjectStorage( "data.os" );
```

## Application configuration

Somewhere to save the IDs and actual objects of things is required.

An application's configuration state can be created like this...


### Save a configuration in the 'root' filesystem

This fragment of code sets up some storage for a few lookup tables which will be reloaded later.
The `configValues` object stores the IDs of the objects to load.  These are saved in a configuration file
and can be reloaded as an object.

``` js

const configValues = {
	accountId : null,
}

const config = {
	ids: configValues,
        
	accountId : null,
}


async function loadStorage() {
	const root = await storage.getRoot();
	try {
		const file = await root.open( "config.jsox" );
		const obj = await file.read()

		// merge current configuration with what was saved...
		Object.assign( configValues, obj );

		// reload the accountID lookup table.                
		config.accountId   = await storage.get( configValues.accountId );
                
                
	} catch(err){
		const file = await root.create( "test.config.jsox" );
                
      // create the table to store account ID to account object lookup
      config.accountId = new BloomNHash()

		// store the index, save the ID for later
      configValues.accountId = await storage.put( config.accountId );
       
      // save the object's identifier for later use.
		file.write( configValues );
	} ;
}


```

### Store Native Config Object

Alternatively you can just store any arbitrary object in object storage itself
instead of working through the file storage...



``` js

const config = {
	accountId : null,
	emailId : null,
}


async function loadStorage() {
	const oldConfig = await storage.get( {id: "myConfig" } );
	if( !oldConfig ) {
		storage.put( config, "myConfig" );
	}
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

Objects stored or loaded, by default, are generically just 'Objects'.  Specific types of
objects may be stored, and reloaded as their appropriate types automatically by JSOX, if
handlers for the user data types are specified.

``` js
class User {
	username = 'default Name';
   constructor() {
   }
}

```

And register storage handlers with object storage...

```
storage.addEncoders( [ { tag:"~U", p:User, f: null } ] );

storage.addDecoders( [ { tag:"~U", p:User, f: null } ] );
```


Then when later stored, `User` objects will be denoted with '~U' as their data type in the JSOX output....

``` js

   const user = new User();
   user.username = "Correct Value";
   const userId = await storage.put( user );
```

