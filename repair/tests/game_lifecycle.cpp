#include "../src/game_lifecycle.h"
#include <cassert>
int main() {
    using A = GameExitAction;
    // Idle/launching is never interpreted as an exit.
    assert(gameExitAction(true,false,0,0,false)==A::Wait);
    assert(gameExitAction(true,false,100,0,false)==A::Wait);
    // A missing process snapshot alone must not shut down the launcher.
    assert(gameExitAction(false,false,0,100,true)==A::Wait);
    assert(gameExitAction(true,false,0,100,false)==A::Wait);
    assert(gameExitAction(true,false,100,100,false)==A::Wait);
    // Old PID may still appear briefly, or an additional game may be running.
    assert(gameExitAction(true,false,100,100,true)==A::Wait);
    assert(gameExitAction(true,true,101,100,true)==A::Wait);
    assert(gameExitAction(true,false,101,100,true)==A::WatchReplacement);
    // Only a signalled, observed process plus a clean empty snapshot closes.
    assert(gameExitAction(true,false,0,100,true)==A::CloseLauncher);
    UnwatchedGameExit fallback;
    assert(!fallback.update(true,false,0)); // No observed game.
    fallback.markObserved();
    assert(!fallback.update(true,false,0));
    assert(!fallback.update(false,false,0)); // Snapshot error resets the streak.
    assert(!fallback.update(true,false,0));
    assert(!fallback.update(true,true,100)); // Other game resets the streak.
    assert(!fallback.update(true,false,0));
    assert(!fallback.update(true,false,0));
    assert(fallback.update(true,false,0));
    fallback.reset(); assert(!fallback.update(true,false,0));
}
