@echo off
REM No XR_RUNTIME_JSON here on purpose. Overriding the runtime per-process
REM stopped working after a SteamVR update: SteamVR's xrCreateInstance returns
REM XR_ERROR_RUNTIME_FAILURE (-2) whenever it is not the *registered* runtime,
REM and the registered one then has no headset to offer because SteamVR is
REM holding it. Pick the runtime once, in SteamVR Settings -> OpenXR or in the
REM Meta app; the loader reads HKLM\SOFTWARE\Khronos\OpenXR\1\ActiveRuntime.

node first-light.mjs
