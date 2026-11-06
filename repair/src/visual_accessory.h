#pragma once
#include "../research/china_hat/hat_mesh.h"
#include <cmath>

// Embedded, untextured test mesh. No file access, game hooks, or player writes.
// Four double-sided walls plus three double-sided triangular points per side.
namespace visual_accessory {
constexpr int vertexCount=4*(12+3*6);
template<class Emit> bool crown(china_hat::Point anchor,float scale,Emit emit) {
 if (!std::isfinite(anchor.x) || !std::isfinite(anchor.y) || !std::isfinite(anchor.z) ||
     std::fabs(anchor.x)>32768.f || std::fabs(anchor.y)>32768.f || std::fabs(anchor.z)>32768.f ||
     !std::isfinite(scale) || scale<.5f || scale>2.f) return false;
 auto triangle=[&](china_hat::Point a,china_hat::Point b,china_hat::Point c) {
  emit(a); emit(b); emit(c); emit(c); emit(b); emit(a);
 };
 // Local X/Z coordinates form an actual 3D square ring, not a screen sprite.
 constexpr float half=.36f, band=.14f, top=.33f;
 for(int side=0;side<4;++side) {
  auto point=[&](float x,float y) {
   float u=x, v=half;
   if(side==1) { u=half; v=-x; }
   else if(side==2) { u=-x; v=-half; }
   else if(side==3) { u=-half; v=x; }
   return china_hat::Point{anchor.x+u*scale,anchor.y+y*scale,anchor.z+v*scale};
  };
  const auto a=point(-half,0),b=point(half,0),c=point(half,band),d=point(-half,band);
  triangle(a,b,c); triangle(a,c,d);
  for(int i=0;i<3;++i) {
   float left=-half+(2.f*half*i/3.f),right=-half+(2.f*half*(i+1)/3.f);
   triangle(point(left,band),point(right,band),point((left+right)*.5f,top));
  }
 }
 return true;
}
}
