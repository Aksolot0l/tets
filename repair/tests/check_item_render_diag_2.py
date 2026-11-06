from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'DIAG_ITEM_RENDER_2.ps1').read_text()
assert '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732' in s
assert s.index('if ($hash -ne $expected)') < s.index('ItemRenderRttiFileScan]::Scan')
for key in ('ItemInHandRenderer','ItemRenderer','ITEM_RENDER_DIAG_2.txt','DESCRIPTOR_RVA','NO_COL','VTABLE','METHOD','self!=col','imageBase+(ulong)col'):
 assert key in s,key
for key in ('ReadProcessMemory','WriteProcessMemory','Invoke-WebRequest','VirtualProtect'):
 assert key not in s,key
print('Item RTTI diagnostic static checks OK; PowerShell and Minecraft tests required')
