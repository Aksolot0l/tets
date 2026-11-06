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
removed. The newer world-pass depth experiment is documented below; actual
head-bone attachment and correct occlusion remain unverified.
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

The separate draggable `Visuals` window offers China Hat and Trail editors.
The separate preview attempts to find a skin-shaped texture bound during the
original local-player render, while its editor is open. It uses read-only GPU
resource observation: no CPU readback, file or network transfer. Third-person
view may be necessary. Candidates are not guaranteed to be skins; the UI labels
them for verification and allows cycling. If none is found, a Steve-like sample
is displayed. This is unverified in Minecraft and the texture may not be found.

Even when a skin texture is found, the preview is a separately projected sample,
not the game's model: its bones, custom geometry and true animation are not
extracted. A front-facing head shows the hat; a side-facing example character
with a simulated path shows trail. The hat uses the same mesh builder, timing,
settings and bob as the game. Its camera scale fits the maximum bob envelope,
not the current bob position, so the character should not pulse as the hat
moves (Windows verification required). The GPU material and actual walking history are
not replicated; no exact pixel match is promised. Windows testing is required.

The user observed that the output-merger had a DSV bound and depth enabled
after the entity pass, but depth was disabled around the screen/UI pass. The
experimental default moves the existing effects to the end of the candidate
entity pass. Scoped D3D11 draw detours enforce depth testing using the observed
DSV and comparison, disable depth writes, apply a constant opacity blend factor,
then restore the original depth/blend state. Draws outside our tessDraw scope
pass through. Missing context, a changed context vtable, unavailable hooks, an
invalid depth comparison, missing/mismatched DSV or state creation failure cause
the guarded path to skip work. Hook installation is deferred until the user
enables a visual; partial hook failures are rolled back where possible. The
`Old overlay (see-through)` switch opts back into the previous screen path.

The user reported that the new world rendering, block occlusion and opacity
worked. Precise occlusion for all other cases and the new independent preview
remain experimental and require more Windows testing.
A bound DSV does not prove it contains correct depth for every object, and
`World draw: submitted` is not proof of visibility or correct occlusion.
Private ABI mismatch, driver behaviour, unobserved GPU paths, device changes,
performance issues, artifacts or crashes remain possible. The existing exact
SHA/PE/prologue validation and previous-hash installer path are retained.
The earlier screenshots confirmed a filled surface, but per-vertex gradient
and game blending are not guaranteed. User testing in a new world is advised.

`DIAG_RENDER.ps1` is optional, read-only research. It prints the game EXE hash
(if Windows permits access), PE timestamp/size, and 16 machine-code bytes from
each of eight executable-section candidate offsets. It prints no process paths,
addresses, player state, or names, does not transmit anything, and does not
modify or inject into the game. Candidate bytes alone do not prove compatibility.
See `repair/tests/china_hat_mesh.cpp` for portable geometry tests. The geometry
is compiled into the adapter; the diagnostic is not part of the installed bundle.

### Alternate hat timing diagnostic

User screenshots show the world pass can incorrectly occlude the hat with
faraway terrain, including its wireframe; the depth-disabled legacy overlay
shows the full shape. This does not prove whether depth values, projection,
viewport or renderer timing are responsible. `Hat at player pass (test)` is
off by default. When enabled in strict mode, it tries one guarded hat draw
after the local player renderer within the entity pass, suppressing the normal
post-entities hat draw. It does not change trail or legacy overlay, always
requires the observed context/DSV/depth and restores blend/depth state. Current
status and depth comparison function are exposed; this path has not been
tested in Minecraft, and `submitted` does not mean it is visually correct.

### Player model investigation (read-only executable fragments)
The user supplied a small on-disk code diagnostic for the same SHA-256-verified
1.1.5 build; no player data or full game executable was supplied. The dispatcher
at RVA `0x55d640` chooses a renderer and calls virtual methods with the fourth
argument in `xmm3` and a fifth float on the stack. The known player entry point
at `0x581910` obtains a model-related object via `0x580ee0`, changes fields on
that object, calls a virtual method, and then calls `0x5695d0`. These observations
identify leads, not a proven model ABI, render target interface, camera setup or
safe way to invoke the renderer a second time. The hook now forwards the fourth
argument as a float so the observer cannot clobber it; this still needs testing
in the game. The follow-up offline diagnostic collects only fixed executable
code windows around those callees and the matrix stack, with exact build hash
checking. It does not capture dynamic memory or guarantee a viable render path.
