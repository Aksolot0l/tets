from pathlib import Path
import importlib.util, json, math, struct
r=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('convert_katana',r/'tools/convert_katana.py')
c=importlib.util.module_from_spec(spec); spec.loader.exec_module(c)
source=(r/'assets/models/cyberpunk-katana.glb').read_bytes()
result=c.convert(source)
magic,version,v,i,g=struct.unpack_from('<4sIIII',result)
assert (magic,version,v,i,g)==(b'GCM1',1,4572,7866,5)
assert len(result)==20+v*24+i*4+g*24
indices=struct.unpack_from('<%dI'%i,result,20+v*24)
assert max(indices)<v
at=20+v*24+i*4; groups=[struct.unpack_from('<II4f',result,at+n*24) for n in range(g)]
assert [x[:2] for x in groups]==[(0,1074),(1074,888),(1962,552),(2514,4224),(6738,1128)]
assert all(0<=channel<=1 for x in groups for channel in x[2:])
assert groups[1][3]>.79 and groups[1][2]<.08 # green material preserved
verts=[struct.unpack_from('<6f',result,20+n*24) for n in range(v)]
assert all(math.isfinite(x) for vert in verts for x in vert)
assert min(x[0] for x in verts)<-8 and max(x[0] for x in verts)>8
assert c.convert(source)==result # deterministic
for b in (b'',source[:-1],b'BAD!'+source[4:]):
 try: c.convert(b)
 except (ValueError,IndexError,KeyError,struct.error,json.JSONDecodeError): pass
 else: raise AssertionError('corrupted GLB unexpectedly accepted')
try: c.rigid_matrix([1.,0,0,0,0,2.,0,0,0,0,1.,0,0,0,0,1.])
except ValueError: pass
else: raise AssertionError('scaled matrix unexpectedly accepted')
print('Katana conversion: 4572 vertices, 2622 triangles, 5 colors; no game integration')
