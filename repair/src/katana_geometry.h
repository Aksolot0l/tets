#pragma once
#include "katana_mesh.h"
#include <cmath>
#include <cstdint>

// CPU-only triangle expansion. This does not access Minecraft or issue draws.
// GCM1 is in the model's source coordinates. The supplied pivot/axes/scale
// map those coordinates into the caller's space; their game values are unknown.
namespace katana_geometry {
struct Vec3 { float x=0,y=0,z=0; };
struct Pose {
 Vec3 origin{}, pivot{};
 Vec3 x{1,0,0}, y{0,1,0}, z{0,0,1};
 float scale=1;
};
struct ColoredVertex { Vec3 position; float rgba[4]{}; };
inline bool finite(Vec3 v) {
 return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
        std::fabs(v.x)<=1000000.f && std::fabs(v.y)<=1000000.f && std::fabs(v.z)<=1000000.f;
}
inline Vec3 apply(const Pose& p,const float* v) {
 const float x=(v[0]-p.pivot.x)*p.scale, y=(v[1]-p.pivot.y)*p.scale, z=(v[2]-p.pivot.z)*p.scale;
 return {p.origin.x+p.x.x*x+p.y.x*y+p.z.x*z,
         p.origin.y+p.x.y*x+p.y.y*y+p.z.y*z,
         p.origin.z+p.x.z*x+p.y.z*y+p.z.z*z};
}
// Callback receives triangle vertices and raw glTF linear baseColor factors.
// With immutable backing bytes, all inputs are checked before the first callback.
template<class Emit> bool triangles(const katana_mesh::View& mesh,const Pose& pose,bool doubleSided,Emit emit) {
 if (!mesh.vertices() || !mesh.indices() || !mesh.groups() || !finite(pose.origin) || !finite(pose.pivot) ||
     !finite(pose.x) || !finite(pose.y) || !finite(pose.z) || !std::isfinite(pose.scale) || pose.scale<=0 || pose.scale>1000) return false;
 const double det=double(pose.x.x)*(double(pose.y.y)*pose.z.z-double(pose.y.z)*pose.z.y)
                 -double(pose.y.x)*(double(pose.x.y)*pose.z.z-double(pose.x.z)*pose.z.y)
                 +double(pose.z.x)*(double(pose.x.y)*pose.y.z-double(pose.x.z)*pose.y.y);
 if(!std::isfinite(det) || std::fabs(det)<1e-12) return false;
 katana_mesh::Vertex vertex;
 for (std::uint32_t i=0;i<mesh.vertices();++i) {
  if (!mesh.vertex(i,vertex) || !finite(apply(pose,vertex.position))) return false;
 }
 for (std::uint32_t n=0;n<mesh.groups();++n) {
  katana_mesh::Group group; if(!mesh.group(n,group)) return false;
  for(std::uint32_t j=0;j<group.count;j+=3) {
   ColoredVertex tri[3]{};
   for(std::uint32_t k=0;k<3;++k) {
    std::uint32_t index; if(!mesh.index(group.first+j+k,index) || !mesh.vertex(index,vertex)) return false;
    tri[k].position=apply(pose,vertex.position);
    for(int c=0;c<4;++c) tri[k].rgba[c]=group.color[c];
   }
   emit(tri[0]); emit(tri[1]); emit(tri[2]);
   if(doubleSided) { emit(tri[2]); emit(tri[1]); emit(tri[0]); }
  }
 }
 return true;
}
// Approximate grip center inferred from the supplied model's bounds. This is
// NOT a proven Minecraft hand pose and must be calibrated in a safe test.
inline Pose katanaLocalPose() {
 Pose p; p.pivot={6.2f,2.6f,0.f}; p.x={0,-1,0}; p.y={1,0,0}; p.z={0,0,1}; p.scale=1.f/17.221883f; return p;
}
}
