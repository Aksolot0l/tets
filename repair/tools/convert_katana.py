#!/usr/bin/env python3
"""Offline, bounded GLB-to-simple-mesh converter for the supplied static katana.

No game or process access. Supports only uncompressed triangle geometry,
embedded BIN buffers, POSITION/NORMAL float VEC3, 16/32-bit indices, rigid
node matrices, and untextured base-color materials. Unsupported scene features
are rejected; other material/vertex properties, including PBR, are discarded.
Coordinates and base colors are baked; item placement/rendering is absent.
"""
from __future__ import annotations
import json
import math
import struct
import sys
from pathlib import Path

MAX_FILE = 8 * 1024 * 1024
MAX_VERTICES = 100_000
MAX_INDICES = 300_000
MAX_GROUPS = 128
I = (1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.)

def require(ok, message):
    if not ok:
        raise ValueError(message)

def matrix_mul(a,b):
    return tuple(sum(a[k*4+r]*b[c*4+k] for k in range(4)) for c in range(4) for r in range(4))

def transform(m, p, w):
    return tuple(sum(m[k*4+r]*p[k] for k in range(3))+m[12+r]*w for r in range(3))

def rigid_matrix(values):
    require(isinstance(values, list) and len(values)==16, 'invalid matrix')
    m=tuple(float(x) for x in values)
    require(all(math.isfinite(x) and abs(x)<1e5 for x in m), 'invalid matrix values')
    require(all(abs(m[i])<1e-5 for i in (3,7,11)) and abs(m[15]-1)<1e-5, 'non-affine matrix')
    for x in range(3):
        for y in range(3):
            dot=sum(m[x*4+k]*m[y*4+k] for k in range(3))
            require(abs(dot-(1 if x==y else 0))<1e-4, 'scaled/sheared matrix not supported')
    return m

def parse_glb(data):
    require(20 <= len(data) <= MAX_FILE, 'file size unsupported')
    magic,version,total=struct.unpack_from('<4sII',data)
    require((magic,version,total)==(b'glTF',2,len(data)), 'invalid GLB header')
    chunks=[]; off=12
    while off < len(data):
        require(off+8<=len(data), 'truncated chunk header')
        length,kind=struct.unpack_from('<I4s',data,off); off+=8
        require(length%4==0 and off+length<=len(data), 'invalid chunk size')
        chunks.append((kind,data[off:off+length])); off+=length
    require(len(chunks)==2 and chunks[0][0]==b'JSON' and chunks[1][0]==b'BIN\0', 'expected JSON and BIN chunks')
    doc=json.loads(chunks[0][1].decode('utf-8'))
    require(isinstance(doc,dict) and doc.get('asset',{}).get('version')=='2.0', 'not glTF 2.0')
    require(not doc.get('extensionsRequired') and not doc.get('images') and not doc.get('textures'), 'extensions/textures unsupported')
    require(not doc.get('skins') and not doc.get('animations'), 'skinning/animation unsupported')
    buffers=doc.get('buffers',[])
    require(len(buffers)==1 and 'uri' not in buffers[0] and isinstance(buffers[0].get('byteLength'),int) and 0<=buffers[0]['byteLength']<=len(chunks[1][1]), 'invalid BIN buffer')
    return doc,chunks[1][1][:buffers[0]['byteLength']]

def read_accessor(doc, blob, index, component, shape):
    accessors=doc.get('accessors',[]); views=doc.get('bufferViews',[])
    require(isinstance(index,int) and 0<=index<len(accessors), 'accessor index invalid')
    a=accessors[index]
    require('sparse' not in a and not a.get('normalized',False), 'sparse/normalized accessor unsupported')
    require(a.get('componentType')==component and a.get('type')==shape, 'unexpected accessor format')
    vi=a.get('bufferView'); require(isinstance(vi,int) and 0<=vi<len(views), 'invalid buffer view')
    v=views[vi]; require(v.get('buffer')==0, 'external buffer unsupported')
    count=a.get('count'); limit=MAX_INDICES if shape=='SCALAR' else MAX_VERTICES
    require(isinstance(count,int) and 0<count<=limit, 'accessor count invalid')
    fmt={ (5126,'VEC3'):'<3f', (5123,'SCALAR'):'<H', (5125,'SCALAR'):'<I' }.get((component,shape))
    require(fmt is not None, 'accessor type unsupported')
    item=struct.calcsize(fmt); start=v.get('byteOffset',0); length=v.get('byteLength',-1)
    aoff=a.get('byteOffset',0); stride=v.get('byteStride',item)
    require(all(isinstance(x,int) for x in (start,length,aoff,stride)), 'invalid offsets')
    require(start>=0 and length>=0 and aoff>=0 and stride>=item and stride<=252 and start+length<=len(blob), 'invalid buffer bounds')
    require(aoff+(count-1)*stride+item<=length, 'accessor out of bounds')
    vals=[struct.unpack_from(fmt,blob,start+aoff+i*stride) for i in range(count)]
    if shape=='VEC3': require(all(math.isfinite(x) and abs(x)<1e5 for row in vals for x in row), 'invalid vertex')
    return vals

def convert(data):
    doc,blob=parse_glb(data); nodes=doc.get('nodes',[]); scenes=doc.get('scenes',[])
    scene=doc.get('scene',0); require(isinstance(scene,int) and 0<=scene<len(scenes), 'invalid scene')
    vertices=[]; indices=[]; groups=[]; active=set(); visited=set()
    def visit(n,parent):
        require(isinstance(n,int) and 0<=n<len(nodes) and n not in active and n not in visited, 'invalid/cyclic/shared scene node')
        require(len(active)<64, 'scene nesting too deep'); active.add(n); visited.add(n); node=nodes[n]
        require(not any(k in node for k in ('translation','rotation','scale','skin','weights')), 'node TRS/skin unsupported')
        m=matrix_mul(parent,rigid_matrix(node['matrix']) if 'matrix' in node else I)
        if 'mesh' in node:
            meshes=doc.get('meshes',[]); mi=node['mesh']; require(isinstance(mi,int) and 0<=mi<len(meshes), 'invalid mesh')
            for prim in meshes[mi].get('primitives',[]):
                require(prim.get('mode',4)==4 and not prim.get('targets') and not prim.get('extensions'), 'primitive unsupported')
                a=prim.get('attributes',{}); pos=read_accessor(doc,blob,a.get('POSITION'),5126,'VEC3')
                norm=read_accessor(doc,blob,a.get('NORMAL'),5126,'VEC3'); require(len(pos)==len(norm), 'normal count mismatch')
                ai=prim.get('indices'); acc=doc.get('accessors',[])
                require(isinstance(ai,int) and 0<=ai<len(acc), 'missing indices')
                it=acc[ai].get('componentType'); require(it in (5123,5125), 'index type unsupported')
                ix=[row[0] for row in read_accessor(doc,blob,ai,it,'SCALAR')]
                require(len(ix)%3==0 and all(i<len(pos) for i in ix), 'invalid triangle indices')
                require(len(vertices)+len(pos)<=MAX_VERTICES and len(indices)+len(ix)<=MAX_INDICES and len(groups)<MAX_GROUPS, 'mesh too large')
                material=prim.get('material'); mats=doc.get('materials',[])
                require(isinstance(material,int) and 0<=material<len(mats), 'missing material')
                pbr=mats[material].get('pbrMetallicRoughness',{})
                require('baseColorTexture' not in pbr and not mats[material].get('extensions'), 'textured/extension material unsupported')
                color=pbr.get('baseColorFactor',[1.,1.,1.,1.]); require(isinstance(color,list) and len(color)==4 and all(isinstance(x,(int,float)) and math.isfinite(x) and 0<=x<=1 for x in color),'invalid color')
                first_v=len(vertices); first_i=len(indices)
                baked=[(*transform(m,p,1),*transform(m,norm[j],0)) for j,p in enumerate(pos)]
                require(all(math.isfinite(x) and abs(x)<=1e6 for row in baked for x in row), 'transformed vertex out of range')
                vertices.extend(baked)
                indices.extend(first_v+i for i in ix)
                groups.append((first_i,len(ix),tuple(float(x) for x in color)))
        children=node.get('children',[]); require(isinstance(children,list), 'invalid children')
        for c in children: visit(c,m)
        active.remove(n)
    roots=scenes[scene].get('nodes',[]); require(isinstance(roots,list) and roots, 'empty scene')
    for root in roots: visit(root,I)
    require(vertices and indices and groups, 'empty mesh')
    out=bytearray(struct.pack('<4sIIII',b'GCM1',1,len(vertices),len(indices),len(groups)))
    for v in vertices: out.extend(struct.pack('<6f',*v))
    for index in indices: out.extend(struct.pack('<I',index))
    for first,count,color in groups: out.extend(struct.pack('<II4f',first,count,*color))
    return bytes(out)

def main():
    if len(sys.argv)!=3:
        raise SystemExit('usage: convert_katana.py INPUT.glb OUTPUT.gcm')
    src,dst=map(Path,sys.argv[1:]); require(src.resolve()!=dst.resolve(),'input and output must differ')
    require(src.stat().st_size<=MAX_FILE,'file too large')
    data=convert(src.read_bytes()); dst.write_bytes(data)
    _,_,v,i,g=struct.unpack_from('<4sIIII',data)
    print(f'Converted {v} vertices, {i//3} triangles, {g} material groups to {dst}')

if __name__=='__main__':
    main()
