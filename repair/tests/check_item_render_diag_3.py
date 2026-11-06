from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'DIAG_ITEM_RENDER_3.ps1').read_text()
assert '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732' in s
assert s.index('if ($hash -ne $expected)') < s.index("'item-in-hand-methods'")
for x in ('Rva = 0x59f000; Length = 0x5000','Rva = 0x509000; Length = 0x1000','Rva = 0xaa8c00; Length = 0x1000','ITEM_RENDER_DIAG_3.txt'):
 assert x in s,x
for x in ('ReadProcessMemory','WriteProcessMemory','Invoke-WebRequest','VirtualProtect'):
 assert x not in s,x
print('Held item code diagnostic static checks OK; Windows test required')
