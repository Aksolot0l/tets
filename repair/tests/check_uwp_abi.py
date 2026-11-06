"""Check the manually declared WinRT ABI and legacy callback against known metadata."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
s = (ROOT/'repair/src/uwp_extra.cpp').read_text()
core = s[s.index('struct CoreWindow :'):s.index('struct CoreWindowStatics :')]
# The following pair ordering matches Windows.UI.Core.ICoreWindow; declaring a
# missing slot would call an unrelated COM method and could crash the game.
expected = ['Activated','AutomationEvent','Character','Closed','InputEnabled','KeyDown','KeyUp',
            'CaptureLost','PointerEntered','PointerExited','PointerMoved','PointerPressed',
            'PointerReleased','TouchHitTesting']
assert re.findall(r'UNUSED_PAIR\((\w+)\)', core)[1:] == expected
props = s[s.index('struct PointerProperties :'):s.index('struct PointerHandler :')]
assert re.findall(r'UNUSED\((\d+)\)', props) == [str(i) for i in range(13)]
assert props.index('get_MouseWheelDelta') < props.index('get_IsHorizontalMouseWheel')
print('WinRT interface declaration order: OK (requires Windows runtime testing)')
