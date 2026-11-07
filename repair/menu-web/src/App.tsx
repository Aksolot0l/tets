import { useCallback, useEffect, useMemo } from "react";
import { RotateCcw } from "lucide-react";
import {
  ACCENT_THEMES,
  DEFAULT_COLLAPSED,
  DEFAULT_GRAPHICS_TOGGLES,
  DEFAULT_GRAPHICS_VALUES,
  DEFAULT_MODULES,
  NAV_ITEMS,
  type TabId,
} from "./data/menu";
import {
  useFps,
  usePersistedState,
  usePrefersReducedMotion,
  useToasts,
} from "./hooks/useClientState";
import { ClosedBackdrop } from "./components/AbstractArt";
import { PixelLogo } from "./components/PixelArt";
import { TitleBar } from "./components/TitleBar";
import { NavRail } from "./components/NavRail";
import { TestTools, type TestToolsPosition } from "./components/TestTools";
import { ToastHost } from "./components/ToastHost";
import { ModulesTab } from "./tabs/ModulesTab";
import { GraphicsTab } from "./tabs/GraphicsTab";
import { HudTab } from "./tabs/HudTab";
import { InfoTab } from "./tabs/InfoTab";

function AppBackground() {
  return (
    <div className="app-bg" aria-hidden="true">
      <span className="aurora aurora-a" />
      <span className="aurora aurora-b" />
      <span className="aurora aurora-c" />
    </div>
  );
}

function useViewportScale() {
  useEffect(() => {
    const root = document.documentElement;
    const update = () => {
      const width = window.innerWidth;
      const height = window.innerHeight;
      const fits = width >= 1440 && height >= 800;
      const scale = fits ? Math.min((width - 470) / 1140, (height - 24) / 716, 1.15) : 1;
      root.style.setProperty("--shell-scale", String(Math.max(1, Number(scale.toFixed(3)))));
    };
    update();
    window.addEventListener("resize", update);
    return () => {
      window.removeEventListener("resize", update);
      root.style.removeProperty("--shell-scale");
    };
  }, []);
}

function ClosedScreen({ onOpen }: { onOpen: () => void }) {
  return (
    <div className="closed-screen">
      <ClosedBackdrop />
      <div className="closed-card">
        <div className="closed-mark" aria-hidden="true">
          <PixelLogo />
        </div>
        <span className="closed-eyebrow">GAYCLIENT / GAME MENU</span>
        <h1>Меню закрыто</h1>
        <p className="closed-copy">
          Интерфейс настроек остановлен. Все изменения сохранены в локальном состоянии и восстановятся после открытия.
        </p>
        <div className="closed-meta" aria-hidden="true">
          <span>
            <kbd>ESC</kbd> закрыть
          </span>
          <span>BEDROCK 1.1.5</span>
          <span>v1.2</span>
        </div>
        <button type="button" className="btn btn-primary closed-cta" onClick={onOpen} autoFocus>
          <RotateCcw size={15} strokeWidth={2.2} aria-hidden="true" />
          Открыть снова
        </button>
        <span className="closed-note">Состояние переживает перезагрузку страницы</span>
      </div>
      <div className="closed-footer">GAYClient / stand of settings interface / no connections</div>
    </div>
  );
}

export default function App() {
  const fps = useFps();
  const prefersReducedMotion = usePrefersReducedMotion();
  const { toasts, showToast, dismissToast } = useToasts();

  const [themeId, setThemeId] = usePersistedState<string>("theme", "emerald");
  const [motion, setMotion] = usePersistedState<boolean>("motion", !prefersReducedMotion);
  const [activeTab, setActiveTab] = usePersistedState<TabId>("tab", "modules");
  const [modules, setModules] = usePersistedState<Record<string, boolean>>("modules", DEFAULT_MODULES);
  const [graphicsValues, setGraphicsValues] = usePersistedState<Record<string, number>>(
    "graphicsValues",
    DEFAULT_GRAPHICS_VALUES,
  );
  const [graphicsToggles, setGraphicsToggles] = usePersistedState<Record<string, boolean>>(
    "graphicsToggles",
    DEFAULT_GRAPHICS_TOGGLES,
  );
  const [collapsed, setCollapsed] = usePersistedState<Record<string, boolean>>("collapsed", DEFAULT_COLLAPSED);
  const initialTestPosition = useMemo<TestToolsPosition>(() => {
    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    if (viewportWidth >= 1900) return { x: 24, y: 240 };
    if (viewportWidth >= 1200) return { x: Math.max(12, viewportWidth - 372), y: Math.max(120, viewportHeight - 64) };
    return { x: 12, y: 140 };
  }, []);
  const [testPosition, setTestPosition] = usePersistedState<TestToolsPosition>(
    "testToolsPosition",
    initialTestPosition,
  );
  const [testCollapsed, setTestCollapsed] = usePersistedState<boolean>(
    "testToolsCollapsed",
    window.innerWidth < 1900,
  );
  const [testToolsVisible, setTestToolsVisible] = usePersistedState<boolean>(
    "testToolsVisible",
    window.innerWidth >= 1200,
  );
  const [menuOpen, setMenuOpen] = usePersistedState<boolean>("menuOpen", true);

  useViewportScale();

  const currentTab: TabId = NAV_ITEMS.some((item) => item.id === activeTab) ? activeTab : "modules";

  useEffect(() => {
    const theme = ACCENT_THEMES.find((item) => item.id === themeId) ?? ACCENT_THEMES[0];
    const root = document.documentElement;
    root.style.setProperty("--accent", theme.accent);
    root.style.setProperty("--accent-strong", theme.strong);
    root.style.setProperty("--accent-deep", theme.deep);
    root.style.setProperty("--accent-rgb", theme.rgb);
    root.dataset.theme = theme.id;
  }, [themeId]);

  useEffect(() => {
    document.documentElement.dataset.motion = motion ? "on" : "off";
  }, [motion]);

  const toggleModule = useCallback(
    (id: string, next: boolean) => {
      setModules((previous) => ({ ...previous, [id]: next }));
    },
    [setModules],
  );

  const updateGraphicsValue = useCallback(
    (id: string, value: number) => {
      setGraphicsValues((previous) => ({ ...previous, [id]: value }));
    },
    [setGraphicsValues],
  );

  const toggleGraphics = useCallback(
    (id: string, next: boolean) => {
      setGraphicsToggles((previous) => ({ ...previous, [id]: next }));
    },
    [setGraphicsToggles],
  );

  const toggleSection = useCallback(
    (key: string) => {
      setCollapsed((previous) => ({ ...previous, [key]: !previous[key] }));
    },
    [setCollapsed],
  );

  const openHudEditor = useCallback(() => {
    setActiveTab("hud");
    showToast("Открыт HUD-редактор", "info");
  }, [setActiveTab, showToast]);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (!menuOpen) return;
      if (event.key === "Escape") {
        event.preventDefault();
        setMenuOpen(false);
        return;
      }
      if ((event.ctrlKey || event.metaKey) && event.key === "Enter") {
        event.preventDefault();
        showToast("Настройки применены");
        return;
      }
      const target = event.target as HTMLElement | null;
      const isTextInput =
        target instanceof HTMLInputElement &&
        !["range", "checkbox", "radio", "button"].includes(target.type);
      const isEditable =
        target !== null &&
        (isTextInput ||
          target.tagName === "TEXTAREA" ||
          target.tagName === "SELECT" ||
          target.isContentEditable);
      if (isEditable || event.ctrlKey || event.metaKey || event.altKey) return;
      const matchedTab = NAV_ITEMS.find((item) => item.hotkey === event.key);
      if (matchedTab) {
        event.preventDefault();
        setActiveTab(matchedTab.id);
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [menuOpen, setActiveTab, setMenuOpen, showToast]);

  return (
    <div className="app">
      <AppBackground />

      {menuOpen ? (
        <div className="shell" role="dialog" aria-label="GAYClient - внутриигровое меню настроек">
          <TitleBar fps={fps} onClose={() => setMenuOpen(false)} />

          <div className="shell-body">
            <NavRail
              active={currentTab}
              onChange={setActiveTab}
              onOpenHud={openHudEditor}
              toolsVisible={testToolsVisible}
              onToggleTools={() => setTestToolsVisible((previous) => !previous)}
            />
            <main className="content" aria-label="Настройки GAYClient">
              <div className="tab-pane" key={currentTab}>
                {currentTab === "modules" ? (
                  <ModulesTab
                    modules={modules}
                    onToggleModule={toggleModule}
                    collapsed={collapsed}
                    onToggleSection={toggleSection}
                  />
                ) : null}
                {currentTab === "graphics" ? (
                  <GraphicsTab
                    values={graphicsValues}
                    toggles={graphicsToggles}
                    onValueChange={updateGraphicsValue}
                    onToggleChange={toggleGraphics}
                    collapsed={collapsed}
                    onToggleSection={toggleSection}
                  />
                ) : null}
                {currentTab === "hud" ? (
                  <HudTab
                    modules={modules}
                    onToggleModule={toggleModule}
                    collapsed={collapsed}
                    onToggleSection={toggleSection}
                    onOpenEditor={openHudEditor}
                    fps={fps}
                  />
                ) : null}
                {currentTab === "info" ? (
                  <InfoTab
                    themeId={themeId}
                    onThemeChange={setThemeId}
                    motion={motion}
                    onMotionChange={setMotion}
                  />
                ) : null}
              </div>
            </main>
          </div>
        </div>
      ) : (
        <ClosedScreen onOpen={() => setMenuOpen(true)} />
      )}

      {menuOpen && testToolsVisible ? (
        <TestTools
          values={graphicsValues}
          onChange={updateGraphicsValue}
          position={testPosition}
          onPositionChange={setTestPosition}
          collapsed={testCollapsed}
          onCollapsedChange={setTestCollapsed}
        />
      ) : null}

      <ToastHost toasts={toasts} onDismiss={dismissToast} />

      <div className="sr-only" aria-live="polite">
        {activeTab === "modules"
          ? "Открыта вкладка Модули"
          : activeTab === "graphics"
            ? "Открыта вкладка Графика"
            : activeTab === "hud"
              ? "Открыта вкладка HUD"
              : "Открыта вкладка Инфо"}
      </div>
    </div>
  );
}
