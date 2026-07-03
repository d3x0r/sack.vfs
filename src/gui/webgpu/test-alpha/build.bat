@echo off
:REM Run from a "Developer Command Prompt for VS 2022" (or similar).
:REM No Vulkan SDK needed — we LoadLibrary("vulkan-1.dll") at runtime
:REM and use the Vulkan headers from Dawn's vendored tree.

setlocal

:REM Prefer the installed Vulkan SDK if present (cleaner headers, easy
:REM to enable validation layers later). Fall back to Dawn's vendored
:REM headers — both work since we dynamically load vulkan-1.dll.
if defined VULKAN_SDK (
    set VK_HEADERS=%VULKAN_SDK%\Include
    echo Using Vulkan SDK at %VULKAN_SDK%
) else if exist "C:\tools\VulkanSDK\1.4.328.1\Include\vulkan\vulkan.h" (
    set VK_HEADERS=C:\tools\VulkanSDK\1.4.328.1\Include
    echo Using Vulkan SDK at C:\tools\VulkanSDK\1.4.328.1
) else (
    set VK_HEADERS=..\..\..\..\build\Dawn_External-prefix\src\Dawn_External\third_party\vulkan-headers\src\include
    echo Using Dawn-vendored Vulkan headers
)

if not exist "%VK_HEADERS%\vulkan\vulkan.h" (
    echo Cannot find Vulkan headers at: %VK_HEADERS%
    exit /b 1
)

cl /nologo /std:c++17 /EHsc /W3 /MD ^
   /I"%VK_HEADERS%" ^
   test_alpha.cpp ^
   user32.lib gdi32.lib

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo.
echo Built test_alpha.exe.  Run it and drag the window over a bright
echo background. ESC to quit.
