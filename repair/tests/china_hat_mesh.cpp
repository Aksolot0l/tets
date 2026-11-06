#include "../research/china_hat/hat_mesh.h"
#include <cassert>
#include <cmath>
#include <limits>
using namespace china_hat;
int main() {
    Mesh m{};
    assert(build(m, {10,20,-30}, 0.6f, 0.35f, 48, 0.12f));
    assert(m.triangleVertices == 144 && m.outlineVertices == 49);
    for (std::size_t i = 0; i < 48; ++i) {
        auto apex = m.triangles[i*3].position;
        auto rim = m.triangles[i*3+1].position;
        auto next = m.triangles[i*3+2].position;
        assert(apex.x == 10 && apex.y > 20 && apex.z == -30);
        assert(rim.y == 20 && std::fabs(std::hypot(rim.x-10, rim.z+30)-0.6f) < .00001f);
        assert(next.x == m.outline[i+1].position.x && next.z == m.outline[i+1].position.z);
        assert(m.triangles[i*3+1].color.a == 160);
    }
    assert(m.outline[0].position.x == m.outline[48].position.x);
    assert(m.outline[0].position.z == m.outline[48].position.z);
    Mesh other{};
    assert(build(other,{10,20,-30},.6f,.35f,48,1.12f));
    assert(other.outline[0].color.r == m.outline[0].color.r);
    assert(other.outline[0].color.g == m.outline[0].color.g);
    assert(!build(m,{32769,0,0},.6f,.3f,48,0));
    assert(!build(m,{0,0,0},0,.3f,48,0) && m.triangleVertices == 0 && m.outlineVertices == 0);
    assert(!build(m,{0,0,0},.6f,.3f,7,0));
    assert(!build(m,{0,0,0},.6f,.3f,97,0));
    assert(!build(m,{0,0,0},.6f,.3f,48,std::numeric_limits<float>::infinity()));
    assert(!build(m,{std::numeric_limits<float>::quiet_NaN(),0,0},.6f,.3f,48,0));
}
