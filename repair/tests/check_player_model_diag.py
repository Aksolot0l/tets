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

script2=(Path(__file__).resolve().parents[1]/'DIAG_PLAYER_MODEL_CODE_2.ps1').read_text()
assert '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732' in script2
assert script2.index('if ($hash -ne $expected)') < script2.index('$report = New-Object')
assert '[IO.File]::ReadAllBytes($game.Path)' in script2 and 'ReadProcessMemory' not in script2 and 'OpenProcess' not in script2
assert 'Invoke-WebRequest' not in script2 and 'Invoke-RestMethod' not in script2
assert re.findall(r"Name = '([^']+)'; Rva = (0x[0-9a-f]+); Length = (0x[0-9a-f]+)",script2)==[('base-render','0x5695d0','0x9000'),('player-model-select','0x580000','0x2000'),('matrix-stack','0x730000','0x3000')]
assert '($_.Flags -band 0x20000000)' in script2 and '$matches.Count -ne 1' in script2
assert "'PLAYER_MODEL_DIAG_2.txt'" in script2
print('Follow-up diagnostic: static checks OK; PowerShell/Windows execution still required')

script3=(Path(__file__).resolve().parents[1]/'DIAG_PLAYER_MODEL_CODE_3.ps1').read_text()
assert '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732' in script3
assert script3.index('if ($hash -ne $expected)') < script3.index('$report = New-Object')
assert '[IO.File]::ReadAllBytes($game.Path)' in script3 and 'ReadProcessMemory' not in script3 and 'OpenProcess' not in script3
assert 'Invoke-WebRequest' not in script3 and 'Invoke-RestMethod' not in script3
assert re.findall(r"Name = '([^']+)'; Rva = (0x[0-9a-f]+); Length = (0x[0-9a-f]+)",script3)==[('shared-actor-draw','0x579000','0x8000'),('matrix-operations','0x15d000','0x5000'),('rotation-helper','0x17e000','0x4000'),('tessellator','0x5d0000','0x2000')]
assert '($_.Flags -band 0x20000000)' in script3 and '$matches.Count -ne 1' in script3
assert "'PLAYER_MODEL_DIAG_3.txt'" in script3
print('Third diagnostic: static checks OK; PowerShell/Windows execution still required')
