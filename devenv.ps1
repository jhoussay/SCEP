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


# devenv
# ------

$devenv = "$env:DEVENV_DIR\devenv.exe"
if (-not(Test-Path -Path $devenv -PathType Leaf))
{
    echo Unable to find devenv.exe. Please verify the env.ps1 content.
    Exit
}

Start-Process "$devenv" "$PSScriptRoot/code/SCEP.sln"
