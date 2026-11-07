import type { CSSProperties } from "react";
import { Check } from "lucide-react";
import {
  InfoPixelIcon,
  PalettePixelIcon,
  ShieldPixelIcon,
  SparklePixelIcon,
  WifiOffPixelIcon,
} from "../components/PixelIcons";
import { ACCENT_THEMES } from "../data/menu";
import { AbstractBand } from "../components/AbstractArt";
import { PixelAvatar, PixelAxolotl } from "../components/PixelArt";
import { Panel, PanelHead, Toggle } from "../components/ui";

type InfoTabProps = {
  themeId: string;
  onThemeChange: (id: string) => void;
  motion: boolean;
  onMotionChange: (next: boolean) => void;
};

function StatusRow({
  label,
  value,
  dot,
  amber = false,
}: {
  label: string;
  value: string;
  dot?: boolean;
  amber?: boolean;
}) {
  return (
    <div className="status-row">
      <span className="status-key">{label}</span>
      <span className={amber ? "status-value is-amber" : "status-value"}>
        {dot ? <span className="status-dot" aria-hidden="true" /> : null}
        {value}
      </span>
    </div>
  );
}

export function InfoTab({ themeId, onThemeChange, motion, onMotionChange }: InfoTabProps) {
  return (
    <div className="tab-stack">
      <Panel>
        <PanelHead
          icon={<InfoPixelIcon size={20} />}
          title="О клиенте"
          tag="v1.2"
          caption="Сведения о клиенте и версии"
          side={
            <span className="offline-chip">
              <WifiOffPixelIcon size={11} />
              локальный стенд
            </span>
          }
        />

        <div className="identity">
          <div className="identity-avatar">
            <PixelAvatar />
          </div>
          <div className="identity-copy">
            <div className="identity-name">
              <span className="brand-gay">GAY</span>
              <span className="brand-client">CLIENT</span>
            </div>
            <span className="identity-sub">GAME MENU BEDROCK 1.1.5</span>
          </div>
          <div className="identity-mascot">
            <PixelAxolotl />
          </div>
        </div>

        <div className="info-rows">
          <div className="info-row">
            <span className="info-key">Клиент</span>
            <span className="info-value">GAYClient v1.2</span>
          </div>
          <div className="info-row">
            <span className="info-key">Игра</span>
            <span className="info-value">Minecraft Bedrock 1.1.5 (Windows)</span>
          </div>
        </div>
      </Panel>

      <div className="info-grid">
        <Panel>
          <PanelHead
            icon={<ShieldPixelIcon size={20} />}
            title="Статус"
            caption="Текущее состояние стенда"
          />
          <div className="status-rows">
            <StatusRow label="Соединение с ядром" value="активно" dot />
            <StatusRow label="Профили" value="3 профиля" />
            <StatusRow label="Звук" value="включён" />
            <StatusRow label="Режим" value="легаси-фолбэк готов" amber />
          </div>
          <AbstractBand />
        </Panel>

        <Panel className="appearance-panel">
          <PanelHead
            icon={<PalettePixelIcon size={20} />}
            title="Оформление"
            tag="темы"
            caption="Акцент интерфейса и анимации"
          />

          <div className="appearance-block">
            <div className="appearance-label">
              <span>Тема акцента</span>
              <small>применяется сразу</small>
            </div>
            <div className="theme-grid" role="group" aria-label="Тема акцента">
              {ACCENT_THEMES.map((theme) => {
                const isActive = theme.id === themeId;
                return (
                  <button
                    key={theme.id}
                    type="button"
                    className="theme-pill"
                    aria-pressed={isActive}
                    aria-label={`Тема ${theme.label}`}
                    onClick={() => onThemeChange(theme.id)}
                    style={{ "--pill": theme.accent, "--pill-rgb": theme.rgb } as CSSProperties}
                  >
                    <span className="theme-dot" style={{ background: theme.accent }} aria-hidden="true" />
                    {theme.label}
                    {isActive ? <Check size={11} strokeWidth={3} aria-hidden="true" /> : null}
                  </button>
                );
              })}
            </div>
          </div>

          <div className="motion-row">
            <span className="motion-icon" aria-hidden="true">
              <SparklePixelIcon size={14} />
            </span>
            <span className="motion-copy">
              <span className="motion-title">Анимации</span>
              <span className="motion-hint">Аврора, свечение и переходы</span>
            </span>
            <Toggle checked={motion} onChange={onMotionChange} label="Анимации интерфейса" />
          </div>

          <div className="accent-preview" aria-hidden="true">
            <span className="accent-preview-top">
              <span>ACCENT</span>
              <span>LOCAL</span>
            </span>
            <span className="accent-preview-word">GAYCLIENT</span>
            <span className="accent-preview-bar">
              <i />
            </span>
          </div>
        </Panel>
      </div>
    </div>
  );
}
