"""Static wiring checks for the katana replacement; cannot replace Windows/game testing."""
from pathlib import Path
import importlib.util, re
r=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('convert_katana',r/'tools/convert_katana.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
asset=(r/'src/katana_asset.h').read_text()
embedded=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})',asset))
assert embedded==c.convert((r/'assets/models/cyberpunk-katana.glb').read_bytes())
n=(r/'src/china_hat_native.cpp').read_text(); g=(r/'src/visual_gpu_depth.h').read_text(); ui=(r/'src/compat.cpp').read_text()
settings=(r/'src/visual_settings.h').read_text()
pose=(r/'src/katana_pose.h').read_text(); rotation=(r/'src/katana_rotation.h').read_text()

# The replacement draws through the proven guarded tessellator path, never
# through a raw context Draw of its own.
assert 'submitKatanaBatch(t,opacity,ctx)' in n and 'renderKatana(local);' in n
assert n.count('renderKatana(local);')==2  # strict world pass + legacy screen pass
assert 'visual_gpu_depth::inVisualDraw=true' in n and 'tessDraw(tess,reinterpret_cast<void*>(base+contextRva),0)' in n
assert 'katana_geometry::triangles(mesh,pose,true' in n
assert 'D3D11_QUERY_OCCLUSION' in n and 'GetData(slots[i],&samples,sizeof(samples),1' in n
assert 'samples>=minSamples' in n
# Suppression happens only while the replacement is occlusion-verified visible.
assert 'bool katanaVisibleRecently(ULONGLONG now)' in n
assert 'if (!katanaVisibleRecently(GetTickCount64()))' in n
assert 'visual_gpu_depth::katanaSuppress=ctx;' in n and 'visual_gpu_depth::katanaSuppress=nullptr;' in n
assert 'if (ctx && ctx==katanaSuppress) return;' in g
# No raw self-issued draw and no unconditional suppression of the original.
assert 'submitKatana(ID3D11DeviceContext' not in n
assert 'katanaObserve' not in n and 'katanaDraws' not in n
assert 'prepareKatanaStream' not in n and 'katana_stream_policy' not in n

# Item identity: first person via the outer held hook, third person via the
# streaming observer or the texture fingerprint learned from a verified draw.
assert 'returnAddress!=base+0x5a1d2d' in n and 'returnAddress!=base+0x59fd1e' not in n
assert 'rva==0x59fd1e && katanaScope' in n
assert 'rva==0x56cd8e && katanaLocalPlayerPass' in n
assert 'katanaSwordTexture' in n and 'context)+0x90' in n
assert 'diamond_sword_candidate::matches' in n
assert 'return originalHeldGeometry(self,a2,a3,stack,a5);' in n  # observer never skips
assert 'local!=reinterpret_cast<std::uintptr_t>(actor)' in n

# Rotation: validated read-only sources with a movement-based sign check.
assert 'readRotation(base,clientInstance.load' in n
assert 'chainGlobalA=0x1921DF8' in rotation and 'chainGlobalB=0x1921888' in rotation
assert 'yawOffset=0xB8' in rotation and 'pitchOffset=0xBC' in rotation
# The adapter-proven player slot is a rotation source that works without the
# published globals, unwrapped yaws are normalised, and the raw trail is
# exposed for diagnostics.
assert 'playerSlot=0x60' in rotation and 'rotationSlot=0x58' in rotation
assert 'wrapYaw' in rotation and 'maxUnwrappedYaw' in rotation
assert 'struct Diagnostics' in rotation and 'void snapshot(' in rotation
assert 'KatanaDebugSnapshot' in n and 'KatanaDebugSnapshot' in open(r/'src/china_hat_native.h').read()
assert 'drawKatanaDiagnostics();' in ui
assert 'katanaTpCallsInPass' in n and 'katanaTpLastTexture' in n
assert 'katanaYawVotes.sample' in n and 'YawSignVote' in rotation
assert 'horizontalForward' in pose and 'gripPivot' in pose

# Hook install path and safety gates.
assert 'validateGame(true)' in n and 'katanaCodeMatches()' in n and 'visual_hooks::install(hooks,3' in n
assert 'hookState.load(std::memory_order_acquire)!=2' in n
assert n.count('hat_validation::mayBePatched(')==2
assert 'katanaMesh().vertices()!=4572 || katanaMesh().indices()!=7866' in n
# The old "paused: unverified" guard is gone; the real gate is occlusion.
set_enable=n[n.index('void SetKatanaEnabled(bool value)'):]
assert 'katanaReplacementVerified' not in set_enable
assert 'Katana paused: unverified' not in ui
assert 'if (!enableVisual())' in set_enable

# UI: statuses, evidence letters and tunable parameters.
for message in ('game hooks unavailable','GPU hooks unavailable','game check failed','item code changed',
                'model invalid','hook install failed','local player not matched','sword not matched',
                'sword candidate','third person not confirmed yet','rotation unreadable; original kept',
                'visible; replacing sword','drawing (world pass)','waiting for held diamond sword'):
    assert message in ui,message
assert 'Diamond sword: katana (test)' in ui and 'SetKatanaEnabled(false);' in ui
assert 'Settings##open_katana' in ui and 'drawKatanaSettings()' in ui
assert 'Seen F:%u V:%u S:%u C:%u' in ui and 'R:%u X:%u D:%u O:%u P:%u' in ui
for name in ('KatanaScale','KatanaTilt','KatanaSide','KatanaHeight','KatanaForward','KatanaOpacity',
             'KatanaFirstSide','KatanaFirstForward'):
    assert name in settings and ('VisualParam::'+name) in ui,name
# Only the local player's own renderer scope can suppress third person.
assert 'katanaLocalPlayerPass' in n and 'katanaThirdSwordSeen=false;' in n
# The world pass must run for the katana even when no other effect is on.
assert '|| katanaEnabled.load(std::memory_order_acquire)' in n
print('Embedded model and world-pass katana wiring OK; no Windows/game validation')
