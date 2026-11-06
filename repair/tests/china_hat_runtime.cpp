#include "../src/china_hat_lines.h"
#include "../src/china_hat_validation.h"
#include <cassert>
#include <vector>
#include <cmath>
#include <cstring>
int main() {
 using namespace china_hat;
 Mesh m{}; assert(build(m,{2,3,4},.58f,.34f,48,.25f,255));
 std::vector<Vertex> surface;
 assert(emitDoubleSidedSurface(m,[&](const Vertex& v){surface.push_back(v);}));
 assert(surface.size()==288);
 for (std::size_t i=0;i<48;i++) {
  const auto& a=surface[i*6]; const auto& b=surface[i*6+1]; const auto& c=surface[i*6+2];
  assert(a.position.y>b.position.y && a.position.y>c.position.y);
  assert(surface[i*6+3].position.y==a.position.y);
  assert(surface[i*6+4].position.x==c.position.x && surface[i*6+5].position.x==b.position.x);
 }
 std::vector<Vertex> list;
 assert(emitWireframe(m,12,[&](const Vertex& v){list.push_back(v);}));
 assert(list.size()==120);
 for (int i=0;i<48;i++) {
  auto a=list[i*2].position, b=list[i*2+1].position;
  assert(std::fabs(a.y-3.0f)<1e-6 && std::fabs(b.y-3.0f)<1e-6);
  assert(std::fabs(a.x-m.outline[i].position.x)<1e-6);
  assert(std::fabs(b.z-m.outline[i+1].position.z)<1e-6);
 }
 for (int i=0;i<12;i++) assert(std::fabs(list[96+i*2].position.y-3.34f)<1e-5);
 list.clear(); assert(!emitWireframe(m,13,[&](const Vertex& v){list.push_back(v);})); assert(list.empty());
 m.triangleVertices=0; assert(!emitWireframe(m,12,[&](const Vertex& v){list.push_back(v);}));
 std::uint8_t hash[32]; std::memcpy(hash,hat_validation::kHash,sizeof hash);
 assert(hat_validation::identity(hash,hat_validation::kTimestamp,hat_validation::kImageSize));
 hash[31]^=1; assert(!hat_validation::identity(hash,hat_validation::kTimestamp,hat_validation::kImageSize));
 assert(!hat_validation::identity(nullptr,hat_validation::kTimestamp,hat_validation::kImageSize));
 assert(!hat_validation::identity(hat_validation::kHash,0,hat_validation::kImageSize));
 assert(!hat_validation::identity(hat_validation::kHash,hat_validation::kTimestamp,hat_validation::kImageSize+1));
}
