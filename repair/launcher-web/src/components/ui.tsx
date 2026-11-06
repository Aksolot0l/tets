import { X } from "lucide-react";
import { useEffect, type ReactNode } from "react";
import { cn } from "../utils/cn";

export function Panel({
  children,
  className,
  accent = false,
}: {
  children: ReactNode;
  className?: string;
  accent?: boolean;
}) {
  return (
    <div className={cn("panel rounded-2xl", accent && "panel-accent", className)}>{children}</div>
  );
}

export function SectionTitle({
  title,
  hint,
  action,
  icon,
}: {
  title: string;
  hint?: string;
  action?: ReactNode;
  icon?: ReactNode;
}) {
  return (
    <div className="flex items-start justify-between gap-3">
      <div className="flex items-start gap-3">
        {icon && (
          <span className="mt-0.5 flex h-8 w-8 shrink-0 items-center justify-center rounded-lg border border-white/8 bg-white/4 text-[var(--accent)]">
            {icon}
          </span>
        )}
        <div>
          <h3 className="text-[15px] font-bold text-[#eaf8ee]">{title}</h3>
          {hint && <p className="mt-0.5 text-xs leading-relaxed text-[#83a08c]">{hint}</p>}
        </div>
      </div>
      {action}
    </div>
  );
}

export function PageHeading({
  eyebrow,
  title,
  description,
  action,
}: {
  eyebrow: string;
  title: string;
  description: string;
  action?: ReactNode;
}) {
  return (
    <div className="mt-1 flex flex-wrap items-end justify-between gap-4">
      <div>
        <div className="flex items-center gap-2 text-[10px] font-extrabold tracking-[0.22em] text-[var(--accent)] uppercase">
          <span className="h-1 w-1 rounded-full bg-[var(--accent)] shadow-[0_0_8px_2px_rgba(var(--accent-rgb),0.45)]" />
          {eyebrow}
        </div>
        <h2 className="mt-2 font-display text-2xl leading-tight text-[#f2fbf4] sm:text-[28px]">{title}</h2>
        <p className="mt-1.5 max-w-xl text-sm leading-relaxed text-[#8ba993]">{description}</p>
      </div>
      {action}
    </div>
  );
}

export function Toggle({
  checked,
  onChange,
  label,
  disabled,
}: {
  checked: boolean;
  onChange: (value: boolean) => void;
  label: string;
  disabled?: boolean;
}) {
  return (
    <button
      type="button"
      role="switch"
      aria-checked={checked}
      aria-label={label}
      disabled={disabled}
      onClick={() => onChange(!checked)}
      className={cn("toggle-track", disabled && "opacity-40")}
      data-on={checked}
    >
      <span className="toggle-knob" />
    </button>
  );
}

export function SettingRow({
  icon,
  title,
  description,
  checked,
  onChange,
  badge,
}: {
  icon?: ReactNode;
  title: string;
  description: string;
  checked: boolean;
  onChange: (value: boolean) => void;
  badge?: string;
}) {
  return (
    <div className="flex items-start justify-between gap-4 py-3.5 first:pt-0 last:pb-0">
      <div className="flex min-w-0 items-start gap-3">
        {icon && (
          <span className="mt-0.5 flex h-8 w-8 shrink-0 items-center justify-center rounded-lg bg-white/4 text-[#9fc7ab]">
            {icon}
          </span>
        )}
        <div className="min-w-0">
          <div className="flex flex-wrap items-center gap-2">
            <p className="text-[13.5px] font-bold text-[#e6f5ea]">{title}</p>
            {badge && (
              <span className="rounded-md border border-white/8 bg-white/4 px-1.5 py-0.5 text-[9px] font-bold tracking-wide text-[#8dab96] uppercase">
                {badge}
              </span>
            )}
          </div>
          <p className="mt-0.5 text-[11.5px] leading-relaxed text-[#7d9a86]">{description}</p>
        </div>
      </div>
      <Toggle checked={checked} onChange={onChange} label={title} />
    </div>
  );
}

export function Modal({
  open,
  onClose,
  title,
  description,
  children,
  footer,
  width = "max-w-lg",
  icon,
}: {
  open: boolean;
  onClose: () => void;
  title: string;
  description?: string;
  children?: ReactNode;
  footer?: ReactNode;
  width?: string;
  icon?: ReactNode;
}) {
  useEffect(() => {
    if (!open) return;
    const onKey = (event: KeyboardEvent) => {
      if (event.key === "Escape") onClose();
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [open, onClose]);

  if (!open) return null;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4" role="presentation">
      <div
        className="absolute inset-0 bg-[#020704]/80 backdrop-blur-md"
        onClick={onClose}
        aria-hidden="true"
      />
      <div
        role="dialog"
        aria-modal="true"
        aria-label={title}
        className={cn(
          "animate-fade-up relative w-full overflow-hidden rounded-2xl border border-white/10 bg-[linear-gradient(160deg,#122419,#07110b)] shadow-[0_40px_120px_-30px_#000]",
          width,
        )}
      >
        <div className="absolute inset-x-0 top-0 h-px bg-gradient-to-r from-transparent via-[rgba(var(--accent-rgb),0.6)] to-transparent" />
        <div className="flex items-start gap-3 p-5 pb-4 sm:p-6 sm:pb-4">
          {icon && (
            <span className="flex h-10 w-10 shrink-0 items-center justify-center rounded-xl border border-[rgba(var(--accent-rgb),0.25)] bg-[rgba(var(--accent-rgb),0.1)] text-[var(--accent)]">
              {icon}
            </span>
          )}
          <div className="min-w-0 flex-1">
            <h3 className="font-display text-lg text-[#f1fbf4]">{title}</h3>
            {description && <p className="mt-1 text-xs leading-relaxed text-[#8ba993]">{description}</p>}
          </div>
          <button
            type="button"
            onClick={onClose}
            aria-label="Закрыть окно"
            className="btn-ghost -mt-1 -mr-1 h-8 w-8 shrink-0 p-0"
          >
            <X size={16} />
          </button>
        </div>
        {children && <div className="px-5 pb-5 sm:px-6">{children}</div>}
        {footer && (
          <div className="flex flex-wrap items-center justify-end gap-2 border-t border-white/6 bg-black/15 px-5 py-4 sm:px-6">
            {footer}
          </div>
        )}
      </div>
    </div>
  );
}

export function PixelLeaves({ className }: { className?: string }) {
  return (
    <svg
      viewBox="0 0 120 120"
      aria-hidden="true"
      className={cn("pointer-events-none text-[#2f6d45]", className)}
      fill="currentColor"
    >
      <rect x="54" y="76" width="6" height="30" opacity="0.8" />
      <rect x="48" y="100" width="6" height="10" opacity="0.6" />
      <rect x="60" y="94" width="6" height="14" opacity="0.5" />
      <rect x="42" y="60" width="10" height="10" />
      <rect x="32" y="50" width="12" height="12" opacity="0.8" />
      <rect x="22" y="42" width="10" height="10" opacity="0.55" />
      <rect x="66" y="52" width="12" height="12" opacity="0.9" />
      <rect x="78" y="42" width="12" height="12" opacity="0.65" />
      <rect x="90" y="32" width="8" height="8" opacity="0.4" />
      <rect x="50" y="46" width="10" height="10" opacity="0.7" />
      <rect x="38" y="32" width="8" height="8" opacity="0.4" />
      <rect x="72" y="26" width="8" height="8" opacity="0.35" />
    </svg>
  );
}

export function PixelSparkles({ className }: { className?: string }) {
  return (
    <div className={cn("pointer-events-none absolute inset-0 overflow-hidden", className)} aria-hidden="true">
      <span className="pixel-spark left-[8%] top-[22%] h-[3px] w-[3px] rounded-[1px]" />
      <span className="pixel-spark left-[18%] top-[62%] h-[4px] w-[4px] rounded-[1px] [animation-delay:1.2s]" />
      <span className="pixel-spark right-[28%] top-[18%] h-[3px] w-[3px] rounded-[1px] [animation-delay:0.5s]" />
      <span className="pixel-spark right-[12%] top-[56%] h-[5px] w-[5px] rounded-[1px] [animation-delay:2.1s]" />
      <span className="pixel-spark left-[44%] top-[12%] h-[2px] w-[2px] rounded-[1px] [animation-delay:1.7s]" />
    </div>
  );
}

export function CreeperArt({ className }: { className?: string }) {
  const blocks = [
    { x: 2, y: 2, w: 5, h: 4, o: 0.9 },
    { x: 9, y: 2, w: 5, h: 4, o: 0.9 },
    { x: 2, y: 7, w: 3, h: 3, o: 0.85 },
    { x: 5, y: 8, w: 2, h: 4, o: 0.8 },
    { x: 9, y: 8, w: 2, h: 4, o: 0.8 },
    { x: 11, y: 7, w: 3, h: 3, o: 0.85 },
    { x: 7, y: 11, w: 2, h: 4, o: 0.75 },
    { x: 3, y: 12, w: 3, h: 3, o: 0.6 },
    { x: 10, y: 12, w: 3, h: 3, o: 0.6 },
    { x: 6, y: 3, w: 4, h: 3, o: 0.35 },
    { x: 0, y: 5, w: 2, h: 5, o: 0.4 },
    { x: 14, y: 5, w: 2, h: 5, o: 0.4 },
  ];
  return (
    <svg viewBox="0 0 16 16" aria-hidden="true" className={className} shapeRendering="crispEdges">
      {blocks.map((block, index) => (
        <rect
          key={index}
          x={block.x}
          y={block.y}
          width={block.w}
          height={block.h}
          fill="currentColor"
          opacity={block.o}
        />
      ))}
    </svg>
  );
}

export function Bars({ active = false, loading = false }: { active?: boolean; loading?: boolean }) {
  const heights = [12, 20, 16, 27];
  return (
    <div className="flex h-9 items-end gap-[4px]">
      {heights.map((height, index) => (
        <span
          key={index}
          className={cn(
            "w-[5px] origin-bottom rounded-[1px] transition-colors duration-500",
            active ? "bg-[var(--accent)]" : "bg-[#3b5f49]",
            loading && "animate-pulse-soft",
          )}
          style={{
            height,
            animationDelay: `${index * 0.18}s`,
            boxShadow: active ? "0 0 8px rgba(var(--accent-rgb),0.45)" : undefined,
          }}
        />
      ))}
    </div>
  );
}

export function EmptyState({
  icon,
  title,
  description,
  action,
}: {
  icon: ReactNode;
  title: string;
  description: string;
  action?: ReactNode;
}) {
  return (
    <div className="flex flex-col items-center justify-center rounded-xl border border-dashed border-white/10 bg-black/10 px-6 py-12 text-center">
      <span className="mb-3 flex h-12 w-12 items-center justify-center rounded-2xl border border-white/8 bg-white/4 text-[#75937e]">
        {icon}
      </span>
      <p className="text-sm font-bold text-[#dcece1]">{title}</p>
      <p className="mt-1 max-w-xs text-xs leading-relaxed text-[#7d9a86]">{description}</p>
      {action && <div className="mt-4">{action}</div>}
    </div>
  );
}
