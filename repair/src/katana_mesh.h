#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Read-only view over offline-converted GCM1 geometry. Independent of the game
// and renderer; callers own the bytes and must keep them alive while using it.
namespace katana_mesh {
constexpr std::uint32_t maxVertices=100000, maxIndices=300000, maxGroups=128;
struct Vertex { float position[3]{}, normal[3]{}; };
struct Group { std::uint32_t first=0, count=0; float color[4]{}; };
inline std::uint32_t u32(const std::uint8_t* p) {
 return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8) | (std::uint32_t(p[2])<<16) | (std::uint32_t(p[3])<<24);
}
inline float f32(const std::uint8_t* p) {
 const auto v=u32(p); float result; static_assert(sizeof(result)==4,"32-bit float required"); std::memcpy(&result,&v,4); return result;
}
class View {
public:
 bool load(const void* bytes,std::size_t length) {
  *this=View{};
  if(!bytes || length<20) return false;
  const auto* p=static_cast<const std::uint8_t*>(bytes);
  if(std::memcmp(p,"GCM1",4)!=0 || u32(p+4)!=1) return false;
  const std::uint32_t v=u32(p+8), i=u32(p+12), g=u32(p+16);
  if(!v || v>maxVertices || !i || i>maxIndices || i%3 || !g || g>maxGroups) return false;
  const std::uint64_t expect=20ull+24ull*v+4ull*i+24ull*g;
  if(expect!=length) return false;
  const std::size_t indexOffset=20+std::size_t(v)*24, groupOffset=indexOffset+std::size_t(i)*4;
  for(std::uint32_t n=0;n<v;++n) for(int k=0;k<6;++k) {
   float x=f32(p+20+std::size_t(n)*24+k*4);
   if(!std::isfinite(x) || std::fabs(x)>1000000.f) return false;
  }
  for(std::uint32_t n=0;n<i;++n) if(u32(p+indexOffset+std::size_t(n)*4)>=v) return false;
  std::uint32_t next=0;
  for(std::uint32_t n=0;n<g;++n) {
   const auto* q=p+groupOffset+std::size_t(n)*24;
   const auto first=u32(q), count=u32(q+4);
   if(first!=next || !count || count%3 || count>i-first) return false;
   for(int k=0;k<4;++k) { float x=f32(q+8+k*4); if(!std::isfinite(x) || x<0 || x>1) return false; }
   next=first+count;
  }
  if(next!=i) return false;
  data_=p; vertices_=v; indices_=i; groups_=g; indexOffset_=indexOffset; groupOffset_=groupOffset; return true;
 }
 std::uint32_t vertices() const { return vertices_; }
 std::uint32_t indices() const { return indices_; }
 std::uint32_t groups() const { return groups_; }
 bool vertex(std::uint32_t i,Vertex& out) const {
  if(!data_ || i>=vertices_) return false;
  const auto* p=data_+20+std::size_t(i)*24;
  for(int k=0;k<3;++k) { out.position[k]=f32(p+k*4); out.normal[k]=f32(p+12+k*4); } return true;
 }
 bool index(std::uint32_t i,std::uint32_t& out) const {
  if(!data_ || i>=indices_) return false;
  out=u32(data_+indexOffset_+std::size_t(i)*4); return true;
 }
 bool group(std::uint32_t i,Group& out) const {
  if(!data_ || i>=groups_) return false;
  const auto* p=data_+groupOffset_+std::size_t(i)*24;
  out.first=u32(p); out.count=u32(p+4); for(int k=0;k<4;++k) out.color[k]=f32(p+8+k*4); return true;
 }
private:
 const std::uint8_t* data_=nullptr; std::uint32_t vertices_=0,indices_=0,groups_=0;
 std::size_t indexOffset_=0,groupOffset_=0;
};
}
