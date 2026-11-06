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
experimental trail into its own batch. The failed surface and depth changes
are disabled; actual head-bone attachment and correct occlusion are not done.
Both options are opt-in, and the revised build still requires testing on Windows.

The path is restricted to the exact user-observed SHA-256, PE timestamp and
image size; diagnostic prologues and loaded code are compared to the file.
If validation or hook installation fails, the feature is unavailable.
Before drawing, it checks player identity, position, dimensions and tessellator
state and restores the global shader color. This does not prove private ABI,
local-player chain or camera semantics. Windows runtime testing is essential.

The hat is off by default; enable `China Hat (test)` and use third-person view.
`Hat height` adjusts placement, which follows interpolated player position and
approximate hitbox height, not the animated head bone. `Trail (test)` draws a
bounded, short-lived line based on world positions. `submitted` in the menu
means the code called the game renderer; it does not prove visible output.
Del disables both features and the overlay; the adapter remains loaded until
exit. Hooks are installed only after one of these features is enabled.

`DIAG_RENDER.ps1` is optional, read-only research. It prints the game EXE hash
(if Windows permits access), PE timestamp/size, and 16 machine-code bytes from
each of eight executable-section candidate offsets. It prints no process paths,
addresses, player state, or names, does not transmit anything, and does not
modify or inject into the game. Candidate bytes alone do not prove compatibility.
See `repair/tests/china_hat_mesh.cpp` for portable geometry tests. The geometry is compiled into the adapter; the separate diagnostic is not part
of the installed client bundle.
