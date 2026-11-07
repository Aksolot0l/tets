import { useId, type CSSProperties, type ReactNode } from "react";

import { ChevronDown } from "lucide-react";
import { cn } from "../utils/cn";
import { formatSliderValue, MODULE_ACCENTS, type ModuleItem, type SliderDef } from "../data/menu";
import { ModulePixelIcon } from "./PixelIcons";

export function Panel({ children, className }: { children: ReactNode; className?: string }) {
  return <section className={cn("panel", className)}>{children}</section>;
}

type PanelHeadProps = {
  icon: ReactNode;
  title: string;
  tag?: string;
  caption?: string;
  side?: ReactNode;
};

export function PanelHead({ icon, title, tag, caption, side }: PanelHeadProps) {
  return (
    <header className="panel-head">
      <span className="panel-icon" aria-hidden="true">
        {icon}
      </span>
      <div className="panel-heading">
        <div className="panel-title-row">
          <h2 className="panel-title">{title}</h2>
          {tag ? <span className="badge-accent">{tag}</span> : null}
        </div>
        {caption ? <p className="panel-caption">{caption}</p> : null}
      </div>
      {side ? <div className="panel-head-side">{side}</div> : null}
    </header>
  );
}

export function Meter({ value, max }: { value: number; max: number }) {
  const share = max > 0 ? Math.round((value / max) * 100) : 0;
  return (
    <div
      className="meter"
      style={{ "--pct": `${share}%` } as CSSProperties}
      role="img"
      aria-label={`Активно ${value} из ${max}`}
    >
      <span>{value}</span>
    </div>
  );
}

type ToggleProps = {
  checked: boolean;
  onChange: (next: boolean) => void;
  label: string;
  className?: string;
};

export function Toggle({ checked, onChange, label, className }: ToggleProps) {
  return (
    <button
      type="button"
      role="switch"
      aria-checked={checked}
      aria-label={label}
      title={label}
      className={cn("toggle", className)}
      onClick={() => onChange(!checked)}
    >
      <span className="toggle-knob" aria-hidden="true" />
    </button>
  );
}

export function ToggleTile({
  checked,
  onChange,
  label,
}: {
  checked: boolean;
  onChange: (next: boolean) => void;
  label: string;
}) {
  return (
    <div className={cn("toggle-tile", checked && "is-on")}>
      <span className="toggle-tile-label">{label}</span>
      <Toggle checked={checked} onChange={onChange} label={label} />
    </div>
  );
}

type RangeInputProps = {
  slider: SliderDef;
  value: number;
  onChange: (value: number) => void;
  id?: string;
  compact?: boolean;
  className?: string;
};

export function RangeInput({ slider, value, onChange, id, compact = false, className }: RangeInputProps) {
  const safeValue = Number.isFinite(value) ? value : slider.defaultValue;
  const share = ((safeValue - slider.min) / (slider.max - slider.min)) * 100;
  const clampedShare = Math.min(100, Math.max(0, share));

  return (
    <input
      id={id}
      className={cn("range", compact && "range-compact", className)}
      style={{ "--val": `${clampedShare}%` } as CSSProperties}
      type="range"
      min={slider.min}
      max={slider.max}
      step={slider.step}
      value={safeValue}
      aria-label={slider.label}
      aria-valuetext={formatSliderValue(slider, safeValue)}
      onChange={(event) => onChange(Number(event.currentTarget.value))}
    />
  );
}

export function ColorSwatch({ color, title, className }: { color: string; title?: string; className?: string }) {
  return (
    <span
      className={cn("color-swatch", className)}
      style={{ background: color, color }}
      title={title}
      aria-hidden="true"
    />
  );
}

export function SliderRow({
  slider,
  value,
  onChange,
  swatchColor,
}: {
  slider: SliderDef;
  value: number;
  onChange: (value: number) => void;
  swatchColor?: string;
}) {
  const inputId = useId();
  const display = formatSliderValue(slider, value);

  return (
    <div className="slider-row">
      <label className="slider-label" htmlFor={inputId}>
        <span className="slider-label-text">{slider.label}</span>
        {swatchColor ? <ColorSwatch color={swatchColor} title={`Текущий цвет: ${swatchColor}`} /> : null}
      </label>
      <div className="slider-control">
        <RangeInput slider={slider} value={value} onChange={onChange} id={inputId} />
      </div>
      <output className="slider-value" htmlFor={inputId}>
        {display}
      </output>
    </div>
  );
}

type SectionProps = {
  id: string;
  title: string;
  description?: string;
  meta?: ReactNode;
  side?: ReactNode;
  preview?: ReactNode;
  open: boolean;
  onToggle: () => void;
  children: ReactNode;
};

export function Section({ id, title, description, meta, side, preview, open, onToggle, children }: SectionProps) {
  const bodyId = `section-body-${id.replace(/\./g, "-")}`;

  return (
    <section className={cn("section", open && "is-open", preview && "has-preview")}>
      <button
        type="button"
        className="section-head"
        aria-expanded={open}
        aria-controls={bodyId}
        onClick={onToggle}
      >
        <span className={cn("section-chevron", !open && "is-closed")} aria-hidden="true">
          <ChevronDown size={14} strokeWidth={2.4} />
        </span>
        <span className="section-title">{title}</span>
        {description ? <span className="section-desc">{description}</span> : null}
        {meta ? <span className="section-meta">{meta}</span> : null}
        {preview ? <span className="section-preview">{preview}</span> : null}
        {side ? <span className="section-side">{side}</span> : null}
      </button>
      <div className="section-body" data-open={open} id={bodyId} aria-hidden={!open} inert={!open}>
        <div className="section-inner">
          <div className="section-content">{children}</div>
        </div>
      </div>
    </section>
  );
}

export function ModuleRow({
  item,
  checked,
  onToggle,
}: {
  item: ModuleItem;
  checked: boolean;
  onToggle: (next: boolean) => void;
}) {
  const accent = MODULE_ACCENTS[item.id] ?? "var(--accent)";
  return (
    <div
      className={cn("module-row", checked && "is-on")}
      style={{ "--chip-accent": accent } as CSSProperties}
    >
      <span className="module-chip" aria-hidden="true">
        <ModulePixelIcon id={item.id} size={26} />
      </span>
      <div className="module-copy">
        <span className="module-name">{item.name}</span>
        <span className="module-desc" title={item.description}>
          {item.description}
        </span>
      </div>
      <div className="module-state">
        <span className={cn("module-state-label", checked && "is-on")}>{checked ? "ВКЛ" : "ВЫКЛ"}</span>
        <Toggle
          checked={checked}
          onChange={onToggle}
          label={`${item.name}: ${checked ? "выключить" : "включить"}`}
        />
      </div>
    </div>
  );
}
