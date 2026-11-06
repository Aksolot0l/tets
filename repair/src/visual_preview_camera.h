#pragma once
// Historical, unused candidate policy retained for research and tests only.
// Getter interception was removed after it interfered with F5 and the hand;
// this header is not wired to the game. Do not re-enable it without evidence.
namespace visual_preview_camera {
inline bool overrideFront(bool requested,bool livePlayer,bool optionsValid,int value) {
 return requested && livePlayer && optionsValid && value>=0 && value<=2;
}
inline int resolve(bool requested,bool livePlayer,bool optionsValid,int original) {
 return overrideFront(requested,livePlayer,optionsValid,original)?2:original;
}
}
