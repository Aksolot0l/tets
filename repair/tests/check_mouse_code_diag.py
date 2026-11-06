"""Static/fixture checks only; PowerShell + the actual game remain untested."""
from pathlib import Path
import re
s=(Path(__file__).resolve().parents[1]/'DIAG_MOUSE_CODE.ps1').read_text()
assert '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732' in s
assert s.index('if ($hash -ne $expected)') < s.index('$report = New-Object')
assert '[IO.File]::ReadAllBytes($game.Path)' in s
assert 'ReadProcessMemory' not in s and 'OpenProcess' not in s
assert 'Invoke-WebRequest' not in s and 'Invoke-RestMethod' not in s
assert '0x20000000' in s and '$count -ge 256' in s and '[Text.RegularExpressions.Regex]::new' in s
assert "'MOUSE_CODE_DIAG.txt'" in s
p=re.search(r"    '(\(\?:\\x48\\x8B.*?)'\)\n",s).group(1)
regex=re.compile(p,re.DOTALL)
positive=b'\x48\x8b\x41\x08\x48\x8b\x40\x08\xc7\x40\x68\x02\x00\x00\x00\xc3'
negative=positive.replace(b'\x02\x00\x00\x00',b'\x03\x00\x00\x00')
assert regex.search(positive.decode('latin1')) and not regex.search(negative.decode('latin1'))
print('Mouse diagnostic static and synthetic pattern checks: OK; Windows still required')
