# China Hat: experimental research, not an installed game feature

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

`hat_mesh.h` is our independent cone geometry prototype with tests. It is not
called by the installed adapter and does not appear in the game. It needs an
identified player/head position, render timing, material/depth state, game
camera transform, correct calling convention, and a compatible game binary.
Do not hook or call arbitrary RVAs until these are validated on the exact game
build. Third-person, crouch, view-bobbing, first-person hiding, other players,
transparency, graphics-state restoration, and teardown all need runtime tests.

`DIAG_RENDER.ps1` is optional, read-only research. It prints the game EXE hash
(if Windows permits access), PE timestamp/size, and 16 machine-code bytes from
each of eight executable-section candidate offsets. It prints no process paths,
addresses, player state, or names, does not transmit anything, and does not
modify or inject into the game. Candidate bytes alone do not prove compatibility.
See `repair/tests/china_hat_mesh.cpp` for portable geometry tests. Neither the
geometry nor this diagnostic is part of the installed client bundle.
