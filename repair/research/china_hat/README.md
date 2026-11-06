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

`hat_mesh.h` is our independent cone geometry prototype. The installed adapter
now has opt-in experimental surface and wireframe paths, using candidate game
functions and render timing. The path is restricted to the exact user-observed SHA-256, PE
timestamp and image size; diagnostic prologues and loaded code are compared to
the file. If validation or hook installation fails, the feature is unavailable.
Before drawing, it checks player identity, position, dimensions and tessellator
state and restores the global shader color. This reduces risk but does **not**
prove the private offsets, ABI, local-player chain, material state, or camera
semantics. Windows runtime testing is essential; no guarantee of an effect,
correct placement or stability. Other builds never use these hooks.

The hat is off by default; enable `China Hat (test)` in the game menu and use
third-person view. `Hat surface (test)` opts into the experimental filled cone;
when off the previously tested wireframe remains. `Hat height` adjusts vertical
placement; a very small vertical bob is added. Its position follows the local
player's interpolated position and approximate collision-box height, not the
actual animated head bone, so pitch/crouching/animations need testing. The
independent `Trail (test)` draws a bounded, short-lived line from sampled world
positions, not a particle trail. It is off by default; discontinuities reset it.

The adapter attempts a depth test with depth writes off only when the same
render thread has a captured D3D11 context and a bound depth view. It restores
the previous state; the game's material may override it or no depth view may
be bound, so correct occlusion is not guaranteed. The candidate filled topology
and material are not confirmed on 1.1.5; the surface might be invisible, white,
incorrect, or unstable. Before testing, save or use a disposable world. Both
features turn off with Del (which also disables the overlay until restart). The
adapter remains loaded until game exit. Private game-render hooks are installed
only after the user enables one of the features; they remain inert when disabled.

`DIAG_RENDER.ps1` is optional, read-only research. It prints the game EXE hash
(if Windows permits access), PE timestamp/size, and 16 machine-code bytes from
each of eight executable-section candidate offsets. It prints no process paths,
addresses, player state, or names, does not transmit anything, and does not
modify or inject into the game. Candidate bytes alone do not prove compatibility.
See `repair/tests/china_hat_mesh.cpp` for portable geometry tests. The geometry is compiled into the adapter; the separate diagnostic is not part
of the installed client bundle.
