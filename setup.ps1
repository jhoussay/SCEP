#Set-PSDebug -Off

# Environnement
# -------------

$env = "$PSScriptRoot\env.ps1"
if (-not(Test-Path -Path $env -PathType Leaf))
{
    echo env.ps1 file does not exist ! Please copy env.ps1.proto to env.ps1 and set up your environment.
    Exit
}

& $env

# vcvarsall.bat
# -------------
$vcvarsall = "$env:VCVARSALL_DIR\vcvarsall.bat"
if (-not(Test-Path -Path $vcvarsall -PathType Leaf))
{
    echo Unable to find vcvarsall.bat. Please verify the env.bat content.
    Exit
}

cmd.exe /c "call `"$vcvarsall`" $env:BUILD && set > %temp%\vcvars.txt"
Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
  if ($_ -match "^(.*?)=(.*)$") {
    Set-Content "env:\$($matches[1])" $matches[2]
  }
}


# windeployqt.bat
# ---------------
$DEPLOY_DIR = "$PSScriptRoot\bin\release"
& windeployqt --force --no-compiler-runtime --no-system-d3d-compiler "$DEPLOY_DIR\SCEP.exe"


# deploy redist
# -------------
# PLATFORM_TOOLSET_LIGHT -> 140
$PLATFORM_TOOLSET_LIGHT=$env:VCToolsVersion.Substring(0,2) + "0"
# PLATFORM_TOOLSET_FULL -> 143
$PLATFORM_TOOLSET_FULL=$env:VCToolsVersion.Substring(0,2) + $env:VCToolsVersion.Substring(3,1)
$inputDir = "$env:VCToolsRedistDir\$env:VSCMD_ARG_TGT_ARCH\Microsoft.VC$PLATFORM_TOOLSET_FULL.CRT"
$files = "msvcp${PLATFORM_TOOLSET_LIGHT}.dll", "msvcp${PLATFORM_TOOLSET_LIGHT}_1.dll", "msvcp${PLATFORM_TOOLSET_LIGHT}_2.dll", "vcruntime${PLATFORM_TOOLSET_LIGHT}.dll", "vcruntime${PLATFORM_TOOLSET_LIGHT}_1.dll"
foreach ( $file in $files )
{
    copy -Force "$inputDir\$file" $DEPLOY_DIR
}

# build setup
# -----------
# msbuild does not support building vdproj projects, falling back to devenv.com
& devenv.com "$PSScriptRoot\setup\SCEP\SCEP.sln" /Rebuild "Release|Default"
