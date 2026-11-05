#include "../src/wheel_counter.h"
#include <cassert>
int main() {
    WheelCounter counter;
    int x = 7, y = 7;
    counter.take(100, 200, x, y); assert(x == 0 && y == 0);
    counter.take(100, 80, x, y); assert(x == 0 && y == -120);
    counter.take(220, 200, x, y); assert(x == 120 && y == 120);
    counter.take(220, 20200, x, y); assert(y == 12000);
    counter.reset(); counter.take(0, 0xfffffff0u, x, y); assert(y == 0);
    counter.take(0, 0x68u, x, y); assert(y == 120);
    counter.take(0, 0xfffffff0u, x, y); assert(y == -120);
}
