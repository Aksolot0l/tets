# GAYClient HUD redesign — visual layer

Integrated from `deliverables/gayclient-hud-visual.patch` in the user's uploaded
`redesign-launcher-visual-interface.zip`. The original handoff was based on
`a4906259e7791f2e55fda659766b216f97a7c468`; this integration is adapted to the
current branch and retains the newer mouse input and preview changes.

- The in-game ImGui adapter uses the supplied visual tokens and redesigned
  Home, HUD, Modules, Appearance and Settings panels. Controls operate on the
  existing module and visual settings; they are not a web page.
- The main menu receives scoped styling through its draw, although the closed
  core may still override individual styles or draw its own decoration.
- On narrow windows the toolbar selects a single panel; on wide windows all
  five and the real Module List preview are visible. Long panels scroll.
- Module List preview uses the real enabled modules. No fake metrics were added.
- The game model replay and hat capture remain intact. Automatic front camera
  and the separate far-block hat occlusion bug remain unresolved.
- Right Shift mouse integration is retained; the user confirmed cursor, menu
  interaction, game-input suppression and restoration in the preceding build.
- The new look has not yet been tested in Windows/Minecraft. Actual appearance,
  scaling and layout can differ, especially where the closed core owns drawing.

`repair/HUD_ALL_IN_ONE.txt` mirrors the relevant current source files. See
`repair/README.md` for the build command and runtime limitations.
