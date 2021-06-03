
# Object Storage Proxy

This service does not itself save any information, but it responds as if it did; instead, this service forwards requests to other storage services.

Requests to other storage services are scheduled such that the best server to respond is asked for information first; something like auto best connection/load balancing of requests.




