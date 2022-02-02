@echo off


rem Environnement
rem -------------

if not exist "%~dp0/env.bat" (
    echo env.bat file does not exist ! Please copy env.bat.proto to env.bat and set up your environment.
    goto end
)

call %~dp0/env.bat


rem devenv
rem ------

if not exist "%DEVENV_DIR%\devenv.exe" (
    echo Unable to find devenv.exe. Please verify the env.bat content.
    goto end
)

start "Visual Studio" "%DEVENV_DIR%\devenv.exe" %~dp0/code/SCEP.sln

:end
