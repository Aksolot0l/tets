#include "../src/menu_mouse_policy.h"
#include <cassert>
#include <cstdint>
using namespace menu_mouse;
int main() {
 CursorPolicy p;
 auto d=p.step(false,true,0,0); assert(!d.entering && d.command==0);
 d=p.step(true,true,0,10); assert(d.entering && d.command==2 && p.restore && p.releasePending);
 d=p.step(true,true,0,20); assert(!d.entering && d.command==0); // no repeated async releases
 d=p.step(true,false,0,30); assert(d.command==0 && !p.releasePending);
 d=p.step(false,false,0,40); assert(d.command==1 && !p.closing);
 d=p.step(false,true,0,50); assert(d.command==0);
 // Menu opened over an already released game UI: do not grab on close.
 CursorPolicy ui; assert(ui.step(true,false,0,0).command==0);
 assert(ui.step(false,false,0,1).command==0);
 // Closing before the async release callback must wait for its completion.
 CursorPolicy async; assert(async.step(true,true,0,10).command==2);
 assert(async.step(false,true,0,20).command==0 && async.closing);
 assert(async.step(false,false,0,30).command==1 && !async.closing);
 // Game's own release while menu is active cancels our restoration.
 CursorPolicy game; assert(game.step(true,true,0,1).command==2);
 assert(game.step(true,false,0,2).command==0);
 assert(game.step(true,false,2,3).command==0 && !game.restore);
 assert(game.step(false,false,0,4).command==0);
 // Game requests a grab while menu is open: defer until menu closes.
 CursorPolicy regrab; assert(regrab.step(true,false,0,1).command==0);
 assert(regrab.step(true,false,1,2).command==0 && regrab.restore);
 assert(regrab.step(false,false,0,3).command==1);
 // No event added by the menu can create a new game press or move/wheel.
 for(unsigned action=0;action<5;++action) {
  assert(!allowMouse(true,action,true,7));
  assert(allowMouse(false,action,true,0));
 }
 assert(!allowMouse(true,0,false,7)); assert(!allowMouse(true,4,false,7));
 assert(allowMouse(true,1,false,1)); assert(!allowMouse(true,1,false,0));
 assert(buttonBit(1)==1 && buttonBit(2)==2 && buttonBit(3)==4 && !buttonBit(4));
 // Failed async callback: no permanently pending restore.
 CursorPolicy timeout; timeout.step(true,true,0,1); timeout.step(false,true,0,2);
 timeout.step(false,true,0,5003); assert(!timeout.closing);
}
