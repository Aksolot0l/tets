"""Static checks for the VISUAL-ONLY HUD redesign. Cannot replace an in-game check.

The rule is simple: the patch may change colours, spacing, radii and panel
layout, but it must not add settings writes, game addresses, render hooks or
fake metrics, and it must keep every existing control and experimental effect.
"""
from pathlib import Path
import re

r = Path(__file__).resolve().parents[1]
u = (r / 'src/compat.cpp').read_text(encoding='utf-8')
theme = (r / 'src/hud_theme.h').read_text(encoding='utf-8')
kit = (r / 'src/hud_visual_kit.h').read_text(encoding='utf-8')

# 1. Visual layer exists and is used by the adapter.
assert '#include "hud_theme.h"' in u and '#include "hud_visual_kit.h"' in u
assert 'namespace hud_visual' in kit and 'namespace hud_theme' in theme
for helper in ('VisualStyleScope', 'sectionTitle', 'visualHint', 'fullWidthSlider',
               'visualTokens', 'editorVisible', 'editorColumn'):
    assert helper in u, helper
assert 'Tokens tokens(' in theme and 'EditorMetrics' in theme
assert 'ButtonSpec button(' in kit and 'ToggleSpec toggle(' in kit
assert 'SliderSpec slider(' in kit and 'CardSpec card(' in kit
assert 'LayoutSpec layout(' in kit and 'readableOn(' in kit
assert 'readableOpacity' in theme and 'editorOpacity' in theme

# 2. Five editor columns and the neighbouring preview, from the mockup.
for panel in ('Home##gc', 'HUD##gc', 'Modules##gc', 'Appearance##gc', 'Settings##gc'):
    assert panel in u, panel
assert 'Preview##gc_caption' in u
assert 'drawModuleHud(runtime,true)' in u
assert 'editorVisible[5]' in u
assert 'hud_theme::panelX' in u and 'hud_theme::previewX' in u

# 3. Every pre-existing control/behaviour is still present.
for preserved in (
        'hud_theme::palette(i).name', 'hudPalette=i', 'hudColor[0]=c.r',
        'Card rounding', 'Widget background opacity', 'HUD text color',
        'Module List HUD', 'Module list scale', 'Module list opacity',
        'Module list X', 'Module list Y', 'Style resets after game exit.',
        'Notifications may remain visible.',
        'China Hat', 'Trail', 'Settings##open_hat', 'Settings##open_trail',
        'Options', 'Preview unavailable; inline settings:',
        'Game model replay (test)', 'Old overlay (see-through)',
        'Vertex gradient (test)', 'Hat at player pass (test)',
        'Waiting for player; sample avatar', 'Sample model; game shader may differ.'):
    assert preserved in u, preserved
for entry in ('ag::ModuleId::Coordinates', 'setModule(runtime,entry.id,enabled)',
              'settings->IsEnabled(entry.id)'):
    assert entry in u, entry

# 4. No behavioural additions: no new writes to game settings, no addresses.
for forbidden in (
        'settings->opacity=', 'settings->theme=', 'settings->accent=',
        'settings->background=', 'settings->text=', 'settings->rounding=',
        'settings->brightness=', 'settings->uiScale=', 'settings->hudScale=',
        'settings->keystrokeScale=', 'settings->animationSpeed=',
        'settings->panelVisible', 'WriteProcessMemory('):
    assert forbidden not in u, forbidden
# The adapter already reads the existing read-only coordinate path; no new hook.
assert 'worldEntitiesRva' not in u
assert 'OMSetDepthStencilState' not in u

# 5. Scoped style survives through actual widgets, and restores on return.
# The legacy click GUI keeps its scoped style whenever it is drawn; the
# embedded new menu can replace it (see check_menu_rml.py) without removing
# this path for the not-replaced case and any embed failure fallback.
assert 'if (menuOpen && !replaced) { VisualStyleScope style; realDrawGui(runtime); }' in u
assert '~VisualStyleScope()' in u and 'guiPopColor(pushedColors)' in u and 'guiPopVar(pushedVars)' in u
assert 'insideLegacyHud || !hud_theme::card(name)' in u
assert 'hud_theme::background(widgetOpacity*legacyOpacity,hudPalette)' in u
assert 'SetMenuMouseOpen(menuOpen)' in u and 'SetMenuMouseOpen(false)' in u
assert 'hud_theme::columnsFit(editorWidth())' in u
assert '(wide || editorSelected==0)' in u and 'editorSelected=i; editorVisible[i]=true' in u
assert 'hud_theme::panelX(index,width)' in u and 'hud_theme::panelHeight(index,width,height)' in u
assert 'ImGuiCond_Always' in u and 'guiPushTextWrap(0.f)' in u
for sym in ('_ZN5ImGui15PushTextWrapPosEf','_ZN5ImGui14PopTextWrapPosEv'):
    assert sym in u and sym.encode() in (r.parent/'GAYClientInternal.dll').read_bytes()

# 6. No mock values or invented widgets.
for fake in ('FPS: 144', 'CPS: 0', '12:34', 'demo avatar', 'AOGIRI GREEN\\n',
             'inventory', 'Ping: '):
    assert fake not in u, f'unexpected mock value: {fake}'

# 7. Documentation matches this visual-only scope.
readme = (r / 'README.md').read_text(encoding='utf-8')
assert 'HUD redesign — visual layer' in readme
doc = (r / 'HUD_ALL_IN_ONE.txt').read_text(encoding='utf-8')
assert 'UPDATE: HUD redesign — visual layer' in doc
assert 'hud_visual_kit.h' in doc

print('HUD visual-only checks OK; Windows/Minecraft verification required')
