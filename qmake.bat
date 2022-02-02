@echo off


rem Environnement
rem -------------

if not exist "%~dp0/env.bat" (
    echo env.bat file does not exist ! Please copy env.bat.proto to env.bat and set up your environment.
    goto end
)

call %~dp0/env.bat


rem vcvarsall.bat
rem -------------
if not exist "%VCVARSALL_DIR%\vcvarsall.bat" (
    echo Unable to find vcvarsall.bat. Please verify the env.bat content.
    goto end
)

call "%VCVARSALL_DIR%\vcvarsall.bat" %BUILD%


rem qmake
rem -----

if not exist "%QT_DIR%\bin\qmake.exe" (
    echo Unable to find qmake.exe. Please verify the env.bat content.
    goto end
)

cd %~dp0/code
rem rm .qmake.stash
"%QT_DIR%\bin\qmake.exe" -tp vc -r SCEP.pro

:end
