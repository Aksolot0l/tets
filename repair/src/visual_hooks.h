#pragma once
#include <cstddef>

// MinHook operations are injected for portable rollback tests. Do not remove
// a trampoline when disabling its active hook failed.
namespace visual_hooks {
struct Hook { void* target; void* detour; void** original; bool created=false; bool active=false; };
template<class Create,class Enable,class Disable,class Remove>
bool install(Hook* hooks,std::size_t count,Create create,Enable enable,Disable disable,Remove remove) {
 bool success=true;
 for (std::size_t i=0;i<count;++i) {
  if (create(hooks[i].target,hooks[i].detour,hooks[i].original)!=0) { success=false; break; }
  hooks[i].created=true;
 }
 if (success) for (std::size_t i=0;i<count;++i) {
  if (enable(hooks[i].target)!=0) { success=false; break; }
  hooks[i].active=true;
 }
 if (!success) for (std::size_t i=0;i<count;++i) {
  auto& hook=hooks[i];
  if (hook.active && disable(hook.target)==0) hook.active=false;
  if (hook.created && !hook.active) remove(hook.target);
 }
 return success;
}
}
