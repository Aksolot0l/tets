#include "../src/katana_geometry.h"
#include "../src/katana_pose.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>
using namespace katana_geometry;
static void put32(std::vector<std::uint8_t>& b,std::size_t i,std::uint32_t n) { for(int k=0;k<4;++k)b[i+k]=static_cast<std::uint8_t>(n>>(k*8)); }
static void putf(std::vector<std::uint8_t>& b,std::size_t i,float x) { std::uint32_t n; std::memcpy(&n,&x,4); put32(b,i,n); }
int main(int argc,char**argv) {
 std::vector<std::uint8_t> b(20+3*24+3*4+24); std::memcpy(b.data(),"GCM1",4);
 put32(b,4,1); put32(b,8,3); put32(b,12,3); put32(b,16,1); putf(b,20+24,1); putf(b,20+48+4,1);
 auto ix=20+72; put32(b,ix+4,1); put32(b,ix+8,2); auto gr=ix+12; put32(b,gr+4,3); putf(b,gr+8,0.25f); putf(b,gr+20,1);
 katana_mesh::View m; assert(m.load(b.data(),b.size()));
 std::vector<ColoredVertex> out; auto emit=[&](ColoredVertex v){ out.push_back(v); };
 Pose p; p.origin={2,3,4}; p.scale=2;
 assert(triangles(m,p,true,emit)); assert(out.size()==6); assert(out[1].position.x==4 && out[2].position.y==5);
 assert(out[0].rgba[0]==.25f && out[0].rgba[3]==1); assert(out[3].position.y==out[2].position.y);
 out.clear(); p.scale=std::numeric_limits<float>::infinity(); assert(!triangles(m,p,true,emit) && out.empty());
 p.scale=1; p.origin.x=std::numeric_limits<float>::quiet_NaN(); assert(!triangles(m,p,true,emit) && out.empty());
 p.origin.x=0; p.x={0,0,0}; assert(!triangles(m,p,true,emit) && out.empty());
 if(argc>1) {
  std::ifstream f(argv[1],std::ios::binary); assert(f);
  std::vector<std::uint8_t> model((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
  assert(m.load(model.data(),model.size()));
  // The real model through the new held-item pose: grip at the origin, blade
  // aimed forward and up, bounded world extents, double-sided emission.
  katana_pose::Frame frame; frame.anchor={0,0,0}; frame.yaw=0.5f; frame.pitchUp=0.2f;
  frame.firstPerson=true; frame.tilt=1.0f; frame.scale=0.052f;
  Pose pose; assert(katana_pose::worldPose(frame,pose));
  out.clear(); assert(triangles(m,pose,true,emit));
  assert(out.size()==15732);
  bool green=false; float maxY=-1e9f; katana_geometry::Vec3 top{};
  for(const auto& v:out) {
   assert(finite(v.position));
   assert(std::fabs(v.position.x)<1.f && std::fabs(v.position.y)<1.f && std::fabs(v.position.z)<1.f);
   if(v.rgba[1]>.79f && v.rgba[0]<.08f) green=true;
   if(v.position.y>maxY) { maxY=v.position.y; top=v.position; }
  }
  assert(green);
  // The raised blade tip must be the highest point and clearly forward along
  // the look direction, proving the orientation is not arbitrary.
  assert(maxY>0.5f);
  const katana_pose::Vec3 hint=katana_pose::horizontalForward(frame.yaw);
  const float forward=top.x*hint.x+top.z*hint.z;
  assert(forward>0.2f);
  // Orthonormal pose axes: pairwise dot products vanish, each axis is unit.
  auto d=[](const Vec3&a,const Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;};
  assert(std::fabs(d(pose.x,pose.y))<1e-4f && std::fabs(d(pose.x,pose.z))<1e-4f && std::fabs(d(pose.y,pose.z))<1e-4f);
  assert(std::fabs(d(pose.x,pose.x)-1)<1e-4f && std::fabs(d(pose.y,pose.y)-1)<1e-4f && std::fabs(d(pose.z,pose.z)-1)<1e-4f);
  assert(std::fabs((pose.pivot.x-5.2f))<1e-3f && std::fabs((pose.pivot.y-2.53f))<1e-3f);
 }
}
