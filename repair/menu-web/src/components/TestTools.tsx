import { useCallback, useEffect, useId, useRef, useState, type Dispatch, type PointerEvent, type SetStateAction } from "react";
import { ChevronDown } from "lucide-react";
import { GripPixelIcon } from "./PixelIcons";
import { findSlider, formatSliderValue, TEST_TOOL_GROUPS } from "../data/menu";
import { cn } from "../utils/cn";
import { RangeInput } from "./ui";

export type TestToolsPosition = { x: number; y: number };

type TestToolsProps = {
  values: Record<string, number>;
  onChange: (id: string, value: number) => void;
  position: TestToolsPosition;
  onPositionChange: Dispatch<SetStateAction<TestToolsPosition>>;
  collapsed: boolean;
  onCollapsedChange: (collapsed: boolean) => void;
};

const PANEL_MARGIN = 8;

export function TestTools({
  values,
  onChange,
  position,
  onPositionChange,
  collapsed,
  onCollapsedChange,
}: TestToolsProps) {
  const panelRef = useRef<HTMLElement>(null);
  const dragRef = useRef<{ pointerId: number; offsetX: number; offsetY: number } | null>(null);
  const [dragging, setDragging] = useState(false);
  const idPrefix = useId();

  const clampPosition = useCallback((x: number, y: number): TestToolsPosition => {
    const panel = panelRef.current;
    const width = panel?.offsetWidth ?? 360;
    const height = panel?.offsetHeight ?? 540;
    const maxX = Math.max(PANEL_MARGIN, window.innerWidth - width - PANEL_MARGIN);
    const maxY = Math.max(PANEL_MARGIN, window.innerHeight - height - PANEL_MARGIN);
    return {
      x: Math.min(Math.max(PANEL_MARGIN, x), maxX),
      y: Math.min(Math.max(PANEL_MARGIN, y), maxY),
    };
  }, []);

  useEffect(() => {
    const applyClamp = () => {
      onPositionChange((previous) => {
        const next = clampPosition(previous.x, previous.y);
        return next.x === previous.x && next.y === previous.y ? previous : next;
      });
    };

    const frame = window.requestAnimationFrame(applyClamp);
    window.addEventListener("resize", applyClamp);
    return () => {
      window.cancelAnimationFrame(frame);
      window.removeEventListener("resize", applyClamp);
    };
  }, [clampPosition, collapsed, onPositionChange]);

  const handlePointerDown = (event: PointerEvent<HTMLDivElement>) => {
    if (event.button !== 0) return;
    const target = event.target as HTMLElement;
    if (target.closest("button")) return;
    if (!panelRef.current) return;
    event.preventDefault();
    event.currentTarget.setPointerCapture(event.pointerId);
    dragRef.current = {
      pointerId: event.pointerId,
      offsetX: event.clientX - position.x,
      offsetY: event.clientY - position.y,
    };
    setDragging(true);
  };

  const handlePointerMove = (event: PointerEvent<HTMLDivElement>) => {
    const drag = dragRef.current;
    if (!drag || drag.pointerId !== event.pointerId) return;
    onPositionChange(clampPosition(event.clientX - drag.offsetX, event.clientY - drag.offsetY));
  };

  const endDrag = (event: PointerEvent<HTMLDivElement>) => {
    const drag = dragRef.current;
    if (!drag || drag.pointerId !== event.pointerId) return;
    if (event.currentTarget.hasPointerCapture(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }
    dragRef.current = null;
    setDragging(false);
  };

  return (
    <aside
      ref={panelRef}
      className={cn("test-tools", dragging && "is-dragging", collapsed && "is-collapsed")}
      style={{ left: position.x, top: position.y }}
      aria-label="Test Tools (Adapter)"
    >
      <div
        className="tt-header"
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={endDrag}
        onPointerCancel={endDrag}
      >
        <span className="tt-grip" aria-hidden="true"><GripPixelIcon size={13} /></span>
        <span className="tt-title">TEST TOOLS (ADAPTER)</span>
        <span className="tt-live" aria-hidden="true">
          UI
        </span>
        <button
          type="button"
          className="tt-collapse"
          aria-expanded={!collapsed}
          aria-label={collapsed ? "Развернуть Test Tools" : "Свернуть Test Tools"}
          onClick={() => onCollapsedChange(!collapsed)}
        >
          <ChevronDown size={14} className={collapsed ? "tt-chevron-closed" : ""} />
        </button>
      </div>

      {!collapsed ? (
        <div className="tt-body">
          <div className="tt-status">
            <div className="tt-status-row">
              <span className="tt-dot" aria-hidden="true" />
              <span className="tt-status-name">Katana:</span>
              <span className="tt-status-value">ожидание</span>
            </div>
            <div className="tt-status-row tt-status-muted">
              <span>Диагностика</span>
              <span className="tt-diag">-</span>
            </div>
          </div>

          {TEST_TOOL_GROUPS.map((group) => (
            <div className="tt-group" key={group.id}>
              <div className="tt-section-head">
                <span>{group.title}</span>
              </div>
              {group.sliderIds.map((sliderId) => {
                const slider = findSlider(sliderId);
                const value = values[slider.id] ?? slider.defaultValue;
                const inputId = `${idPrefix}-${slider.id.replace(/\./g, "-")}`;
                return (
                  <div className="tt-row" key={slider.id}>
                    <label className="tt-label" htmlFor={inputId}>
                      {slider.short ?? slider.label}
                    </label>
                    <div className="tt-range">
                      <RangeInput
                        compact
                        slider={slider}
                        value={value}
                        onChange={(next) => onChange(slider.id, next)}
                        id={inputId}
                      />
                    </div>
                    <output className="tt-value" htmlFor={inputId}>
                      {formatSliderValue(slider, value)}
                    </output>
                  </div>
                );
              })}
            </div>
          ))}

          <div className="tt-footer">
            <span className="tt-footer-dot" aria-hidden="true" />
            Значения синхронизированы с вкладкой «Графика»
          </div>
        </div>
      ) : null}
    </aside>
  );
}
