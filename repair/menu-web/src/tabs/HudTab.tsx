import { useEffect, useRef, useState, type CSSProperties } from "react";
import { MousePointer2 } from "lucide-react";
import { HUD_MODULES, MODULE_ACCENTS } from "../data/menu";
import { cn } from "../utils/cn";
import { Meter, ModuleRow, Panel, PanelHead, Section } from "../components/ui";
import {
  ArrowPixelIcon,
  CrosshairPixelIcon,
  ModulePixelIcon,
  ResolutionPixelIcon,
  ScanPixelIcon,
} from "../components/PixelIcons";
import { PixelChest } from "../components/PixelArt";

const WIDGET_POSITIONS: CSSProperties[] = [
  { left: 14, top: 14 },
  { right: 14, top: 14 },
  { right: 14, bottom: 14 },
  { left: 14, bottom: 14 },
  { left: 14, top: "46%" },
  { right: 14, top: "46%" },
];

function widgetValue(id: string, fps: number): string {
  switch (id) {
    case "fps":
      return `${fps > 0 ? fps : 60} FPS`;
    case "cps":
      return "7 CPS";
    case "clock":
      return "14:32";
    case "keys":
      return "W A D";
    case "watermark":
      return "GAYClient";
    case "coords":
      return "128 / 64 / -301";
    case "resolution":
      return "1920 × 1080";
    case "performance":
      return "CPU 18% / RAM 42%";
    case "moduleList":
      return "3 MODULES";
    default:
      return "-";
  }
}

type HudTabProps = {
  modules: Record<string, boolean>;
  onToggleModule: (id: string, next: boolean) => void;
  collapsed: Record<string, boolean>;
  onToggleSection: (key: string) => void;
  onOpenEditor: () => void;
  fps: number;
};

export function HudTab({ modules, onToggleModule, collapsed, onToggleSection, onOpenEditor, fps }: HudTabProps) {
  const [editing, setEditing] = useState(false);
  const timer = useRef<number | null>(null);
  const activeModules = HUD_MODULES.filter((item) => modules[item.id]);
  const activeCount = activeModules.length;
  const widgets = activeModules.slice(0, WIDGET_POSITIONS.length);

  useEffect(
    () => () => {
      if (timer.current !== null) window.clearTimeout(timer.current);
    },
    [],
  );

  const activateEditor = () => {
    setEditing(true);
    onOpenEditor();
    if (timer.current !== null) window.clearTimeout(timer.current);
    timer.current = window.setTimeout(() => setEditing(false), 2200);
  };

  return (
    <div className="tab-stack">
      <Panel>
        <PanelHead
          icon={<ResolutionPixelIcon size={20} />}
          title="HUD"
          tag="9 модулей"
          caption="Единое состояние с разделом «Модули»"
          side={<Meter value={activeCount} max={HUD_MODULES.length} />}
        />
        <div className="panel-sections">
          <Section
            id="hud.modules"
            title="ЭЛЕМЕНТЫ HUD"
            description="Активные элементы на экране"
            meta={
              <span className="section-count">
                <i className={activeCount > 0 ? "is-on" : ""} aria-hidden="true" />
                {activeCount} из {HUD_MODULES.length} включено
              </span>
            }
            open={!collapsed["hud.modules"]}
            onToggle={() => onToggleSection("hud.modules")}
          >
            <div className="module-list">
              {HUD_MODULES.map((item) => (
                <ModuleRow
                  key={item.id}
                  item={item}
                  checked={Boolean(modules[item.id])}
                  onToggle={(next) => onToggleModule(item.id, next)}
                />
              ))}
            </div>
          </Section>
        </div>
      </Panel>

      <Panel className={cn("editor-panel", editing && "panel-accent")}>
        <PanelHead
          icon={<CrosshairPixelIcon size={20} />}
          title="Редактор"
          tag="предпросмотр"
          caption="Расположение элементов настраивается перетаскиванием в HUD-редакторе"
        />
        <div className="editor-body">
          <div className={cn("hud-canvas", editing && "is-editing")}>
            <span className="hud-corner tl" aria-hidden="true" />
            <span className="hud-corner tr" aria-hidden="true" />
            <span className="hud-corner bl" aria-hidden="true" />
            <span className="hud-corner br" aria-hidden="true" />
            <span className="hud-scan" aria-hidden="true" />
            <span className="hud-caption" aria-hidden="true">
              HUD CANVAS / 1920 × 1080
            </span>
            <span className="hud-crosshair" aria-hidden="true">
              <i />
              <b />
            </span>

            {widgets.length > 0 ? (
              widgets.map((item, index) => (
                <span
                  className={cn("hud-widget", item.id === "watermark" && "hud-widget-brand")}
                  style={
                    {
                      ...WIDGET_POSITIONS[index],
                      "--chip-accent": MODULE_ACCENTS[item.id] ?? "var(--accent)",
                    } as CSSProperties
                  }
                  key={item.id}
                >
                  <span className="hud-widget-icon" aria-hidden="true">
                    <ModulePixelIcon id={item.id} size={9} />
                  </span>
                  <span className="hud-widget-value">{widgetValue(item.id, fps)}</span>
                </span>
              ))
            ) : (
              <span className="hud-empty">
                <PixelChest />
                <span>Все HUD-модули выключены</span>
              </span>
            )}

            {editing ? (
              <span className="hud-editing-badge">
                <ScanPixelIcon size={11} />
                HUD-редактор открыт
              </span>
            ) : null}
          </div>

          <div className="editor-side">
            <span className="editor-badge">
              <ArrowPixelIcon size={12} />
              HUD EDITOR
            </span>
            <p className="editor-lead">Расставьте элементы по сетке и закрепите их у краёв экрана.</p>
            <p className="editor-hint">Изменения положения сохраняются локально.</p>
            <button type="button" className="btn btn-primary editor-cta" onClick={activateEditor}>
              <MousePointer2 size={15} strokeWidth={2.2} aria-hidden="true" />
              Открыть редактор
            </button>
          </div>
        </div>
      </Panel>
    </div>
  );
}
