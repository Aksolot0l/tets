# Cyberpunk Katana (research asset)

- **Model:** «Катана в стиле киберпанк» / Cyberpunk Katana
- **Author:** dentrapank, https://sketchfab.com/dentrapank
- **Source:** https://sketchfab.com/3d-models/c7149629b9424f7baee0918751bcdf30
- **License:** Creative Commons Attribution 4.0 International (CC BY 4.0):
  https://creativecommons.org/licenses/by/4.0/
- **Changes:** the GLB is an unmodified copy of the file supplied by the user.
- **SHA-256:** `83996618e51bb3533ae4af3b04fefca2522101c9e31bfcd8b9368ce7973df5d3`.

This asset is included for investigation only. It is not yet loaded by the
client, and there is currently no diamond-sword replacement. Its GLB metadata
identifies five triangle meshes with five materials, no texture images, no
skinning, and no animations. Any future conversion must retain attribution and
mark modifications.

### Offline conversion (experimental, not used by the client)

`python repair/tools/convert_katana.py repair/assets/models/cyberpunk-katana.glb katana.gcm`
converts this static, untextured model into the experimental GCM1 format. The
output is a derivative of the model above; retain its author/source/license
attribution and state that the geometry and base colors were converted when
distributing it. PBR properties (including emissive and metallic), extra vertex
attributes and material culling settings are not preserved. Placement relative
to the hand and the game renderer are not implemented.

GCM1 is little-endian: ASCII `GCM1`, uint32 version (1), uint32 vertex count,
uint32 index count, uint32 group count; then vertices of six float32 values
(position xyz, normal xyz), uint32 triangle indices, and groups of uint32 first
index, uint32 index count, four float32 base-color factors (RGBA). Groups cover
all indices consecutively. Reader limits are 100,000 vertices, 300,000 indices
and 128 groups. `src/katana_mesh.h` validates the entire byte buffer before
exposing a borrowed read-only view; its backing bytes must remain alive and
unchanged throughout use. It is standalone and not wired into the DLL.
