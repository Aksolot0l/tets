#include "../src/hud_visual_kit.h"
#include <cassert>
#include <cstring>
int main(){
 using namespace hud_theme; using namespace hud_visual;
 for (int i=0;i<kPaletteCount;i++) {
  const auto t=tokens(i,.94f,10.f,1.f);

  const auto primary=button(t,ButtonTone::Primary);
  const auto secondary=button(t,ButtonTone::Secondary);
  const auto ghost=button(t,ButtonTone::Ghost);
  const auto danger=button(t,ButtonTone::Danger);
  assert(primary.height==26.f && primary.paddingX==10.f);
  assert(primary.rounding<=t.rounding);                 // buttons are tighter than cards
  assert(primary.border.a>secondary.border.a);          // primary is stronger
  assert(ghost.text.a<=primary.text.a);                 // ghost recedes
  assert(danger.text.r>danger.text.g);                  // destructive is red

  const auto tg=toggle(t);
  assert(tg.width>tg.height && tg.knob<tg.height && tg.inset>0.f);
  assert(tg.trackOn.a>tg.trackOff.a);                   // on-state reads stronger

  const auto sl=slider(t);
  assert(sl.height<sl.grab && sl.grab>0.f);
  assert(sl.fill.a>0.f && sl.grabColor.a>0.f);

  const auto flat=card(t);
  const auto nested=card(t,true);
  assert(flat.rounding==t.rounding && flat.borderSize==1.f);
  assert(nested.pad>0.f && flat.pad>0.f);
  assert(flat.accentBar.a>0.f);

  const auto sec=section(t);
  assert(sec.barWidth==2.f && sec.barHeight>sec.barWidth);
  assert(sec.gapBefore>0.f);

  const auto hotPill=pill(t,true);
  const auto coolPill=pill(t,false);
  assert(hotPill.text.a>coolPill.text.a);
  assert(hotPill.rounding>=999.f);

  const auto ok=status(t,true);
  const auto warn=status(t,false,true);
  assert(ok.dot.a>0.f && warn.dot.r>warn.dot.g);

  const auto sw=swatch(t);
  assert(sw.size>sw.rounding && sw.gap>0.f);

  assert(readableOn(t.window,t.text));
  assert(contrastFloor()>=4.5f);

  const auto pc=paletteCard(i);
  assert(pc.dot.a==1.f && pc.height>=20.f);
 }

 // Geometry: five columns plus the preview gutter stay inside the viewport.
 assert(readableOn(rgba(0,0,0),rgba(255,255,255)));
 assert(!readableOn(rgba(255,255,255),rgba(255,255,255)));
 assert(!readableOn(rgba(0,0,0),rgba(255,255,255,.05f)));
 const auto wide=layout(1920.f);
 assert(wide.columns==5 && !wide.stacked);
 assert(wide.columnWidth<=editorMetrics.panelWidth);
 const auto narrow=layout(900.f);
 assert(narrow.stacked);
 assert(panelWidthFor(1280.f)>=150.f);
 assert(previewX(1920.f)>panelX(4));
 assert(editorOpacity(.1f)>=.9f);   // chrome never fades out
 assert(readableOpacity(0.f)>=.18f);
 for (float w=1024.f; w<=3840.f; w+=137.f) {
  assert(panelWidthFor(w)>0.f);
  assert(panelWidthFor(w)<=editorMetrics.panelWidth);
  if (columnsFit(w)) {
   assert(panelX(4,w)+panelWidthFor(w)+editorMetrics.previewGap <= previewX(w));
   for (int i=1;i<5;++i) assert(panelX(i-1,w)+panelWidthFor(w)+editorMetrics.panelGap<=panelX(i,w)+.01f);
  } else {
   assert(compactPanelWidth(w)<=w-editorMetrics.panelLeft*2.f);
  }
  for (int i=0;i<5;++i) assert(panelHeight(i,w,720.f)+panelTop(w)<=720.f);
 }
 assert(!columnsFit(1024.f));
 assert(columnsFit(1280.f));
 assert(compactPanelWidth(320.f)<=288.f);
 assert(previewX(600.f)>=editorMetrics.panelLeft+compactPanelWidth(600.f)+editorMetrics.previewGap);
 assert(panelHeight(2,320.f,200.f)+panelTop(320.f)<=200.f);
}
