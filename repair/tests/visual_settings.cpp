#include "../src/visual_settings.h"
#include <cassert>
#include <limits>
int main() {
 for (std::size_t i=0;i<static_cast<std::size_t>(VisualParam::Count);++i) {
  auto p=static_cast<VisualParam>(i); auto s=visualParamSpecs[i];
  assert(sanitizeVisualParam(p,s.value)==s.value);
  assert(sanitizeVisualParam(p,s.low-10)==s.low);
  assert(sanitizeVisualParam(p,s.high+10)==s.high);
  assert(sanitizeVisualParam(p,std::numeric_limits<float>::quiet_NaN())==s.value);
 }
 assert(sanitizeVisualParam(VisualParam::Count,1.f)==0.f);
}
