param(
    [string]$Prefix = (Join-Path $env:LOCALAPPDATA 'Programs\Xe')
)

$ErrorActionPreference = 'Stop'
$ProjectDir = Split-Path -Parent $PSScriptRoot
$WorkDir = Join-Path ([System.IO.Path]::GetTempPath()) ("xe-windows-install-" + [guid]::NewGuid().ToString('N'))
$LibDir = Join-Path $Prefix 'lib\xelang'
$BinDir = Join-Path $Prefix 'bin'

foreach ($tool in @('cmake', 'cl')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Required tool '$tool' was not found. Install CMake and Visual Studio C++ Build Tools, then use a Developer PowerShell."
    }
}

try {
    New-Item -ItemType Directory -Force -Path `
        (Join-Path $LibDir 'runtime'), `
        (Join-Path $LibDir 'std'), `
        (Join-Path $LibDir 'compiler'), `
        (Join-Path $LibDir 'libraries'), `
        $BinDir, $WorkDir | Out-Null

    Copy-Item (Join-Path $ProjectDir 'src\runtime.hpp') (Join-Path $LibDir 'runtime\runtime.hpp') -Force
    Copy-Item (Join-Path $ProjectDir 'std\*.xe') (Join-Path $LibDir 'std') -Force
    Copy-Item (Join-Path $ProjectDir 'compiler\*.xe') (Join-Path $LibDir 'compiler') -Force
    Copy-Item (Join-Path $ProjectDir 'libraries\*.xel') (Join-Path $LibDir 'libraries') -Force

    & cmake -S $ProjectDir -B (Join-Path $WorkDir 'build') `
        -DBUILD_TESTING=OFF `
        "-DXELANG_RUNTIME_DIR=$(Join-Path $LibDir 'runtime')" `
        "-DXELANG_STDLIB_DIR=$(Join-Path $LibDir 'std')"
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }

    & cmake --build (Join-Path $WorkDir 'build') --config Release --target xelang --parallel
    if ($LASTEXITCODE -ne 0) { throw 'Xe bootstrap compiler build failed.' }

    $HostXe = Join-Path $WorkDir 'build\Release\xe.exe'
    if (-not (Test-Path $HostXe)) { $HostXe = Join-Path $WorkDir 'build\xe.exe' }
    if (-not (Test-Path $HostXe)) { throw 'CMake did not produce xe.exe.' }

    $Sources = @('token', 'lexer', 'ast', 'parser', 'checker', 'codegen', 'optimizer', 'runtime', 'main') |
        ForEach-Object { Join-Path $ProjectDir "compiler\$_.xe" }
    $SelfHostedXe = Join-Path $WorkDir 'selfhosted-xe.exe'
    & $HostXe --build-bootstrap @Sources -o $SelfHostedXe
    if ($LASTEXITCODE -ne 0) { throw 'Self-hosted compiler bootstrap failed.' }

    Copy-Item $SelfHostedXe (Join-Path $LibDir 'compiler\selfhosted-xe.exe') -Force
    $Wrapper = Join-Path $BinDir 'xe.cmd'
    @"
@echo off
"%~dp0..\lib\xelang\compiler\selfhosted-xe.exe" %*
"@ | Set-Content -Encoding Ascii $Wrapper

    & (Join-Path $LibDir 'compiler\selfhosted-xe.exe') --version
    if ($LASTEXITCODE -ne 0) { throw 'Installed Xe compiler failed its version check.' }
    Write-Host "Installed Xe in $Prefix"
    Write-Host "Add $BinDir to PATH. For Windows executables, use xe --build-pe and install NASM plus LLVM lld tools."
}
finally {
    if (Test-Path $WorkDir) { Remove-Item $WorkDir -Recurse -Force }
}
