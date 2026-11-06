# Cyberpunk Katana (research asset)

- **Model:** «Катана в стиле киберпанк» / Cyberpunk Katana
- **Author:** dentrapank, https://sketchfab.com/dentrapank
- **Source:** https://sketchfab.com/3d-models/c7149629b9424f7baee0918751bcdf30
- **License:** Creative Commons Attribution 4.0 International (CC BY 4.0):
  https://creativecommons.org/licenses/by/4.0/
- **Changes:** the GLB is an unmodified copy of the file supplied by the user.
- **SHA-256:** `83996618e51bb3533ae4af3b04fefca2522101c9e31bfcd8b9368ce7973df5d3`.

The original GLB has five triangle meshes and materials, no texture images, no
skinning and no animations. A simplified converted derivative is embedded in
the opt-in experimental first-person renderer, not yet tested in Minecraft.
Third-person replacement is not implemented.

### Offline conversion and embedding

`python repair/tools/convert_katana.py repair/assets/models/cyberpunk-katana.glb katana.gcm`
converts this static, untextured model into the experimental GCM1 format. The
output is a derivative of the model above; retain its author/source/license
attribution and state that the geometry and base colors were converted when
distributing it. PBR properties (including emissive and metallic), extra vertex
attributes and material culling settings are not preserved. An approximate grip transform is used by an experimental
first-person renderer;
its position and graphics material have not been tested in the game.

GCM1 is little-endian: ASCII `GCM1`, uint32 version (1), uint32 vertex count,
uint32 index count, uint32 group count; then vertices of six float32 values
(position xyz, normal xyz), uint32 triangle indices, and groups of uint32 first
index, uint32 index count, four float32 base-color factors (RGBA). Groups cover
all indices consecutively. Reader limits are 100,000 vertices, 300,000 indices
and 128 groups. `src/katana_mesh.h` validates the entire byte buffer before
exposing a borrowed read-only view; its backing bytes must remain alive and
unchanged throughout use. The experimental first-person DLL module uses it.

`src/katana_geometry.h` expands the mesh into colored triangles without any
game calls. Its approximate grip pivot is derived from the supplied model, not
from Minecraft, and requires in-game calibration. It passes raw linear base
color factors to the caller and does not reconstruct lighting or emissive PBR.
