from pathlib import Path
import re

script=(Path(__file__).resolve().parents[1]/'DIAG_PLAYER_MODEL_CODE.ps1').read_text()
assert '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732' in script
assert script.index('if ($hash -ne $expected)') < script.index('$report = New-Object')
assert "Get-Process -Name 'Minecraft.Windows'" in script
assert '[IO.File]::ReadAllBytes($game.Path)' in script
assert 'ReadProcessMemory' not in script and 'OpenProcess' not in script
assert 'Invoke-WebRequest' not in script and 'Invoke-RestMethod' not in script
windows=re.findall(r"Name = '([^']+)'; Rva = (0x[0-9a-f]+); Length = (0x[0-9a-f]+)",script)
assert windows==[('player-render','0x581910','0x8000'),('entity-render','0x55d640','0x3000'),('render-entities','0x5ae250','0x2000')]
assert '($_.Flags -band 0x20000000)' in script and '$matches.Count -ne 1' in script
assert "'PLAYER_MODEL_DIAG.txt'" in script
print('Player model diagnostic: static checks OK; PowerShell/Windows execution still required')
