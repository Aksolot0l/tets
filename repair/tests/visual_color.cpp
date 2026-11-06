#include "../src/visual_color.h"
#include <cassert>
#include <cmath>
int main() {
 using namespace visual_color;
 for (auto effect:{Effect::Hat,Effect::Trail}) {
  auto original=material(false,effect);
  for (float x:original) assert(x==1.f);
  auto color=material(true,effect);
  for (float x:color) assert(std::isfinite(x) && x>=0.f && x<=1.f);
  assert(color[3]==1.f && (color[0]<.9f || color[1]<.9f || color[2]<.9f));
 }
 assert(material(true,Effect::Hat)!=material(true,Effect::Trail));
}
