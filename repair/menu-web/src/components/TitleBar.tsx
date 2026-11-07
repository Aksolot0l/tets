import { X } from "lucide-react";
import type { BridgeStatus } from "../lib/bridge";
import { PixelLogo } from "./PixelArt";

const FPS_LEVELS = [30, 60, 90, 120];

const BRIDGE_LABEL: Record<BridgeStatus, string> = {
  connected: "ИГРА",
  connecting: "ПОИСК",
  demo: "ДЕМО",
};

export function TitleBar({
  fps,
  bridgeStatus,
  onClose,
}: {
  fps: number;
  bridgeStatus: BridgeStatus;
  onClose: () => void;
}) {
  return (
    <header className="titlebar">
      <div className="brand">
        <div className="brand-mark" aria-hidden="true">
          <PixelLogo />
        </div>
        <div className="brand-text">
          <div className="brand-word">
            <span className="brand-gay">GAY</span>
            <span className="brand-client">CLIENT</span>
            <span className="brand-pip" aria-hidden="true" />
          </div>
          <div className="brand-sub">
            <span>GAME MENU</span>
            <span>BEDROCK 1.1.5</span>
          </div>
        </div>
      </div>

      <div className="title-spacer" />

      <div className="fps-meter" title="Живой замер кадров (requestAnimationFrame)">
        <span className="fps-dot" aria-hidden="true" />
        <span className="fps-copy">
          <span className="fps-label">LIVE FPS</span>
          <span className="fps-value">{fps > 0 ? fps : "-"}</span>
        </span>
        <span className="fps-bars" aria-hidden="true">
          {FPS_LEVELS.map((level) => (
            <span key={level} className={fps >= level ? "is-lit" : ""} style={{ height: `${(level / 120) * 100}%` }} />
          ))}
        </span>
      </div>

      <span
        className={`bridge-badge bridge-${bridgeStatus}`}
        title={
          bridgeStatus === "connected"
            ? "Мост активен: изменения применяются в игре"
            : "Игра не найдена — локальный демо-режим"
        }
      >
        <span className="bridge-dot" aria-hidden="true" />
        {BRIDGE_LABEL[bridgeStatus]}
      </span>

      <span className="version-chip">v1.2</span>

      <span className="esc-hint" aria-hidden="true">
        <kbd>ESC</kbd>
      </span>

      <button type="button" className="btn btn-danger icon-btn close-btn" onClick={onClose} aria-label="Закрыть меню (Esc)">
        <X size={17} strokeWidth={2.4} />
      </button>
    </header>
  );
}
