# Small offline mouse input research report for Minecraft 1.1.5.
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
# Scan only on-disk executable sections, not process memory. This pattern is a
# broad research lead for a small pointer-chain routine with a mode value of 2;
# it is NOT proof of a mouse function and can produce false positives. Output
# is bounded to 256 executable-code excerpts, no player/world/input contents.
$report = New-Object System.Text.StringBuilder
[void]$report.AppendLine('Minecraft mouse code search diagnostic v1')
[void]$report.AppendLine("EXE SHA256: $hash")
$regex = [Text.RegularExpressions.Regex]::new(
    '(?:\x48\x8B[\x40-\x7F][\s\S]{1,4}){2}[\s\S]{0,64}\xC7[\s\S]{0,5}\x02\x00\x00\x00[\s\S]{0,24}\xC3')
$encoder = [Text.Encoding]::GetEncoding(28591)
$count = 0
foreach ($section in $sections) {
    if (($section.Flags -band 0x20000000) -eq 0 -or $section.Size -eq 0) { continue }
    $start = [long]$section.FileOffset
    $end = $start + [long]$section.Size
    if ($start -lt 0 -or $end -gt $data.Length) { throw 'Invalid executable section; no report was created.' }
    $text = $encoder.GetString($data, [int]$start, [int]$section.Size)
    foreach ($match in $regex.Matches($text)) {
        if ($count -ge 256) { break }
        $index = [int]$match.Index
        $begin = [Math]::Max(0, $index - 512)
        $length = [Math]::Min(1024, [int]$section.Size - $begin)
        $rva = [long]$section.Rva + $begin
        [void]$report.AppendLine(('candidate-{0:D3} RVA={1:x8} LENGTH={2:x} MATCH={3:x8}' -f $count, $rva, $length, ([long]$section.Rva + $index)))
        for ($n = 0; $n -lt $length; $n += 32) {
            $part = [Math]::Min(32, $length - $n)
            [void]$report.AppendLine([BitConverter]::ToString($data, [int]($start + $begin + $n), $part).Replace('-', ''))
        }
        [void]$report.AppendLine('END')
        $count++
    }
    if ($count -ge 256) { break }
}
[void]$report.AppendLine("CANDIDATES: $count (limit 256)")
$output = Join-Path ([Environment]::GetFolderPath('Desktop')) 'MOUSE_CODE_DIAG.txt'
[IO.File]::WriteAllText($output, $report.ToString(), [Text.Encoding]::ASCII)
Write-Host 'Done. Attach MOUSE_CODE_DIAG.txt from your Desktop to the chat.'
