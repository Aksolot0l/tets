import { Info, LayoutGrid, MonitorCog, SlidersHorizontal, Wrench, type LucideIcon } from "lucide-react";
import { SparklePixelIcon } from "./PixelIcons";
import { NAV_ITEMS, type TabId } from "../data/menu";

const ICONS: Record<"modules" | "graphics" | "hud" | "info", LucideIcon> = {
  modules: LayoutGrid,
  graphics: SlidersHorizontal,
  hud: MonitorCog,
  info: Info,
};

type NavRailProps = {
  active: TabId;
  onChange: (tab: TabId) => void;
  onOpenHud: () => void;
  toolsVisible: boolean;
  onToggleTools: () => void;
};

export function NavRail({ active, onChange, onOpenHud, toolsVisible, onToggleTools }: NavRailProps) {
  return (
    <nav className="rail" aria-label="Разделы меню">
      <div className="rail-heading">
        <span>НАВИГАЦИЯ</span>
        <span className="rail-heading-hint">1-4</span>
      </div>

      <div className="rail-nav">
        {NAV_ITEMS.map((item) => {
          const Icon = ICONS[item.icon];
          const isActive = item.id === active;
          return (
            <button
              key={item.id}
              type="button"
              className="nav-item"
              aria-current={isActive ? "page" : undefined}
              onClick={() => onChange(item.id)}
            >
              <Icon size={17} strokeWidth={2} aria-hidden="true" />
              <span>{item.label}</span>
              <kbd className="nav-hotkey" aria-hidden="true">
                {item.hotkey}
              </kbd>
            </button>
          );
        })}
        <button
          type="button"
          className={toolsVisible ? "nav-item nav-mobile-tools is-on" : "nav-item nav-mobile-tools"}
          onClick={onToggleTools}
          aria-pressed={toolsVisible}
          aria-label={toolsVisible ? "Скрыть Test Tools" : "Показать Test Tools"}
        >
          <Wrench size={17} strokeWidth={2} aria-hidden="true" />
          <span>Tools</span>
          <i className="tools-dot" aria-hidden="true" />
        </button>
      </div>

      <div className="rail-footer">
        <button
          type="button"
          className={toolsVisible ? "rail-tools is-on" : "rail-tools"}
          onClick={onToggleTools}
          aria-pressed={toolsVisible}
        >
          <Wrench size={13} strokeWidth={2.2} aria-hidden="true" />
          <span>TEST TOOLS</span>
          <i aria-hidden="true" />
        </button>
        <div className="rail-tip">
          <div className="rail-tip-top">
            <span className="rail-tip-icon" aria-hidden="true">
              <SparklePixelIcon size={12} />
            </span>
            <span>HUD EDITOR</span>
          </div>
          <p>Расстановка элементов</p>
          <span className="rail-tip-caption">Позиции и размеры - в одном месте</span>
        </div>
        <button type="button" className="btn btn-primary rail-cta" onClick={onOpenHud}>
          Настроить HUD
        </button>
        <div className="rail-local">
          <i aria-hidden="true" />
          <span>LOCAL STATE</span>
          <span>v1.2</span>
        </div>
      </div>
    </nav>
  );
}
