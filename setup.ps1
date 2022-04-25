#Set-PSDebug -Off

# Environnement
# -------------

$env = "$PSScriptRoot\env.ps1"
if (-not(Test-Path -Path $env -PathType Leaf)) {
    Write-Output "env.ps1 file does not exist ! Please copy env.ps1.proto to env.ps1 and set up your environment."
    Exit
}

& $env

# vcvarsall.bat
# -------------
$vcvarsall = "$env:VCVARSALL_DIR\vcvarsall.bat"
if (-not(Test-Path -Path $vcvarsall -PathType Leaf)) {
    Write-Output "Unable to find vcvarsall.bat. Please verify the env.bat content."
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
$PLATFORM_TOOLSET_LIGHT = $env:VCToolsVersion.Substring(0, 2) + "0"
# PLATFORM_TOOLSET_FULL -> 143
$PLATFORM_TOOLSET_FULL = $env:VCToolsVersion.Substring(0, 2) + $env:VCToolsVersion.Substring(3, 1)
$inputDir = "$env:VCToolsRedistDir\$env:VSCMD_ARG_TGT_ARCH\Microsoft.VC$PLATFORM_TOOLSET_FULL.CRT"
$files = "msvcp${PLATFORM_TOOLSET_LIGHT}.dll", "msvcp${PLATFORM_TOOLSET_LIGHT}_1.dll", "msvcp${PLATFORM_TOOLSET_LIGHT}_2.dll", "vcruntime${PLATFORM_TOOLSET_LIGHT}.dll", "vcruntime${PLATFORM_TOOLSET_LIGHT}_1.dll"
foreach ( $file in $files ) {
    Copy-Item -Force "$inputDir\$file" $DEPLOY_DIR
}

# version update
# --------------

$vdproj_file = "$PSScriptRoot\setup\SCEP\SCEP.vdproj"
$setup_version = (Select-String -Path $vdproj_file -Pattern '"ProductVersion" = "8:([0-9].[0-9].[0-9])"' -AllMatches | ForEach-Object { $_.Matches.Groups[1].value })
$setup_product_code = (Select-String -Path $vdproj_file -Pattern '"ProductCode" = "8:[{]([0-9A-Fa-f]{8}[-][0-9A-Fa-f]{4}[-][0-9A-Fa-f]{4}[-][0-9A-Fa-f]{4}[-][0-9A-Fa-f]{12})[}]"' -AllMatches | ForEach-Object { $_.Matches.Groups[1].value })
$current_version = ""
Get-Content "$PSScriptRoot/VERSION" | Foreach-Object {
    if (-not([string]::IsNullOrEmpty($current_version))) {
        $current_version = $current_version + "."
    }
    $var = $_.Split('=')
    #$key = $var[0]
    $value = $var[1]
    $current_version = $current_version + $value
}
$final_warning = $false
if ($setup_version -ne $current_version) {
    # Update Product Version and Product Code of SCEP.vdproj
    $content = (Get-Content -path $vdproj_file -Raw)
    # New Product Version
    $str_before = "`"ProductVersion`" = `"8:$setup_version`""
    $str_after = "`"ProductVersion`" = `"8:$current_version`""
    $content = $content -replace $str_before, $str_after
    # New Product Code
    $str_before = "`"ProductCode`" = `"8:{$setup_product_code}`""
    $guid = New-Guid
    $current_product_code = $guid.ToString().ToUpper()
    $str_after = "`"ProductCode`" = `"8:{$current_product_code}`""
    $content = $content -replace $str_before, $str_after
    # Overwrite content
    $content | Set-Content -Encoding UTF8 -Path $vdproj_file
    # Final warning
    $final_warning = $true
}

# build setup
# -----------
# 1/ msbuild does not support building vdproj projects, falling back to devenv.com
& devenv.com "$PSScriptRoot\setup\SCEP\SCEP.sln" /Rebuild "Release|Default"
# 2/ the generated setup does not require admin rights
# https://stackoverflow.com/questions/4080131/how-to-make-a-setup-work-for-limited-non-admin-users
& "$env:WindowsSdkVerBinPath\x86\MsiInfo.exe" "$PSScriptRoot\setup\SCEP\Release\SCEP.msi" -w 10
# 3/ rename and copy setup
Copy-Item -Force "$PSScriptRoot\setup\SCEP\Release\SCEP.msi" "$PSScriptRoot\SCEP_${current_version}_${env:BUILD}.msi"

if ($final_warning) {
    Write-Output ""
    Write-Output ""
    Write-Output "WARNING: New SCEP version detected !!!"
    Write-Output "    ($setup_version --> $current_version)"
    Write-Output "    The version and the product code of the Windows Installer have been updated (see SCEP.vdproj)."
    Write-Output "    Please commit these changes with this release."
    Write-Output ""
}