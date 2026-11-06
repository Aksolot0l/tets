#pragma once
#include <cmath>
namespace visual_model_capture_policy {
// This is not a new camera: it magnifies the actor's existing world projection.
struct Viewport { float x,y,width,height,minDepth,maxDepth; };
inline bool viewport(const Viewport& source,float size,Viewport& out) {
 if (!std::isfinite(source.width) || !std::isfinite(source.height) || !std::isfinite(source.minDepth) || !std::isfinite(source.maxDepth) || source.width<1 || source.height<1 || source.minDepth<0 || source.maxDepth>1 || source.minDepth>source.maxDepth || !std::isfinite(size) || size<1 || size>1024) return false;
 const float aspect=source.width/source.height;
 if (!std::isfinite(aspect) || aspect<.2f || aspect>5.f) return false;
 const float h=size*3.f, w=h*aspect;
 out={(size-w)*.5f,(size-h)*.5f,w,h,source.minDepth,source.maxDepth};
 return true;
}
inline bool safeDraw(bool contextOk,bool depthOk,bool stencil,bool outputUav,bool streamOutput,bool predication,unsigned viewportCount) {
 return contextOk && depthOk && !stencil && !outputUav && !streamOutput && !predication && viewportCount==1;
}
}
