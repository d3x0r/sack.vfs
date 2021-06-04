
# Object Storage Service

This service only serves to store objects in physical storage.   It allows remote clients and proxies to connect and share the same storage.

Objects may be 'watched' by clients, such that when they are changed, they can receive an event notification of the change.

## Issues to solve

- watch lists may become very large (consider reddit or facebook wall for every user currently viewing the site)
- without noting that someone is interested in a change, changes may not be propagated.
- storage servers which were offline when some `PUT` commands happen won't get those changes, and may serve obsolete information if they are the preferred provider.

- proxies can watch 'put' things and when things come back, then tell them to request PUT from other storages (if any?)
- proxies can watch storage services that go away and come back - and mark them unreliable, so anything they reply with has to be validated with someone else's get request.
  - when can distrust be removed?  Eventually something that wasn't in sync will mismatch but I don't know what that might have been (see watch PUT)

