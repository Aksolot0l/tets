#pragma once
#include <cstdint>

// No WinRT calls or pointers. The game handler's +0x72 is its reported grab
// state; +0x74 is its pending command (0 none, 1 grab, 2 release) in the
// exact executable. Async transitions can finish after the menu closes.
namespace menu_mouse {
struct CursorPolicy {
 bool open=false, restore=false, releasePending=false, closing=false;
 std::uint64_t closingSince=0;
 struct Decision { std::uint32_t command; bool entering; };
 Decision step(bool menu, bool grabbed, std::uint32_t command, std::uint64_t now) {
  const bool entering=menu && !open;
  if (entering) {
   open=true; closing=false; releasePending=false;
   restore=command==2 ? false : (grabbed || command==1);
  }
  if (menu) {
   if (!entering && command==1) restore=true;
   if (!entering && command==2) restore=false;
   // Do not allow a grab during the menu. A release request only needs to be
   // scheduled once: the game's release method clears its command and posts
   // an asynchronous callback that later changes the reported grab state.
   if (command==1) command=0;
   if (!grabbed) { releasePending=false; if (command==2) command=0; }
   else if (!releasePending) { command=2; releasePending=true; }
   return {command,entering};
  }
  if (open) { open=false; closing=restore; closingSince=now; }
  if (closing) {
   if (command==2 || now<closingSince || now-closingSince>5000) {
    closing=false;
   } else if (!grabbed) {
    // The release callback completed; restore the state that preceded us.
    command=1; closing=false; releasePending=false;
   } else if (!releasePending) closing=false;
  }
  return {command,false};
 }
};
// Only buttons previously forwarded to the game may have their up edge
// forwarded while the menu is open. New button presses, movement and wheel
// are discarded. A caller can instead synthesize all outstanding up edges.
inline unsigned buttonBit(unsigned action) { return action>=1 && action<=3 ? 1u<<(action-1) : 0u; }
inline bool allowMouse(bool menu, unsigned action, bool pressed, unsigned oldHeld) {
 return !menu || (!pressed && (oldHeld & buttonBit(action)) != 0);
}
}
