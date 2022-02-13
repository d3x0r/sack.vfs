
# Object Storage Proxy

This service does not itself save any information, but it responds as if it did; instead, this service forwards requests to other storage services.

Requests to other storage services are scheduled such that the best server to respond is asked for information first; something like auto best connection/load balancing of requests.



## Configuration

`config.jsox` file has a list of storage servers to connect to; and login servers to register with.

## Operation

- Connect to storage servers.
- while connected, if any servers receive a put, mark on all other servers that a put failed.
- if a server comes back online with a put that failed, and a server is online that the put is not also failed, then ask the server with the put that did not fail for the item.
  - if the item matches, ask another server.
  - if the item is different, store the new item, and clear the put failure.
  - if another put happens, before the put is resolved, use the newst put value, and clear put failure.


