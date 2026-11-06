#pragma once
namespace visual_depth_policy {
// Strict mode fails closed if the world pass or scoped draw hooks are missing.
inline bool worldDraw(bool strict,bool worldHook,bool gpuHooks,bool context,bool depthView,bool validCompare) {
 return strict && worldHook && gpuHooks && context && depthView && validCompare;
}
inline bool overlayDraw(bool strict) { return !strict; }
}
