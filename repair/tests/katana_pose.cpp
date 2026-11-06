#include "../src/katana_pose.h"
#include "../src/katana_geometry.h"
#include <cassert>
#include <cmath>
#include <limits>
using namespace katana_pose;

static float dot3(Vec3 a,Vec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }

int main() {
 // Rotation helpers: the horizontal forward matches the movement math that
 // was validated for this executable, and the right vector is perpendicular.
 assert(std::fabs(dot3(horizontalForward(0.7f),right(0.7f)))<1e-6f);
 assert(std::fabs(dot3(look(0.7f,0.3f),look(0.7f,0.3f))-1.f)<1e-5f);
 const Vec3 l=look(0.0f,0.0f);
 assert(std::fabs(l.x)<1e-6f && std::fabs(l.y)<1e-6f && l.z<-0.999f);
 const Vec3 up=look(0.0f,1.4f);
 assert(up.y>0.98f);  // positive pitch means looking up

 // A first-person frame: grip right/below/in front of the camera.
 Frame f; f.anchor={0.34f,-0.40f,0.30f}; f.yaw=0.f; f.pitchUp=0.f;
 f.firstPerson=true; f.tilt=1.0f; f.scale=0.052f;
 katana_geometry::Pose pose;
 assert(worldPose(f,pose));
 // Axes are orthonormal and the pose is usable by the geometry expander.
 assert(std::fabs(dot3({pose.x.x,pose.x.y,pose.x.z},{pose.y.x,pose.y.y,pose.y.z}))<1e-4f);
 assert(std::fabs(dot3({pose.x.x,pose.x.y,pose.x.z},{pose.z.x,pose.z.y,pose.z.z}))<1e-4f);
 assert(std::fabs(dot3({pose.y.x,pose.y.y,pose.y.z},{pose.z.x,pose.z.y,pose.z.z}))<1e-4f);
 assert(std::fabs(dot3({pose.x.x,pose.x.y,pose.x.z},{pose.x.x,pose.x.y,pose.x.z})-1.f)<1e-4f);
 // Blade direction = -pose.x: up and forward for a positive tilt.
 const Vec3 blade=scale({pose.x.x,pose.x.y,pose.x.z},-1.f);
 assert(blade.y>0.5f);
 assert(dot3(blade,horizontalForward(0.f))>0.3f);
 // The broad blade side faces the camera (opposite the look direction).
 assert(dot3({pose.z.x,pose.z.y,pose.z.z},scale(look(0.f,0.f),-1.f))>0.3f);
 // The model's tip lands forward and above the anchor.
 const float tipAlong=14.09f*f.scale;
 const Vec3 tip=add(f.anchor,scale(blade,tipAlong));
 assert(tip.y>f.anchor.y+0.5f);
 assert(dot3(add(tip,scale(f.anchor,-1.f)),horizontalForward(0.f))>0.3f);

 // Straight up/down looks must not degenerate: the blade stays tilted in the
 // yaw plane instead of producing NaNs.
 Frame vertical=f; vertical.pitchUp=1.5707f;
 katana_geometry::Pose vpose;
 assert(worldPose(vertical,vpose));
 Frame down=f; down.pitchUp=-1.5707f;
 assert(worldPose(down,vpose));
 assert(std::isfinite(vpose.x.x) && std::isfinite(vpose.origin.x));

 // Third-person frame anchored at the player's hand.
 Frame t; t.anchor={0.32f,1.30f,0.10f}; t.yaw=2.2f; t.pitchUp=-0.4f;
 t.firstPerson=false; t.tilt=1.0f; t.scale=0.052f;
 katana_geometry::Pose tp;
 assert(worldPose(t,tp));
 const Vec3 tblade=scale({tp.x.x,tp.x.y,tp.x.z},-1.f);
 assert(tblade.y>0.5f);
 assert(dot3(tblade,horizontalForward(2.2f))>0.3f);

 // Rejected inputs: bad tilt, scale, non-finite anchor.
 Frame bad=f; bad.tilt=2.0f; assert(!worldPose(bad,pose));
 bad=f; bad.tilt=-0.1f; assert(!worldPose(bad,pose));
 bad=f; bad.scale=0.f; assert(!worldPose(bad,pose));
 bad=f; bad.anchor={std::numeric_limits<float>::quiet_NaN(),0,0}; assert(!worldPose(bad,pose));
 bad=f; bad.yaw=std::numeric_limits<float>::infinity(); assert(!worldPose(bad,pose));

 // First/third person discrimination with hysteresis.
 const Vec3 eye{10.f,64.f,-20.f};
 assert(firstPersonView(eye,eye,false));
 assert(firstPersonView(eye,eye,true));
 assert(!firstPersonView({eye.x+4.f,eye.y,eye.z},eye,false));
 // Distances inside the hysteresis band keep the previous mode.
 assert(firstPersonView({eye.x,eye.y,eye.z-0.95f},eye,true));
 assert(!firstPersonView({eye.x,eye.y,eye.z-0.95f},eye,false));
 // Unusable input keeps the previous mode instead of guessing.
 assert(firstPersonView({std::numeric_limits<float>::quiet_NaN(),0,0},eye,true));
}
