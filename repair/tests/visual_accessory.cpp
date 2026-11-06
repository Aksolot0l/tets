#include "../src/visual_accessory.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
int main() {
 using namespace visual_accessory;
 using china_hat::Point;
 std::vector<Point> v;
 auto collect=[&](Point p) { v.push_back(p); };
 assert(crown({2,64,3},1.f,collect));
 assert(v.size()==static_cast<std::size_t>(vertexCount));
 float minX=3,maxX=1,minZ=4,maxZ=2,minY=65,maxY=63;
 for(const auto& p:v) {
  assert(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z));
  minX=std::min(minX,p.x);maxX=std::max(maxX,p.x);minY=std::min(minY,p.y);maxY=std::max(maxY,p.y);minZ=std::min(minZ,p.z);maxZ=std::max(maxZ,p.z);
 }
 assert(std::fabs(minX-1.64f)<.001f && std::fabs(maxX-2.36f)<.001f);
 assert(std::fabs(minZ-2.64f)<.001f && std::fabs(maxZ-3.36f)<.001f);
 assert(std::fabs(minY-64.f)<.001f && std::fabs(maxY-64.33f)<.001f);
 v.clear(); assert(!crown({0,0,0},.49f,collect)); assert(v.empty());
 assert(!crown({0,0,0},2.01f,collect)); assert(v.empty());
 assert(!crown({0,std::numeric_limits<float>::quiet_NaN(),0},1.f,collect)); assert(v.empty());
 assert(!crown({0,0,40000},1.f,collect)); assert(v.empty());
 assert(crown({0,0,0},.5f,collect)); assert(v.size()==static_cast<std::size_t>(vertexCount));
}
