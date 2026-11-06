"""Static checks; execution against Windows/Minecraft still required."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'DIAG_MOUSE_CODE_2.ps1').read_text()
assert '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732' in s
assert s.index('if ($hash -ne $expected)') < s.index('$report = New-Object')
assert '[IO.File]::ReadAllBytes($game.Path)' in s
assert 'ReadProcessMemory' not in s and 'OpenProcess' not in s
assert 'Invoke-WebRequest' not in s and 'Invoke-RestMethod' not in s
assert '0x20000000' in s and "'MOUSE_CODE_DIAG_2.txt'" in s
assert "Rva = 0x6e8000; Length = 0x20000" in s
assert "Rva = 0x7c0000; Length = 0x20000" in s
print('Mouse follow-up diagnostic static checks: OK; Windows still required')
