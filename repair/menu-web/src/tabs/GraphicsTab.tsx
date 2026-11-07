import { Fragment } from "react";
import { InfoPixelIcon, SlidersPixelIcon } from "../components/PixelIcons";import { ALL_SLIDERS, GRAPHICS_GROUPS, groupColor } from "../data/menu";
import { cn } from "../utils/cn";
import { ColorSwatch, Panel, PanelHead, Section, SliderRow, ToggleTile } from "../components/ui";
import { EffectPreview } from "../components/EffectPreviews";

type GraphicsTabProps = {
  values: Record<string, number>;
  toggles: Record<string, boolean>;
  onValueChange: (id: string, value: number) => void;
  onToggleChange: (id: string, next: boolean) => void;
  collapsed: Record<string, boolean>;
  onToggleSection: (key: string) => void;
};

export function GraphicsTab({
  values,
  toggles,
  onValueChange,
  onToggleChange,
  collapsed,
  onToggleSection,
}: GraphicsTabProps) {
  return (
    <div className="tab-stack">
      <Panel>
        <PanelHead
          icon={<SlidersPixelIcon size={20} />}
          title="Графика"
          tag="живые настройки"
          caption="Параметры эффектов применяются сразу"
          side={
            <span className="live-pill" title="Значения обновляются во время перетаскивания">
              <span className="live-dot" aria-hidden="true" />
              LIVE
            </span>
          }
        />

        <div className="panel-sections">
          {GRAPHICS_GROUPS.map((group) => {
            const effectColor = groupColor(values, group);
            const enabledToggle = group.toggles[0];
            const enabled = enabledToggle ? Boolean(toggles[enabledToggle.id]) : true;
            const sectionKey = `graphics.${group.id}`;
            const hasColor = group.id !== "katana";

            return (
              <Section
                key={group.id}
                id={sectionKey}
                title={group.title}
                description={group.description}
                meta={`${group.sliders.length} парам.`}
                side={
                  <span className="gfx-side">
                    {hasColor ? <ColorSwatch color={effectColor} title={`Цвет эффекта: ${effectColor}`} /> : null}
                    {group.toggles.length > 0 ? (
                      <span className={cn("gfx-state", enabled && "is-on")}>{enabled ? "ВКЛ" : "ВЫКЛ"}</span>
                    ) : (
                      <span className="gfx-state is-neutral">3D</span>
                    )}
                  </span>
                }
                preview={<EffectPreview id={group.id} values={values} />}
                open={!collapsed[sectionKey]}
                onToggle={() => onToggleSection(sectionKey)}
              >
                {group.toggles.length > 0 ? (
                  <div className="toggle-grid">
                    {group.toggles.map((toggle) => (
                      <ToggleTile
                        key={toggle.id}
                        label={toggle.label}
                        checked={Boolean(toggles[toggle.id])}
                        onChange={(next) => onToggleChange(toggle.id, next)}
                      />
                    ))}
                  </div>
                ) : null}

                <div className="slider-list">
                  {group.sliders.map((slider) => {
                    const isColorStart = slider.id.endsWith(".colorR");
                    const isRainbow = slider.id.endsWith(".rainbowSpeed");
                    return (
                      <Fragment key={slider.id}>
                        {isColorStart && hasColor ? (
                          <div className="color-strip">
                            <ColorSwatch color={effectColor} />
                            <span className="color-strip-label">Цвет эффекта</span>
                            <code className="color-strip-code">{effectColor}</code>
                          </div>
                        ) : null}
                        <SliderRow
                          slider={slider}
                          value={values[slider.id] ?? slider.defaultValue}
                          onChange={(next) => onValueChange(slider.id, next)}
                          swatchColor={isRainbow ? effectColor : undefined}
                        />
                      </Fragment>
                    );
                  })}
                </div>
              </Section>
            );
          })}
        </div>

        <p className="panel-note">
          <InfoPixelIcon size={11} />
          {GRAPHICS_GROUPS.length} секций / {ALL_SLIDERS.length} параметров
          <span className="panel-note-spacer" />
          Сохраняется локально
        </p>
      </Panel>
    </div>
  );
}
