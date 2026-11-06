#include "../src/visual_preview_camera.h"
#include <cassert>
int main() {
 using namespace visual_preview_camera;
 assert(resolve(true,true,true,0)==2); assert(resolve(true,true,true,1)==2);
 assert(resolve(true,true,true,2)==2); assert(resolve(true,true,true,3)==3);
 assert(resolve(false,true,true,0)==0); assert(resolve(true,false,true,0)==0);
 assert(resolve(true,true,false,1)==1); assert(resolve(true,true,true,-1)==-1);
 // The user's stored setting is an input only: turning the request off restores
 // the exact original returned by the game, including a changed setting.
 assert(resolve(true,true,true,0)==2 && resolve(false,true,true,0)==0);
 assert(resolve(true,true,true,1)==2 && resolve(false,true,true,1)==1);
}
