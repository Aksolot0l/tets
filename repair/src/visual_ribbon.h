#pragma once
#include "../research/china_hat/hat_mesh.h"
#include <cmath>
namespace visual_ribbon {
// One vertical trail panel as two triangles, emitted in both orientations.
// Points are already camera-relative. This is geometry only, not a promise
// that the native game accepts the candidate triangle topology/material.
template<class Emit>
bool segment(const china_hat::Vertex& a,const china_hat::Vertex& b,float height,Emit emit) {
 if (!std::isfinite(height) || height<=0 || height>2.0f) return false;
 for (const auto p:{a.position,b.position})
  if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return false;
 auto upperA=a, upperB=b; upperA.position.y+=height; upperB.position.y+=height;
 emit(a); emit(b); emit(upperB); emit(a); emit(upperB); emit(upperA);
 emit(upperB); emit(b); emit(a); emit(upperA); emit(upperB); emit(a);
 return true;
}
}
