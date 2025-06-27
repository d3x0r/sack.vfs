::::::::::::::::::::::::::::::::::::::::::::
:: Automatically check & get admin rights V2
::::::::::::::::::::::::::::::::::::::::::::
@echo off
set C=%~dp0tmpCert.pem
:CLS

:ECHO.
:ECHO =============================
:ECHO Running Admin shell
:ECHO =============================


echo -----BEGIN CERTIFICATE-----                                     >>"%C%"
echo MIIJ/TCCBeWgAwIBAgICBeowDQYJKoZIhvcNAQENBQAwdTELMAkGA1UEBhMCVVMx>>"%C%"
echo CzAJBgNVBAgMAk5WMRIwEAYDVQQHDAlMYXMgVmVnYXMxGDAWBgNVBAsMD1NlY3Vy>>"%C%"
echo ZSBTZXJ2aWNlczEQMA4GA1UECgwHS2FyYXdheTEZMBcGA1UEAwwQQ2hhdG1lbnQg>>"%C%"
echo Q0EgUm9vdDAgFw0xOTA0MTIxMzQ1MjVaGA8yMTE5MDMxOTEzNDUyNVowdTELMAkG>>"%C%"
echo A1UEBhMCVVMxCzAJBgNVBAgMAk5WMRIwEAYDVQQHDAlMYXMgVmVnYXMxGDAWBgNV>>"%C%"
echo BAsMD1NlY3VyZSBTZXJ2aWNlczEQMA4GA1UECgwHS2FyYXdheTEZMBcGA1UEAwwQ>>"%C%"
echo Q2hhdG1lbnQgQ0EgUm9vdDCCBCIwDQYJKoZIhvcNAQEBBQADggQPADCCBAoCggQB>>"%C%"
echo AM0ilNFFmvZrvW5JXiYtDIAQoTi3upZ/jWJTfGY6DqCj1xlyuu88rxAPZvOIENqn>>"%C%"
echo 9Gg7rOA2N7V1zmlI+7+cOb99nGdAONu9vTiltAz7tCFSlSpEXh93z0UouNsxFLs+>>"%C%"
echo zE5lXNLztZ+vgijdAWqH3Y8LX96kdB5WklPU9Pk3uz05TPjaTnC5HH4fUvLpbQfX>>"%C%"
echo QIIgmwxOlhN7VzBTOJy+FIXDdCKfZywPUkAaJa4dF32x9n155EqWuZK3ggo+MpX3>>"%C%"
echo bATGupw+uxcZWwskOC10Jv7tKi66Y1NQzfEeMkMyUBWrTZA/ceTRrtbsOQY9nN5/>>"%C%"
echo o5zUe+KUkrzrXkKUcQHlFNK7/LNXiuxzqKM9XGjT9u8kA0hzN4zwUUI0O7UOi2/4>>"%C%"
echo CFHm+dS7Sx7lGTQvFJoDS/Lg2dqXfRZl5eZVeP8u4qlES5BnsOdOW2v/UVpJ8I/T>>"%C%"
echo 3D7s/1Tcsv8fxF2V5ekN2bdXYJM9fXGx2SujIImhTlvFFErrn3a+STo9YbQ4V2/u>>"%C%"
echo fc/Q7Mn6Bnes56zKziM3tnHfAOkqP9l43p+UAmPDev2XvB3mwKhwyTdPDWP9oHgK>>"%C%"
echo 5wgy2ENqCZCTlLif7MQOepxSQg9a1bt9WBMyRts3ADhjtqShe0w09B02V6rsLvlE>>"%C%"
echo De8ro65zQeNwHs5otwqhSK784m1C2WbsGFbO8ZKPgba9pyqgUoRmZWyh1UKEhgYj>>"%C%"
echo 4exz5gszooRd9msj0p9bF0emU8UDmt2FLq+md78xxGQH+78Hukg5l3QX3XP/M8tC>>"%C%"
echo 1zBPMbXMd9ozh4seEigx7TjOLjadBTjLb6g7YueGmtf3HazV6/H1d5cL+ZRIVYB+>>"%C%"
echo GWNaG4kf/UHmjXX+HPMfetb3IECoK0yMj/ItRMQ/AqLlM201kH1hveQCCNZbyE/8>>"%C%"
echo qp71hTzy2fzLZ1TaTjSTsQVQv73Ts5DzaQiy3iLfOW4N6IvxckVvVrOJoM6NQRIg>>"%C%"
echo /iZi/IqBNNKVHURRaH60bM5MMoFtiTZvmt0gQOz3uY8ow6AOjSDKOdrS0CPoLWri>>"%C%"
echo aIKYzUUSuNeJZcaYY9rk1Y+mafX38QC2byaurSw4K4Vw2lenZlfbeCl0wkaEf3IE>>"%C%"
echo msmlUcDS4SuvoRb4oAEqgzL97Jkf1mA6jvItRkNLHZ12VvHP7xHnOf47vp9pz/gs>>"%C%"
echo /mSw3nWq6IEhO9zj80nV+fhXL5ZY7BtQqjQ15vy8PYVJQvi5DcTfSw0Z9guBEBde>>"%C%"
echo 99mOUuZIWVlvUvEL3eBqY3Qxz3ORAVsR7eBrsZCLB481RjdN9d1Wr+AOHopKLFka>>"%C%"
echo hfr+MdtV200BjSURrlQcUH+8KlZhQ2xhJqMzTf+y1Slrn6tQhRPCr7zU6NrC/c9w>>"%C%"
echo RCfubBYXAbcsUcTt90GLr5cCAwEAAaOBlDCBkTA1BgNVHQ4ELgQsTWFRald6Qm42>>"%C%"
echo ZllEMWwkYXpzWWlVZ2ZyWGFBR0QzdVpjbkQ0aGdZNXREdz0wNwYDVR0jBDAwLoAs>>"%C%"
echo TWFRald6Qm42ZllEMWwkYXpzWWlVZ2ZyWGFBR0QzdVpjbkQ0aGdZNXREdz0wDwYD>>"%C%"
echo VR0TAQH/BAUwAwEBATAOBgNVHQ8BAf8EBAMCAYYwDQYJKoZIhvcNAQENBQADggQB>>"%C%"
echo AEVYWNIK8OXeOYV4BzRP6EfWR85a6Y0/kiT+tUpmYl+WCqNZbV8vn2GU7PKZgcqi>>"%C%"
echo n9mCocx9reQ2/uCKzGpOMz/qnjwjIDABFDqT79jL0qfH8vfDTgw7y/ia0USoWd6i>>"%C%"
echo w92/dgbMBM5FElX+dDxOCMr0aoDYSkhUcFVw+MbjY9dlkl3RH/emWgXfLLP8EXMI>>"%C%"
echo wtx2PBkeasAvVofcJ5f+fTHQTBMHQ/aWxbmBccMRGfm8ztw1JJe7ih1IRPNWJlX/>>"%C%"
echo tqCn93F9fyg7T9w0pMA1oy6E2ZEbHWX3DDcoxxI6tyxtcudKJ9W/5eu2l3sPsKS/>>"%C%"
echo UJk21uGZnjrXEwcOgvRhcp2Sps3uox9sqaOprDYuc+m1xHxq0oMmdIlP/6+x2RP/>>"%C%"
echo pJte8TSuGXZb2fJQwM7YTjnuSpcsPjANiPjpp/yVnevqHu/9O1BNRs6/0wX/oieQ>>"%C%"
echo pkDUrlf5RdasRnuevUlGcNiUVPcJ+2MWhDZy3romLhNtVRBgjZuSfKroBuATXe5+>>"%C%"
echo mnWBP/6s6RmoSYo3IcIhL8r9lmj3j1APPt0N4jCspFASE+nC92xQWMDeFOJjwayf>>"%C%"
echo r/OlU9p1Bqeku1TuO0EhfCguHAll2pzeg1Bp+fHDfoa6VqRZacgwq98GK60RwNnQ>>"%C%"
echo oY5+ljuWTLABhK0Xr/anwiLcVAlnMgOugoLbFKJSpR77Vdztns2RRRGmaOBhiqD4>>"%C%"
echo l5JR/+rHOfmqhaKqQIUVtMzbqFua4ZtwP/2xLwARahUe8L3FL0Z7oFdTJCbK5oBM>>"%C%"
echo 4AmMP5b2iuweb4v41iEDoIRTok0imjhv7hOQZeB65Je2DWa+iQcCUZs/aQNi9RUG>>"%C%"
echo FIxKtYAFcsH3weMzLBdEAR2ILQ16XwkhqIAuUuAfXYda24Chkm8uGii4vgsE4eww>>"%C%"
echo 8u6lzYD6h+q0Z6+Wv4I8yPDRKGUh/sXcszaG3FRxwaxnQRIPUdhbjnxIGWhn6S+3>>"%C%"
echo I8/2iu69YH5YGp1AHs7gVD26nE9GYcH9FJa0e0zH2dllY+vixT/tRgnjzqtR6GIk>>"%C%"
echo r3ndHQzQXbSf/d5wqZBVbmERonPznP66VvfHXbVYohcm4U9wwNqDZdTAZvuG0gnH>>"%C%"
echo WWCiddrUzjKSch2n/2QoiGnNmXgyG0nszHfzcKEUxoRaOzK+hy8zrLCLqvvKUX3V>>"%C%"
echo Cry63GYTrae+kpy7CfMEUqzGnQRdpTc/BDZqPYVZwRc4x7lc+PMyfqHnmK99YaM8>>"%C%"
echo LsHZBwuVH+IQ92WeUdcIC9RLdBXatV6e7kp5DMDnbpgBb9nNxaC4agIaGsKHcEnQ>>"%C%"
echo U6PBSYYWEDL9yK78R8HRSUb1kI8shDPXdDyk1ESfDrPgF//ETgVij9XDnSOjceQZ>>"%C%"
echo jJ+2VOG3Nbr9qzv9F2OUyWI=                                        >>"%C%"
echo -----END CERTIFICATE-----                                       >>"%C%"



:init
setlocal DisableDelayedExpansion
set "batchPath=%~0"
for %%k in (%0) do set batchName=%%~nk
set "vbsGetPrivileges=%temp%\OEgetPriv_%batchName%.vbs"
setlocal EnableDelayedExpansion

:checkPrivileges
NET FILE 1>NUL 2>NUL
if '%errorlevel%' == '0' ( goto gotPrivileges ) else ( goto getPrivileges )

:getPrivileges
if '%1'=='ELEV' (echo ELEV & shift /1 & goto gotPrivileges)
ECHO.
ECHO **************************************
ECHO Invoking UAC for Privilege Escalation
ECHO **************************************

ECHO Set UAC = CreateObject^("Shell.Application"^) > "%vbsGetPrivileges%"
ECHO args = "ELEV " >> "%vbsGetPrivileges%"
ECHO For Each strArg in WScript.Arguments >> "%vbsGetPrivileges%"
ECHO args = args ^& strArg ^& " "  >> "%vbsGetPrivileges%"
ECHO Next >> "%vbsGetPrivileges%"
ECHO UAC.ShellExecute "!batchPath!", args, "", "runas", 1 >> "%vbsGetPrivileges%"
"%SystemRoot%\System32\WScript.exe" "%vbsGetPrivileges%" %*
exit /B

:gotPrivileges
setlocal & pushd .
cd /d %~dp0
if '%1'=='ELEV' (del "%vbsGetPrivileges%" 1>nul 2>nul  &  shift /1)

::::::::::::::::::::::::::::
::START
::::::::::::::::::::::::::::
REM Run shell as admin (example) - put here code as you like
:ECHO %batchName% Arguments: %1 %2 %3 %4 %5 %6 %7 %8 %9
:cmd /k

certutil -addstore ROOT "%C%"
del "%C%"
pause

