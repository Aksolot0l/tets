# Small offline item-renderer research report for Minecraft 1.1.5.
# Reads the game executable FILE only, never process memory or player data.
# Only fixed windows of executable machine code and the known file hash are
# written. No path, username, worlds, account data, or network transfer.
$ErrorActionPreference = 'Stop'
$expected = '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732'
$game = Get-Process -Name 'Minecraft.Windows' -ErrorAction Stop | Select-Object -First 1
if (-not $game.Path) { throw 'Cannot find game executable path. Run this while Minecraft is open.' }
[byte[]]$data = [IO.File]::ReadAllBytes($game.Path)
$hasher = [Security.Cryptography.SHA256]::Create()
try { $hash = [BitConverter]::ToString($hasher.ComputeHash($data)).Replace('-', '').ToLowerInvariant() }
finally { $hasher.Dispose() }
if ($hash -ne $expected) { throw 'Different game build: no report was created.' }

if ($data.Length -lt 64 -or [BitConverter]::ToUInt16($data, 0) -ne 0x5a4d) { throw 'Invalid executable.' }
$pe = [int][BitConverter]::ToUInt32($data, 0x3c)
if ($pe -lt 0 -or $pe + 24 -gt $data.Length -or [BitConverter]::ToUInt32($data, $pe) -ne 0x4550 -or [BitConverter]::ToUInt16($data, $pe + 4) -ne 0x8664) { throw 'Invalid x64 PE header.' }
$sectionCount = [int][BitConverter]::ToUInt16($data, $pe + 6)
$sectionOffset = $pe + 24 + [int][BitConverter]::ToUInt16($data, $pe + 20)
if ($sectionCount -lt 1 -or $sectionCount -gt 96 -or $sectionOffset + 40 * $sectionCount -gt $data.Length) { throw 'Invalid section table.' }
$sections = @()
for ($i = 0; $i -lt $sectionCount; $i++) {
    $offset = $sectionOffset + 40 * $i
    $sections += [PSCustomObject]@{
        Rva = [uint32][BitConverter]::ToUInt32($data, $offset + 12)
        Size = [uint32][BitConverter]::ToUInt32($data, $offset + 16)
        FileOffset = [uint32][BitConverter]::ToUInt32($data, $offset + 20)
        Flags = [uint32][BitConverter]::ToUInt32($data, $offset + 36)
    }
}
# These are fixed RVAs near documented renderer functions, not an address scan.
$windows = @(
    @{ Name = 'item-in-hand-methods'; Rva = 0x59f000; Length = 0x5000 },
    @{ Name = 'item-stack-helper'; Rva = 0x509000; Length = 0x1000 },
    @{ Name = 'diamond-sword-registry'; Rva = 0xaa8c00; Length = 0x1000 }
)
$report = New-Object System.Text.StringBuilder
[void]$report.AppendLine('Minecraft held-item code diagnostic v3')
[void]$report.AppendLine("EXE SHA256: $hash")
foreach ($window in $windows) {
    $matches = @($sections | Where-Object {
        ($_.Flags -band 0x20000000) -ne 0 -and
        [long]$window.Rva -ge [long]$_.Rva -and
        ([long]$window.Rva - [long]$_.Rva + [long]$window.Length) -le [long]$_.Size
    })
    if ($matches.Count -ne 1) { throw 'Requested executable code is not in one valid section; no report was created.' }
    $start = [long]$matches[0].FileOffset + [long]$window.Rva - [long]$matches[0].Rva
    if ($start -lt 0 -or $start + $window.Length -gt $data.Length) { throw 'Invalid code window; no report was created.' }
    [void]$report.AppendLine(('{0} RVA={1:x8} LENGTH={2:x}' -f $window.Name, $window.Rva, $window.Length))
    for ($n = 0; $n -lt $window.Length; $n += 32) {
        $length = [Math]::Min(32, $window.Length - $n)
        [void]$report.AppendLine([BitConverter]::ToString($data, [int]($start + $n), $length).Replace('-', ''))
    }
    [void]$report.AppendLine('END')
}
$output = Join-Path ([Environment]::GetFolderPath('Desktop')) 'ITEM_RENDER_DIAG_3.txt'
[IO.File]::WriteAllText($output, $report.ToString(), [Text.Encoding]::ASCII)
Write-Host 'Done. Attach ITEM_RENDER_DIAG_3.txt from your Desktop to the chat.'
