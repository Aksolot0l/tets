import type { CSSProperties } from "react";
import { EFFECT_MODULES, HUD_MODULES, MODULE_ACCENTS } from "../data/menu";
import { Meter, ModuleRow, Panel, PanelHead, Section } from "../components/ui";
import {
  CrownPixelIcon,
  SparklePixelIcon,
  FpsPixelIcon,
  LinkPixelIcon,
  ModuleListPixelIcon,
  ModulePixelIcon,
} from "../components/PixelIcons";

type ModulesTabProps = {
  modules: Record<string, boolean>;
  onToggleModule: (id: string, next: boolean) => void;
  collapsed: Record<string, boolean>;
  onToggleSection: (key: string) => void;
};

function SparkleNoteIcon() {
  return <SparklePixelIcon size={11} />;
}

export function ModulesTab({ modules, onToggleModule, collapsed, onToggleSection }: ModulesTabProps) {
  const hudActive = HUD_MODULES.filter((item) => modules[item.id]).length;
  const effectsActive = EFFECT_MODULES.filter((item) => modules[item.id]).length;

  return (
    <div className="tab-stack">
      <Panel>
        <PanelHead
          icon={<ModuleListPixelIcon size={20} />}
          title="Игровые модули"
          tag="9 модулей"
          caption="HUD-элементы, которые отрисовываются поверх игры"
          side={<Meter value={hudActive} max={HUD_MODULES.length} />}
        />
        <div className="modules-overview" aria-label="Быстрый обзор HUD-модулей">
          {HUD_MODULES.map((item) => (
            <span
              key={item.id}
              className={modules[item.id] ? "overview-chip is-on" : "overview-chip"}
              style={
                {
                  "--chip-accent": MODULE_ACCENTS[item.id] ?? "var(--accent)",
                } as CSSProperties
              }
              title={item.description}
            >
              <span className="overview-icon" aria-hidden="true">
                <ModulePixelIcon id={item.id} size={13} />
              </span>
              {item.name}
            </span>
          ))}
        </div>
        <div className="panel-sections">
          <Section
            id="modules.hud"
            title="HUD"
            description="Счётчики и информация"
            meta={
              <span className="section-count">
                <i className={hudActive > 0 ? "is-on" : ""} aria-hidden="true" />
                {hudActive} из {HUD_MODULES.length} активно
              </span>
            }
            open={!collapsed["modules.hud"]}
            onToggle={() => onToggleSection("modules.hud")}
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
        <p className="panel-note">
          <LinkPixelIcon size={11} />
          Состояние синхронизировано с вкладкой HUD
          <span className="panel-note-spacer" />
          <FpsPixelIcon size={10} />
          Применяется мгновенно
        </p>
      </Panel>

      <Panel>
        <PanelHead
          icon={<CrownPixelIcon size={20} />}
          title="Эффекты"
          tag="испытания"
          caption="Косметические эффекты и 3D-элементы вокруг игрока"
          side={<Meter value={effectsActive} max={EFFECT_MODULES.length} />}
        />
        <div className="panel-sections">
          <Section
            id="modules.effects"
            title="ЭФФЕКТЫ"
            description="Визуальные модификации"
            meta={
              <span className="section-count">
                <i className={effectsActive > 0 ? "is-on" : ""} aria-hidden="true" />
                {effectsActive} из {EFFECT_MODULES.length} активно
              </span>
            }
            open={!collapsed["modules.effects"]}
            onToggle={() => onToggleSection("modules.effects")}
          >
            <div className="module-list">
              {EFFECT_MODULES.map((item) => (
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
        <p className="panel-note">
          <SparkleNoteIcon />
          Подборка испытательных эффектов / параметры во вкладке «Графика»
        </p>
      </Panel>
    </div>
  );
}
