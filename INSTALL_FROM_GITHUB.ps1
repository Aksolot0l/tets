# One installer/updater for the experimental GAYClient build (Windows PowerShell 5.1+).
# The downloaded manifest checks transfer consistency, not publisher authenticity.
param(
    [string]$Destination = 'D:\GAYClient-Repair',
    [string]$Branch = 'arena/0c7aa1e6-tets'
)
$ErrorActionPreference = 'Stop'
$archiveUrl = "https://codeload.github.com/Aksolot0l/tets/zip/refs/heads/$Branch"
$expectedFiles = @{
    'repair/client.exe' = 'client.exe'
    'repair/GAYClientLauncher.dll' = 'GAYClientLauncher.dll'
    'repair/WebView2Loader.dll' = 'WebView2Loader.dll'
    'repair/THIRD_PARTY_NOTICES.txt' = 'THIRD_PARTY_NOTICES.txt'
    'GAYClientInternal.dll' = 'GAYClientCore.dll'
    'repair/GAYClientInternal.dll' = 'GAYClientInternal.dll'
    'repair/ENABLE_ACCESS.cmd' = 'ENABLE_ACCESS.cmd'
    'repair/RESTORE_ACCESS.cmd' = 'RESTORE_ACCESS.cmd'
    'index.html' = 'web-menu.html'
}

Write-Host 'GAYClient installer / updater'
Write-Warning 'This build is experimental and the original core does not match the old SHA256SUMS.txt.'
Write-Warning 'The GitHub manifest and files come from the same branch; their hashes do not prove publisher authenticity.'
Write-Warning 'The installer grants ALL APPLICATION PACKAGES read/execute access to both DLLs, saving the original ACLs first.'
Write-Host "Install or update folder: $Destination"
if ((Read-Host 'Close Minecraft, client.exe and START_CLIENT.exe first. Type YES to continue') -cne 'YES') {
    Write-Host 'Cancelled; no changes made.'
    exit 0
}

$zipPath = Join-Path ([IO.Path]::GetTempPath()) ('gayclient-' + [Guid]::NewGuid().ToString('N') + '.zip')
$stage = $null
$zip = $null
$keepStage = $false
$changes = New-Object 'System.Collections.Generic.List[object]'

function Find-ArchiveEntry($archive, [string]$relativeName) {
    $found = @($archive.Entries | Where-Object {
        $name = $_.FullName
        $i = $name.IndexOf('/')
        $i -gt 0 -and $name.Substring($i + 1) -ceq $relativeName
    })
    if ($found.Count -ne 1) { throw "Missing or duplicate archive file: $relativeName" }
    return $found[0]
}

function Set-DllAccess([string]$directory) {
    $dlls = @(
        (Join-Path $directory 'GAYClientInternal.dll'),
        (Join-Path $directory 'GAYClientCore.dll')
    )
    $backups = @(
        (Join-Path $directory 'GAYClientInternal.acl.local.txt'),
        (Join-Path $directory 'GAYClientCore.acl.local.txt')
    )
    $hasFirst = Test-Path -LiteralPath $backups[0] -PathType Leaf
    $hasSecond = Test-Path -LiteralPath $backups[1] -PathType Leaf
    if ($hasFirst -ne $hasSecond) { throw 'Only one ACL backup exists. No permissions were changed; inspect the backups before continuing.' }
    if (-not $hasFirst) {
        try {
            for ($i = 0; $i -lt 2; $i++) {
                & icacls.exe $dlls[$i] /save $backups[$i]
                if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $backups[$i] -PathType Leaf)) {
                    throw "Could not back up permissions for $($dlls[$i])."
                }
            }
        } catch {
            foreach ($backup in $backups) { if (Test-Path -LiteralPath $backup -PathType Leaf) { Remove-Item -LiteralPath $backup -Force } }
            throw
        }
    } else {
        Write-Host 'Keeping the existing ACL backups.'
    }
    try {
        foreach ($dll in $dlls) {
            & icacls.exe $dll /grant '*S-1-15-2-1:(RX)'
            if ($LASTEXITCODE -ne 0) { throw "Could not grant permissions for $dll." }
        }
    } catch {
        Write-Warning 'Permission change failed. Attempting to restore both original ACLs; backups will be kept.'
        foreach ($backup in $backups) {
            & icacls.exe $directory /restore $backup
            if ($LASTEXITCODE -ne 0) { Write-Warning "Could not restore: $backup" }
        }
        throw
    }
}

try {
    if (@(Get-Process -Name 'Minecraft.Windows','client','START_CLIENT' -ErrorAction SilentlyContinue).Count -gt 0) {
        throw 'Minecraft, client.exe or START_CLIENT.exe is still running. Close them and run the installer again.'
    }
    $destinationPath = [IO.Path]::GetFullPath($Destination).TrimEnd('\', '/')
    $parent = [IO.Path]::GetDirectoryName($destinationPath)
    if ([string]::IsNullOrWhiteSpace($destinationPath) -or [string]::IsNullOrWhiteSpace($parent) -or
        -not (Test-Path -LiteralPath $parent -PathType Container)) { throw 'The destination or its parent directory is invalid.' }
    $existing = Test-Path -LiteralPath $destinationPath
    if ($existing -and -not (Test-Path -LiteralPath $destinationPath -PathType Container)) { throw 'Destination exists but is not a folder.' }
    $stage = Join-Path $parent ('.gayclient-install-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $stage | Out-Null
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    Write-Host 'Downloading from GitHub...'
    Invoke-WebRequest -Uri $archiveUrl -OutFile $zipPath -UseBasicParsing -TimeoutSec 120
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($zipPath)
    $manifestEntry = Find-ArchiveEntry $zip 'repair/install-manifest.json'
    if ($manifestEntry.Length -gt 65536) { throw 'Manifest is too large.' }
    $reader = [IO.StreamReader]::new($manifestEntry.Open(), [Text.Encoding]::UTF8)
    try { $manifest = $reader.ReadToEnd() | ConvertFrom-Json } finally { $reader.Dispose() }
    $files = @($manifest.files)
    if ($manifest.schema -ne 1 -or $files.Count -ne $expectedFiles.Count) { throw 'Unsupported manifest.' }
    $seen = @{}
    foreach ($file in $files) {
        $src = [string]$file.source; $dst = [string]$file.destination
        if (-not $expectedFiles.ContainsKey($src) -or $expectedFiles[$src] -cne $dst -or $seen.ContainsKey($dst)) { throw 'Unexpected or duplicate manifest path.' }
        $seen[$dst] = $true
        if ($file.size -lt 1 -or $file.size -gt 67108864 -or $file.sha256 -cnotmatch '^[0-9a-f]{64}$') { throw "Invalid manifest entry: $src" }
        foreach ($hash in @($file.previousSha256)) { if ($hash -cnotmatch '^[0-9a-f]{64}$') { throw "Invalid previous hash: $src" } }
        $entry = Find-ArchiveEntry $zip $src
        if ($entry.Length -ne $file.size) { throw "Unexpected file size: $src" }
        $outPath = Join-Path $stage $dst
        $inputStream = $entry.Open()
        try {
            $outputStream = [IO.File]::Open($outPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
            try { $inputStream.CopyTo($outputStream) } finally { $outputStream.Dispose() }
        } finally { $inputStream.Dispose() }
        if ((Get-FileHash -LiteralPath $outPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne $file.sha256) { throw "SHA-256 mismatch: $src" }
    }
    $zip.Dispose(); $zip = $null
    # The loader was obtained from a mirrored WebView2 SDK package. Require a
    # valid Windows Authenticode signature by Microsoft, in addition to the
    # manifest hash, before any existing installation is changed.
    $webSignature = Get-AuthenticodeSignature -LiteralPath (Join-Path $stage 'WebView2Loader.dll')
    if ($webSignature.Status -ne 'Valid' -or $null -eq $webSignature.SignerCertificate -or
        $webSignature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation') {
        throw 'WebView2Loader.dll does not have a valid Microsoft signature; no files were changed.'
    }
    if ($existing) {
        # Validate the whole existing installation before touching any file.
        foreach ($file in $files) {
            $target = Join-Path $destinationPath $file.destination
            if (Test-Path -LiteralPath $target) {
                $item = Get-Item -LiteralPath $target -Force
                if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Not a regular file: $target" }
                $hash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
                if ($hash -cne $file.sha256 -and @($file.previousSha256) -notcontains $hash) {
                    throw "Existing file has an unknown version; it was not overwritten: $target"
                }
            }
        }
        try {
            foreach ($file in $files) {
                $target = Join-Path $destinationPath $file.destination
                $source = Join-Path $stage $file.destination
                if (Test-Path -LiteralPath $target -PathType Leaf) {
                    $hash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
                    if ($hash -ceq $file.sha256) { continue }
                    $backup = Join-Path $stage ('backup-' + $file.destination)
                    # ReplaceFile preserves the target's ACL; retain a byte backup for rollback.
                    [IO.File]::Replace($source, $target, $backup)
                    $changes.Add(@{ Target = $target; Backup = $backup; New = $false })
                } else {
                    [IO.File]::Move($source, $target)
                    $changes.Add(@{ Target = $target; Backup = $null; New = $true })
                }
            }
            # Retire only our exact, unmodified old companion EXE. Unknown files
            # with this name are left untouched; the move is rollback-able.
            $oldLauncher = Join-Path $destinationPath 'START_CLIENT.exe'
            if (Test-Path -LiteralPath $oldLauncher) {
                $oldItem = Get-Item -LiteralPath $oldLauncher -Force
                if (-not $oldItem.PSIsContainer -and -not ($oldItem.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                    $oldHash = (Get-FileHash -LiteralPath $oldLauncher -Algorithm SHA256).Hash.ToLowerInvariant()
                    $retiredHashes = @(
                        'c6b302f4829e668eca81d24bed1fb9fb2498a9fae20e4e78a5a0c035c08e67ba',
                        '15ba3b333c4f1c8247b9fb6100bb5fa99aedfd1463cdcf83a02adc4dbc19856b'
                    )
                    if ($retiredHashes -ccontains $oldHash) {
                        $retiredBackup = Join-Path $stage 'retired-START_CLIENT.exe'
                        [IO.File]::Move($oldLauncher, $retiredBackup)
                        $changes.Add(@{ Target = $oldLauncher; Backup = $retiredBackup; Removed = $true })
                        Write-Host 'Retired the known old START_CLIENT.exe.'
                    } else { Write-Warning 'Unknown START_CLIENT.exe left untouched.' }
                } else { Write-Warning 'Non-regular START_CLIENT.exe left untouched.' }
            }
        } catch {
            Write-Warning 'File update failed. Attempting to roll back changed files.'
            for ($i = $changes.Count - 1; $i -ge 0; $i--) {
                $change = $changes[$i]
                try {
                    if ($change.Removed) { [IO.File]::Move($change.Backup, $change.Target) }
                    elseif ($change.New) { [IO.File]::Delete($change.Target) }
                    else { [IO.File]::Replace($change.Backup, $change.Target, $null) }
                } catch {
                    $keepStage = $true
                    Write-Warning "Rollback failed for $($change.Target). Backups kept in $stage"
                }
            }
            throw
        }
    } else {
        [IO.Directory]::Move($stage, $destinationPath)
        $stage = $null
    }
    Write-Host "Files installed / updated: $destinationPath"
    Set-DllAccess $destinationPath
    Write-Host 'Done. Run client.exe from this folder and start the game in the client.'
    Write-Host 'Other files and local ACL backups were preserved.'
} catch {
    Write-Error "Installer stopped: $($_.Exception.Message)" -ErrorAction Continue
    exit 1
} finally {
    if ($null -ne $zip) { $zip.Dispose() }
    if (-not $keepStage -and $null -ne $stage -and (Test-Path -LiteralPath $stage)) { Remove-Item -LiteralPath $stage -Recurse -Force }
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
}
