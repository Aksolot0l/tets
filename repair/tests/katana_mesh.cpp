#include "../src/katana_mesh.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
using namespace katana_mesh;
static void put32(std::vector<std::uint8_t>& b,std::size_t i,std::uint32_t n) {
 for(int k=0;k<4;++k) b[i+k]=static_cast<std::uint8_t>(n>>(k*8));
}
static void putf(std::vector<std::uint8_t>& b,std::size_t i,float x) { std::uint32_t n=0; std::memcpy(&n,&x,4); put32(b,i,n); }
int main(int argc,char** argv) {
 std::vector<std::uint8_t> b(20+3*24+3*4+24); std::memcpy(b.data(),"GCM1",4);
 put32(b,4,1); put32(b,8,3); put32(b,12,3); put32(b,16,1);
 putf(b,20+24,1.f); putf(b,20+2*24+4,1.f);
 auto ix=20+3*24; put32(b,ix+4,1); put32(b,ix+8,2);
 auto gr=ix+12; put32(b,gr,0); put32(b,gr+4,3); for(int i=0;i<4;++i) putf(b,gr+8+i*4,1.f);
 View m; assert(m.load(b.data(),b.size())); assert(m.vertices()==3 && m.indices()==3 && m.groups()==1);
 Vertex v; Group g; std::uint32_t index=99;
 assert(m.vertex(1,v) && v.position[0]==1.f); assert(m.index(2,index) && index==2); assert(m.group(0,g) && g.count==3 && g.color[3]==1.f);
 assert(!m.vertex(3,v) && !m.index(3,index) && !m.group(1,g));
 assert(!m.load(b.data(),b.size()-1) && m.vertices()==0); assert(!m.vertex(0,v));
 put32(b,ix,3); assert(!m.load(b.data(),b.size())); put32(b,ix,0);
 put32(b,gr+4,2); assert(!m.load(b.data(),b.size())); put32(b,gr+4,3);
 put32(b,20,0x7fc00000); assert(!m.load(b.data(),b.size())); put32(b,20,0);
 putf(b,gr+8,1.2f); assert(!m.load(b.data(),b.size())); putf(b,gr+8,1.f);
 put32(b,8,0xffffffff); assert(!m.load(b.data(),b.size()));
 if(argc>1) {
  std::ifstream f(argv[1],std::ios::binary); assert(f);
  std::vector<std::uint8_t> asset((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
  assert(m.load(asset.data(),asset.size()));
  assert(m.vertices()==4572 && m.indices()==7866 && m.groups()==5);
  assert(m.group(1,g) && g.first==1074 && g.count==888 && g.color[1]>.79f);
  assert(m.index(7865,index) && index<4572);
 }
}
