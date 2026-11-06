#pragma once
namespace visual_depth_policy {
// Strict mode fails closed if the world pass or scoped draw hooks are missing.
inline bool worldDraw(bool strict,bool worldHook,bool gpuHooks,bool context,bool depthView,bool validCompare) {
 return strict && worldHook && gpuHooks && context && depthView && validCompare;
}
inline bool overlayDraw(bool strict) { return !strict; }
// A diagnostic alternative to the post-entities hat draw, never a fallback to
// the depth-disabled screen path. Both paths must not draw in the same pass.
inline bool playerHat(bool strict,bool selected,bool inEntities,bool local,bool valid,bool attempted) {
 return strict && selected && inEntities && local && valid && !attempted;
}
inline bool entitiesHat(bool selected) { return !selected; }
}
