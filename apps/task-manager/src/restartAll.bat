cd %~dp0
call ..\service\restartSideplayrService.bat
call ..\%SITE%-ui\restart.bat
