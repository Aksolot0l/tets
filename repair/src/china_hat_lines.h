#pragma once
#include "../research/china_hat/hat_mesh.h"
#include <cstddef>
// Emit an independent ring plus evenly spaced apex-to-rim spokes. The caller
// chooses line-list topology/material, and owns all native rendering state.
namespace china_hat {
template<class Emit> bool emitWireframe(const Mesh& mesh, std::size_t spokes, Emit emit) {
 if (mesh.outlineVertices<9 || mesh.outlineVertices>kMaxSegments+1) return false;
 const std::size_t n=mesh.outlineVertices-1;
 if (!spokes || spokes>n || n%spokes || mesh.triangleVertices!=n*3) return false;
 for (std::size_t i=0;i<n;i++) { emit(mesh.outline[i]); emit(mesh.outline[i+1]); }
 for (std::size_t i=0;i<n;i+=n/spokes) { emit(mesh.triangles[i*3]); emit(mesh.outline[i]); }
 return true;
}
}
