
1) app has SIGINT that is ignored.
  - no SIGINT in library, doesn't shut down the event threads that are keeping it open in the first place.
  - SIGINT in library, which prevents or forces exit (if it's the only handler, no exit, if it says exit() then

2) SIGINT in application - shuts down event loops (clearTimeout( tick ) )
  - no SIGINT in library, library gets beforeExit and exit.

3) SIGINT in application - does process.exit()
  - no SIGINT in library, library gets NO beforeExit and only exit.



4) no beforeExit event from ctrl-C on linux.
  - unless, there is a SIGINT handler.

## Linux

test.mjs - SIGINT, stopTimer, gracefully exits shorterly after ctrl-C; beforeExit, exit, but no SIGINT in addon. Addon does get node::AddEnvironmentCleanupHook(), (or node::atExit() ).

test1.mjs - SIGINT, process.exit(0), generates exit on ctrl-C; no beforeExit, no exit, and no SIGINT in addon.  Addon does NOT get node::AddEnvironmentCleanupHook(), (or node::atExit() )

test2.mjs - no SIGINT, exits on ctrl-C without beforeExit, (addon without SIGINT gets nothing); no beforeExit, no exit, and no SIGINT in addon. Addon does get node::AddEnvironmentCleanupHook(), (or node::atExit() )



