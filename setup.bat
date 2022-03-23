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


rem windeployqt.bat
rem ---------------
set DEPLOY_DIR=%~dp0\bin\release
call windeployqt --force --no-compiler-runtime --no-system-d3d-compiler %DEPLOY_DIR%\SCEP.exe


rem deploy redist
rem -------------
rem PLATFORM_TOOLSET_LIGHT -> 140
set PLATFORM_TOOLSET_LIGHT=%VCToolsVersion:~0,2%0
rem PLATFORM_TOOLSET_FULL -> 143
set PLATFORM_TOOLSET_FULL=%VCToolsVersion:~0,2%%VCToolsVersion:~3,1%
copy /Y "%VCToolsRedistDir%\%VSCMD_ARG_TGT_ARCH%\Microsoft.VC%PLATFORM_TOOLSET_FULL%.CRT\msvcp%PLATFORM_TOOLSET_LIGHT%.dll" %DEPLOY_DIR%
copy /Y "%VCToolsRedistDir%\%VSCMD_ARG_TGT_ARCH%\Microsoft.VC%PLATFORM_TOOLSET_FULL%.CRT\msvcp%PLATFORM_TOOLSET_LIGHT%_1.dll" %DEPLOY_DIR%
copy /Y "%VCToolsRedistDir%\%VSCMD_ARG_TGT_ARCH%\Microsoft.VC%PLATFORM_TOOLSET_FULL%.CRT\msvcp%PLATFORM_TOOLSET_LIGHT%_2.dll" %DEPLOY_DIR%
copy /Y "%VCToolsRedistDir%\%VSCMD_ARG_TGT_ARCH%\Microsoft.VC%PLATFORM_TOOLSET_FULL%.CRT\vcruntime%PLATFORM_TOOLSET_LIGHT%.dll" %DEPLOY_DIR%
copy /Y "%VCToolsRedistDir%\%VSCMD_ARG_TGT_ARCH%\Microsoft.VC%PLATFORM_TOOLSET_FULL%.CRT\vcruntime%PLATFORM_TOOLSET_LIGHT%_1.dll" %DEPLOY_DIR%


rem build setup
rem -----------
rem msbuild does not support building vdproj projects, falling back to devenv.com
devenv.com %~dp0\setup\SCEP\SCEP.sln /Rebuild "Release|Default"

:end
