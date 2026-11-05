# Experimental build: installs the compatibility adapter and the original core.
# Windows PowerShell 5.1+; no automatic execution or ACL changes.
param(
    [string]$Destination = (Join-Path $env:LOCALAPPDATA 'GAYClient-Repair')
)
$ErrorActionPreference = 'Stop'
# This branch must be published to GitHub before the installer can download it.
$archiveUrl = 'https://codeload.github.com/Aksolot0l/tets/zip/refs/heads/arena/5d11f1ec-tets'
$files = @(
    @{ Source = 'client.exe'; Destination = 'client.exe'; Size = 4702208; SHA256 = 'e0f90e9aa78e417f0337acbb8f84162e27b9fb9396cf2c22af5950a16db11b19' },
    @{ Source = 'GAYClientInternal.dll'; Destination = 'GAYClientCore.dll'; Size = 4228404; SHA256 = 'f74a42c4aedd1c9d1f0c92200e9c7e977d800ab686d108114b2711c0c81835c5' },
    @{ Source = 'repair/GAYClientInternal.dll'; Destination = 'GAYClientInternal.dll'; Size = 151040; SHA256 = 'b0a7d82f2ea68e02fd66067242fa34ecdf18a82fa15b95e8a7f6f8d4fb190754' },
    @{ Source = 'repair/ENABLE_ACCESS.cmd'; Destination = 'ENABLE_ACCESS.cmd'; Size = 1440; SHA256 = 'c106b456540d9c373cd9e968081245091515e46cf9c8a92dcd22843f011439a0' },
    @{ Source = 'repair/RESTORE_ACCESS.cmd'; Destination = 'RESTORE_ACCESS.cmd'; Size = 769; SHA256 = '5d3ef1a0a75f30a7e35a8c67213d8357659718c1c79a2489646f2f846b410eb6' }
)
Write-Warning 'This is an EXPERIMENTAL build. It has not been tested in Windows or Minecraft.'
Write-Warning 'The original core DLL does not match the old SHA256SUMS.txt. Hashes below only identify repository files, not their trustworthiness.'
Write-Host "Destination: $Destination"
if ((Read-Host 'Download and install this experimental build? Type YES to continue') -cne 'YES') {
    Write-Host 'Cancelled; no files were installed.'
    exit 0
}
$zipPath = Join-Path ([IO.Path]::GetTempPath()) ("gayclient-" + [Guid]::NewGuid().ToString('N') + '.zip')
$stage = $null
$zip = $null
try {
    $destinationPath = [IO.Path]::GetFullPath($Destination).TrimEnd('\', '/')
    if ([string]::IsNullOrWhiteSpace($destinationPath) -or (Test-Path -LiteralPath $destinationPath)) {
        throw "Destination already exists or is invalid: $destinationPath. Choose a new folder."
    }
    $parent = [IO.Path]::GetDirectoryName($destinationPath)
    if ([string]::IsNullOrWhiteSpace($parent) -or -not (Test-Path -LiteralPath $parent -PathType Container)) {
        throw "Parent directory must exist: $parent"
    }
    $stage = Join-Path $parent ('.gayclient-install-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $stage | Out-Null
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    Write-Host 'Downloading repair branch from GitHub...'
    Invoke-WebRequest -Uri $archiveUrl -OutFile $zipPath -UseBasicParsing -TimeoutSec 120
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($zipPath)
    foreach ($file in $files) {
        # Permit exactly one top-level archive directory and never extract paths from the archive.
        $matching = @($zip.Entries | Where-Object {
            $name = $_.FullName
            $separator = $name.IndexOf('/')
            $separator -gt 0 -and $name.Substring($separator + 1) -ceq $file.Source
        })
        if ($matching.Count -ne 1 -or $matching[0].Length -ne $file.Size) {
            throw "Missing, duplicate, or unexpected file in archive: $($file.Source)"
        }
        $outPath = Join-Path $stage $file.Destination
        $inputStream = $matching[0].Open()
        try {
            $outputStream = [IO.File]::Open($outPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
            try { $inputStream.CopyTo($outputStream) } finally { $outputStream.Dispose() }
        } finally { $inputStream.Dispose() }
        $hash = (Get-FileHash -LiteralPath $outPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($hash -cne $file.SHA256) { throw "SHA-256 mismatch for $($file.Source); no files installed." }
    }
    $zip.Dispose(); $zip = $null
    [IO.Directory]::Move($stage, $destinationPath)
    $stage = $null
    Write-Host "Installed to: $destinationPath"
    Write-Host 'The program was not started and permissions were not changed.'
    Write-Host 'If AppContainer cannot access the DLLs, see ENABLE_ACCESS.cmd and RESTORE_ACCESS.cmd in that folder.'
} catch {
    Write-Error "Installation failed: $($_.Exception.Message)" -ErrorAction Continue
    exit 1
} finally {
    if ($null -ne $zip) { $zip.Dispose() }
    if ($null -ne $stage -and (Test-Path -LiteralPath $stage)) { Remove-Item -LiteralPath $stage -Recurse -Force }
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
}
