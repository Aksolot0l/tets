#pragma once
// Candidate Bedrock perspective values for the exact validated build.
// Do not set/persist options: override only the getter result while the
// explicitly requested model preview is active, and only for a live player.
namespace visual_preview_camera {
inline bool overrideFront(bool requested,bool livePlayer,bool optionsValid,int value) {
 return requested && livePlayer && optionsValid && value>=0 && value<=2;
}
inline int resolve(bool requested,bool livePlayer,bool optionsValid,int original) {
 return overrideFront(requested,livePlayer,optionsValid,original)?2:original;
}
}
