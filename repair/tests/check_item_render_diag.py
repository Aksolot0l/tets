from pathlib import Path
import hashlib, json, struct
r=Path(__file__).resolve().parents[1]; s=(r/'DIAG_ITEM_RENDER.ps1').read_text()
assert "2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732" in s
assert s.index('if ($hash -ne $expected)') < s.index('ItemRenderFileScan]::Scan')
for x in ('diamond_sword','iteminhandrenderer','itemrenderer','ITEM_RENDER_DIAG.txt','TOTAL_XREFS','data[i+1]!=0x8d','data[i+1]!=0x8b'):
 assert x in s,x
for x in ('ReadProcessMemory','WriteProcessMemory','Invoke-WebRequest','Get-Content $game.Path','VirtualProtect','Set-Content $game.Path'):
 assert x not in s,x
p=r/'assets/models/cyberpunk-katana.glb'; b=p.read_bytes(); magic,version,length=struct.unpack_from('<4sII',b)
assert (magic,version,length)==(b'glTF',2,len(b))
assert hashlib.sha256(b).hexdigest()=='83996618e51bb3533ae4af3b04fefca2522101c9e31bfcd8b9368ce7973df5d3'
size,kind=struct.unpack_from('<I4s',b,12); assert kind==b'JSON'
d=json.loads(b[20:20+size]); assert len(d['meshes'])==5 and len(d['materials'])==5
assert not d.get('images') and not d.get('skins') and not d.get('animations')
assert d['asset']['extras']['license']=='CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)'
assert 'dentrapank' in (r/'assets/models/README.md').read_text()
print('Item diagnostic static checks and GLB identity OK; PowerShell and Minecraft tests required')
