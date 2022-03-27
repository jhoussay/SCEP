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

# qmake.exe
# ---------

$qmake = "$env:QT_DIR\bin\qmake.exe"
if (-not(Test-Path -Path $qmake -PathType Leaf))
{
    echo Unable to find qmake.exe. Please verify the env.bat content.
    Exit
}

# date
# ----
$date = (Get-Date -Format "dd/MM/yyyy")
$date_header = "code\SCEP\include\SCEP\Date.h"
cd "$PSScriptRoot"
"#pragma once" > $date_header
"static const char* SCEP_DATE = ""$date"";" >> $date_header

# version
# -------
cd "$PSScriptRoot"
$version_header = "code\SCEP\include\SCEP\Version.h"
"#pragma once" > $version_header
Get-Content VERSION | Foreach-Object {
   $var = $_.Split('=')
   #New-Variable -Name $var[0] -Value $var[1]
   $key = $var[0]
   $value = $var[1]
   "static constexpr unsigned int $key = $value;" >> code/SCEP/include/SCEP/Version.h
}

# qmake
# -----
cd "$PSScriptRoot\code"
# rm .qmake.stash
& "$env:QT_DIR\bin\qmake.exe" -tp vc -r SCEP.pro
cd "$PSScriptRoot"
