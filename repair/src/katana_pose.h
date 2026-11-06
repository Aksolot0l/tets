#pragma once
#include "katana_geometry.h"
#include <cmath>

// Pure CPU pose for the held-item katana experiment. No game or D3D access.
//
// The katana is positioned in the same camera-relative space the proven hat /
// jump-ring path uses: vertices are submitted as worldPosition - cameraOrigin
// and transformed by the game's own view-projection at tessellator flush time.
//
// Orientation model (validated against the converted GCM1 bounds):
//   model -X : blade direction (tip at model x = -8.89)
//   model +X : handle/pommel direction (pommel at model x = +8.33)
//   model +Y : blade top / edge side
//   model +Z : thin flat side (normal of the broad blade faces)
//
// The pose places the grip pivot at the anchor and aims the blade forward and
// up along the player's look direction, mirroring how the vanilla sword is
// held. Values are defaults plus bounded UI parameters, not a claim that the
// result is already verified in game.
namespace katana_pose {
struct Vec3 { float x=0,y=0,z=0; };

inline float dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline Vec3 add(Vec3 a, Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 scale(Vec3 a, float f) { return {a.x*f,a.y*f,a.z*f}; }
inline bool finite(Vec3 v) {
 return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
        std::fabs(v.x)<=100000.f && std::fabs(v.y)<=100000.f && std::fabs(v.z)<=100000.f;
}
inline bool unit(Vec3 v) { const float d=dot(v,v); return std::isfinite(d) && d>1e-8f; }
inline Vec3 normalize(Vec3 v) { const float d=std::sqrt(dot(v,v)); return {v.x/d,v.y/d,v.z/d}; }

// Look direction for the rotation values observed on this 1.1.5 build. The
// horizontal part matches the empirically validated movement math used by a
// public cheat table for the same executable (forward motion is driven by the
// negative sine and cosine of the yaw-like value); vertical is sine of the
// pitch-like value where positive means looking up.
inline Vec3 look(float yaw, float pitchUp) {
 const float c=std::cos(pitchUp);
 return {-std::sin(yaw)*c, std::sin(pitchUp), -std::cos(yaw)*c};
}
inline Vec3 horizontalForward(float yaw) { return {-std::sin(yaw),0.f,-std::cos(yaw)}; }
inline Vec3 right(float yaw) { return {std::cos(yaw),0.f,-std::sin(yaw)}; }

// Model-space grip pivot: centre of the black handle section, just behind the
// guard, derived from the converted model bounds (handle x 3.30..8.33,
// y 2.06..3.01, grey guard at x 2.69..4.49).
constexpr float gripPivot[3]={5.2f,2.53f,0.f};

struct Frame {
 Vec3 anchor{};       // grip position, already camera-relative
 float yaw=0.f;       // rotation value as read from the game
 float pitchUp=0.f;   // positive = looking up
 bool firstPerson=false;
 float tilt=1.0f;     // radians above horizontal for the blade axis
 float scale=0.052f;  // world units per model unit
};

// Builds the model->world pose. Returns false for non-finite or degenerate
// input; the caller must then keep the original item untouched.
inline bool worldPose(const Frame& f, katana_geometry::Pose& out) {
 if (!finite(f.anchor) || !std::isfinite(f.yaw) || !std::isfinite(f.pitchUp) ||
     !std::isfinite(f.tilt) || f.tilt<0.f || f.tilt>1.6f ||
     !std::isfinite(f.scale) || f.scale<=0.f || f.scale>1.f) return false;
 const Vec3 up{0,1,0};
 const Vec3 hint=horizontalForward(f.yaw);   // never degenerate
 const Vec3 lookDir=look(f.yaw,f.pitchUp);
 if (!unit(hint) || !unit(lookDir)) return false;
 // Blade axis W: forward tilted up by the requested angle.
 const float ct=std::cos(f.tilt), st=std::sin(f.tilt);
 Vec3 w=add(scale(hint,ct),scale(up,st));
 if (!unit(w)) return false;
 w=normalize(w);
 // Flat-side normal F: face the broad blade side roughly toward the camera
 // (opposite the look direction), orthogonalised against W.
 Vec3 f0=add(scale(lookDir,-1.f),scale(up,-0.12f));
 if (!unit(f0)) return false;
 f0=normalize(f0);
 const float d=dot(f0,w);
 Vec3 flatAxis=add(f0,scale(w,-d));
 if (!unit(flatAxis)) return false;
 flatAxis=normalize(flatAxis);
 // Width axis E completes the frame; sign chosen so the model top faces up.
 Vec3 width=cross(w,flatAxis);
 if (!unit(width)) return false;
 width=normalize(width);
 if (width.y<0) width=scale(width,-1.f);
 if (!finite(w) || !finite(flatAxis) || !finite(width)) return false;
 out=katana_geometry::Pose{};
 out.origin={f.anchor.x,f.anchor.y,f.anchor.z};
 out.pivot={gripPivot[0],gripPivot[1],gripPivot[2]};
 out.x={-w.x,-w.y,-w.z};   // model +X is the handle direction, blade is -X
 out.y={width.x,width.y,width.z};
 out.z={flatAxis.x,flatAxis.y,flatAxis.z};
 out.scale=f.scale;
 return true;
}

// First-person discriminator: the render camera sits at the eye in first
// person and at a boom distance in third person. Small hysteresis avoids
// flicker at the boundary.
inline bool firstPersonView(Vec3 camera, Vec3 eye, bool previous) {
 if (!finite(camera) || !finite(eye)) return previous;
 const Vec3 d=add(camera,scale(eye,-1.f));
 const float dist=std::sqrt(dot(d,d));
 return previous ? dist<1.1f : dist<0.8f;
}
}
