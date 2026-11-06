# China Hat: experimental, build-specific game feature

Java example: FDPClient's `ChineseHat` handles a 3D render event, interpolates
entity position and emits a cone relative to the camera. It uses Forge/OpenGL,
not the Windows 10 Edition 1.1.5 renderer, so its code cannot be dropped into
this UWP adapter. Source (GPL-3.0, not copied into this project):
https://github.com/SkidderMC/FDPClient/blob/5635c781f2c0344f5d94abe86e368ccac084b228/src/main/java/net/ccbluex/liquidbounce/features/module/modules/visual/Hat.kt

Independent research lead: Zodiak describes itself as an internal client for
Windows 10 Edition 1.1.5; its source includes candidate player-render and
Tessellator entry points. Its offsets are *not* proof they match the user's
particular binary, and the repository has no license. No source was copied.
https://github.com/WinsentShame/Zodiak/tree/d2fcb0ffd4f8ada69ca4d84a83d3f546b300d01d

`hat_mesh.h` is an independent cone geometry prototype. The user confirmed an
older wireframe build in game. After a subsequent surface/depth/trail update
caused both effects to disappear for the user, this build restores the old
wireframe draw sequence (with an optional vertical offset) and separates the
experimental trail into its own batch. That earlier surface/depth attempt was
removed. New optional surface batches are documented below; no forced depth
state is used. Actual head-bone attachment and correct occlusion are not done.
Both effects are opt-in, and new changes still require testing on Windows.
A subsequent user screenshot showed the old coordinate chain identifying an
unrelated character on a server. The visual effects no longer use that chain:
they read the candidate local-player slot of a recently observed client
instance. A candidate 1.1.5 tick entry point is hooked only on opt-in, with
file/memory code comparison, exact executable hash gate and rollback. The tick
ABI and local slot have an initial user confirmation but are not fully validated. The standalone coordinates
experiment is separate and can still use the old, potentially nonlocal path.

The path is restricted to the exact user-observed SHA-256, PE timestamp and
image size; diagnostic prologues and loaded code are compared to the file.
If validation or hook installation fails, the feature is unavailable.
Before drawing, it checks player identity, position, dimensions and tessellator
state and restores the global shader color. This does not prove private ABI,
local-player chain or camera semantics. Windows runtime testing is essential.

The separate draggable `Visuals` window contains `China Hat`, `Trail`, and
`Options` tabs. Both modules are off by default; their per-session controls
include fill, animated whole-material rainbow or fixed color, rainbow speed,
shape/position/bob for the hat, and ribbon height/lifetime for the trail.
Settings are not persisted. The material tint and simple local-player lines were
confirmed by the user; the new UI, animated colors and controls are untested on
Windows. Del disables both features and the overlay; hooks remain installed.

Fill is on by default for each module once enabled, but can be turned off in
the UI before enabling the module. It uses a candidate mode-2 triangle batch
after the known line batch and checks the tessellator is idle first. There is
no proof mode 2 or this material works in the user's game: the result may be
invisible, cause corruption, or crash. `Fill: submitted` means only that the
batch was sent, not that it was visible. The game controls depth and blending;
correct occlusion and opacity are not guaranteed. The hat still follows player
position and hitbox rather than the head bone. When fill is disabled the known
line rendering remains available. All new behavior requires Windows testing.

`DIAG_RENDER.ps1` is optional, read-only research. It prints the game EXE hash
(if Windows permits access), PE timestamp/size, and 16 machine-code bytes from
each of eight executable-section candidate offsets. It prints no process paths,
addresses, player state, or names, does not transmit anything, and does not
modify or inject into the game. Candidate bytes alone do not prove compatibility.
See `repair/tests/china_hat_mesh.cpp` for portable geometry tests. The geometry is compiled into the adapter; the separate diagnostic is not part
of the installed client bundle.
