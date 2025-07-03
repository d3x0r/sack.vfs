


# WIFI Interface (WIN32)


## Interfaces

`sack.WIFI.interfaces`

results with an array of objects.

```
[
   {
      device: <string hardware device name>
      , name : <friendly name for wifi adapter>
      , state : <numeric code of state, updates every time interfaces is fetched>
      , status : <string representing state>

   }
   , ...
}
```

## Events

`sack.WIFI.onEvent( cb )`

`cb` is a function that is called for each WIFI event.

The callback function is passed an event object with the following fields

``` js
{
   code: <Numberic code for event - from 
   codeText: <Unique String for each code>
   source: <Numberic code for source - from L2_NOTIFICATION_SOURCE_* list>
   sourceText: <String for source type>
   interface: <Numeric index into interface array>
}
```
   
``` js

   mode : <numeric code for operational state change - unknown specification>
   
   profile: <string profile name, if relavent for operation
   SSID : <string SSID name, if relevant for operation
```

   


## Connect 

`sack.WIFI.connect( device number or name, SSID, Profile )`

returns with numeric error code, 0 is ERROR_SUCCESS


## Disconnect

`sack.WIFI.connect( device number or name )`

returns with numeric error code, 0 is ERROR_SUCCESS

