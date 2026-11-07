import type { ReactNode } from "react";

export type PixelIconProps = {
  size?: number;
  className?: string;
};

function PixelIcon({ size = 24, className, children }: PixelIconProps & { children: ReactNode }) {
  return (
    <svg
      className={className ? `pixel-icon ${className}` : "pixel-icon"}
      width={size}
      height={size}
      viewBox="0 0 10 10"
      shapeRendering="crispEdges"
      aria-hidden="true"
      focusable="false"
    >
      {children}
    </svg>
  );
}

export function FpsPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="5" y="0" width="2" height="1" />
        <rect x="4" y="1" width="2" height="1" />
        <rect x="4" y="2" width="2" height="1" />
        <rect x="3" y="3" width="2" height="1" />
        <rect x="3" y="4" width="4" height="1" />
        <rect x="4" y="5" width="2" height="1" opacity="0.85" />
        <rect x="4" y="6" width="2" height="1" opacity="0.75" />
        <rect x="3" y="7" width="2" height="1" opacity="0.65" />
        <rect x="3" y="8" width="1" height="1" opacity="0.5" />
        <rect x="8" y="2" width="1" height="1" opacity="0.4" />
        <rect x="8" y="4" width="1" height="1" opacity="0.3" />
        <rect x="8" y="6" width="1" height="1" opacity="0.2" />
        <rect x="0" y="3" width="1" height="1" opacity="0.3" />
        <rect x="0" y="5" width="1" height="1" opacity="0.2" />
      </g>
    </PixelIcon>
  );
}

export function CpsPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="2" y="1" width="2" height="1" />
        <rect x="2" y="2" width="3" height="1" />
        <rect x="2" y="3" width="4" height="1" />
        <rect x="2" y="4" width="5" height="1" />
        <rect x="2" y="5" width="4" height="1" />
        <rect x="2" y="6" width="2" height="1" />
        <rect x="3" y="7" width="2" height="1" opacity="0.8" />
        <rect x="4" y="8" width="1" height="1" opacity="0.6" />
        <rect x="7" y="0" width="1" height="1" opacity="0.75" />
        <rect x="9" y="2" width="1" height="1" opacity="0.55" />
        <rect x="6" y="3" width="1" height="1" opacity="0.4" />
        <rect x="9" y="5" width="1" height="1" opacity="0.3" />
        <rect x="7" y="6" width="1" height="1" opacity="0.25" />
      </g>
    </PixelIcon>
  );
}

export function ClockPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <g opacity="0.62">
          <rect x="3" y="1" width="4" height="1" />
          <rect x="2" y="2" width="1" height="1" />
          <rect x="7" y="2" width="1" height="1" />
          <rect x="1" y="3" width="1" height="4" />
          <rect x="8" y="3" width="1" height="4" />
          <rect x="2" y="7" width="1" height="1" />
          <rect x="7" y="7" width="1" height="1" />
          <rect x="3" y="8" width="4" height="1" />
        </g>
        <rect x="4" y="3" width="1" height="3" />
        <rect x="5" y="5" width="2" height="1" />
        <rect x="4" y="5" width="1" height="1" opacity="0.7" />
        <rect x="4" y="0" width="1" height="1" opacity="0.3" />
        <rect x="4" y="9" width="1" height="1" opacity="0.3" />
      </g>
    </PixelIcon>
  );
}

export function KeysPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="2" width="8" height="1" opacity="0.75" />
        <rect x="1" y="3" width="1" height="4" opacity="0.75" />
        <rect x="8" y="3" width="1" height="4" opacity="0.75" />
        <rect x="2" y="6" width="6" height="1" opacity="0.5" />
        <rect x="2" y="3" width="6" height="1" opacity="0.18" />
        <rect x="2" y="4" width="1" height="2" opacity="0.35" />
        <rect x="7" y="4" width="1" height="2" opacity="0.35" />
        <rect x="3" y="8" width="4" height="1" opacity="0.45" />
        <rect x="3" y="4" width="1" height="1" />
        <rect x="3" y="7" width="3" height="1" opacity="0.65" />
      </g>
    </PixelIcon>
  );
}

export function WatermarkPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="2" width="8" height="1" opacity="0.72" />
        <rect x="1" y="3" width="1" height="5" opacity="0.72" />
        <rect x="8" y="3" width="1" height="3" opacity="0.72" />
        <rect x="4" y="5" width="4" height="3" opacity="0.5" />
        <rect x="2" y="4" width="4" height="1" />
        <rect x="2" y="6" width="3" height="1" opacity="0.55" />
        <rect x="2" y="8" width="3" height="1" opacity="0.3" />
        <rect x="7" y="6" width="1" height="1" opacity="0.4" />
        <rect x="0" y="0" width="1" height="1" opacity="0.25" />
        <rect x="9" y="0" width="1" height="1" opacity="0.25" />
      </g>
    </PixelIcon>
  );
}

export function CoordsPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="4" y="1" width="1" height="7" opacity="0.8" />
        <rect x="1" y="5" width="7" height="1" opacity="0.8" />
        <rect x="3" y="0" width="3" height="1" />
        <rect x="0" y="4" width="1" height="3" />
        <rect x="8" y="4" width="1" height="3" />
        <rect x="3" y="8" width="3" height="1" opacity="0.45" />
        <rect x="6" y="2" width="1" height="1" opacity="0.65" />
        <rect x="7" y="1" width="1" height="1" opacity="0.45" />
        <rect x="8" y="2" width="1" height="1" opacity="0.8" />
        <rect x="4" y="4" width="2" height="2" />
      </g>
    </PixelIcon>
  );
}

export function ResolutionPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="1" width="8" height="1" opacity="0.8" />
        <rect x="1" y="2" width="1" height="4" opacity="0.8" />
        <rect x="8" y="2" width="1" height="4" opacity="0.8" />
        <rect x="1" y="6" width="8" height="1" opacity="0.8" />
        <rect x="2" y="3" width="2" height="2" />
        <rect x="5" y="3" width="3" height="1" opacity="0.55" />
        <rect x="5" y="5" width="2" height="1" opacity="0.35" />
        <rect x="4" y="7" width="2" height="1" opacity="0.65" />
        <rect x="3" y="8" width="4" height="1" opacity="0.5" />
      </g>
    </PixelIcon>
  );
}

export function PerformancePixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="8" width="8" height="1" opacity="0.55" />
        <rect x="2" y="5" width="1" height="3" opacity="0.7" />
        <rect x="4" y="3" width="1" height="5" />
        <rect x="6" y="1" width="1" height="7" />
        <rect x="8" y="4" width="1" height="4" opacity="0.45" />
        <rect x="2" y="4" width="1" height="1" opacity="0.3" />
        <rect x="4" y="2" width="1" height="1" opacity="0.45" />
        <rect x="6" y="0" width="1" height="1" opacity="0.7" />
      </g>
    </PixelIcon>
  );
}

export function ModuleListPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="1" width="2" height="2" />
        <rect x="4" y="1" width="5" height="1" opacity="0.72" />
        <rect x="4" y="2" width="3" height="1" opacity="0.35" />
        <rect x="1" y="4" width="2" height="2" opacity="0.8" />
        <rect x="4" y="4" width="4" height="1" opacity="0.55" />
        <rect x="4" y="5" width="2" height="1" opacity="0.28" />
        <rect x="1" y="7" width="2" height="2" opacity="0.5" />
        <rect x="4" y="7" width="5" height="1" opacity="0.4" />
        <rect x="4" y="8" width="3" height="1" opacity="0.22" />
      </g>
    </PixelIcon>
  );
}

export function ChinaHatPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="4" y="1" width="2" height="1" opacity="0.55" />
        <rect x="4" y="2" width="2" height="1" opacity="0.8" />
        <rect x="3" y="3" width="4" height="1" opacity="0.85" />
        <rect x="3" y="4" width="4" height="1" opacity="0.7" />
        <rect x="2" y="5" width="6" height="1" opacity="0.6" />
        <rect x="1" y="6" width="8" height="1" opacity="0.5" />
        <rect x="0" y="7" width="10" height="1" opacity="0.9" />
        <rect x="1" y="8" width="8" height="1" opacity="0.25" />
        <rect x="6" y="3" width="1" height="3" fill="#0a1f12" opacity="0.4" />
        <rect x="5" y="5" width="2" height="1" fill="#0a1f12" opacity="0.3" />
      </g>
    </PixelIcon>
  );
}

export function TrailPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="0" y="3" width="2" height="2" opacity="0.25" />
        <rect x="2" y="4" width="2" height="2" opacity="0.38" />
        <rect x="4" y="4" width="2" height="2" opacity="0.52" />
        <rect x="5" y="6" width="2" height="2" opacity="0.68" />
        <rect x="7" y="6" width="2" height="2" opacity="0.85" />
        <rect x="8" y="8" width="2" height="2" />
        <rect x="1" y="8" width="6" height="1" opacity="0.25" />
        <rect x="0" y="1" width="1" height="1" opacity="0.2" />
      </g>
    </PixelIcon>
  );
}

export function JumpRingsPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <g opacity="0.45">
          <rect x="3" y="1" width="4" height="1" />
          <rect x="2" y="2" width="1" height="1" />
          <rect x="7" y="2" width="1" height="1" />
          <rect x="1" y="3" width="1" height="4" />
          <rect x="8" y="3" width="1" height="4" />
          <rect x="2" y="7" width="1" height="1" />
          <rect x="7" y="7" width="1" height="1" />
          <rect x="3" y="8" width="4" height="1" />
        </g>
        <rect x="4" y="4" width="2" height="1" />
        <rect x="3" y="5" width="1" height="2" opacity="0.8" />
        <rect x="6" y="5" width="1" height="2" opacity="0.8" />
        <rect x="4" y="6" width="2" height="1" opacity="0.65" />
        <rect x="0" y="1" width="1" height="1" opacity="0.55" />
        <rect x="9" y="8" width="1" height="1" opacity="0.55" />
      </g>
    </PixelIcon>
  );
}

export function KatanaPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="8" y="1" width="1" height="1" />
        <rect x="7" y="2" width="2" height="1" />
        <rect x="6" y="3" width="2" height="1" opacity="0.9" />
        <rect x="5" y="4" width="2" height="1" opacity="0.85" />
        <rect x="4" y="5" width="2" height="1" opacity="0.8" />
        <rect x="3" y="6" width="2" height="1" opacity="0.75" />
        <rect x="2" y="7" width="2" height="1" opacity="0.7" />
        <rect x="1" y="8" width="2" height="1" opacity="0.65" />
        <rect x="2" y="5" width="2" height="1" opacity="0.4" />
        <rect x="4" y="7" width="2" height="1" opacity="0.5" />
        <rect x="0" y="7" width="1" height="1" opacity="0.55" />
        <rect x="1" y="6" width="1" height="1" opacity="0.4" />
        <rect x="2" y="9" width="2" height="1" opacity="0.45" />
        <rect x="3" y="8" width="1" height="1" opacity="0.3" />
        <rect x="0" y="5" width="1" height="1" opacity="0.25" />
      </g>
    </PixelIcon>
  );
}

export function CrownPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="3" width="1" height="4" opacity="0.9" />
        <rect x="4" y="2" width="1" height="5" opacity="0.9" />
        <rect x="7" y="3" width="1" height="4" opacity="0.9" />
        <rect x="2" y="4" width="1" height="3" opacity="0.7" />
        <rect x="6" y="4" width="1" height="3" opacity="0.7" />
        <rect x="3" y="5" width="3" height="2" opacity="0.8" />
        <rect x="1" y="6" width="8" height="1" opacity="0.75" />
        <rect x="1" y="7" width="8" height="2" opacity="0.9" />
        <rect x="0" y="4" width="1" height="1" opacity="0.55" />
        <rect x="8" y="4" width="1" height="1" opacity="0.55" />
        <rect x="3" y="7" width="1" height="1" fill="currentColor" opacity="0.35" />
        <rect x="5" y="4" width="1" height="1" fill="currentColor" opacity="0.4" />
      </g>
    </PixelIcon>
  );
}

export const MODULE_PIXEL_ICONS = {
  fps: FpsPixelIcon,
  cps: CpsPixelIcon,
  clock: ClockPixelIcon,
  keys: KeysPixelIcon,
  watermark: WatermarkPixelIcon,
  coords: CoordsPixelIcon,
  resolution: ResolutionPixelIcon,
  performance: PerformancePixelIcon,
  moduleList: ModuleListPixelIcon,
  chinaHat: ChinaHatPixelIcon,
  trail: TrailPixelIcon,
  jumpRings: JumpRingsPixelIcon,
  katana: KatanaPixelIcon,
  crown: CrownPixelIcon,
} as const;

export function ModulePixelIcon({ id, size = 24, className }: PixelIconProps & { id: string }) {
  const Icon = MODULE_PIXEL_ICONS[id as keyof typeof MODULE_PIXEL_ICONS] ?? FpsPixelIcon;
  return <Icon size={size} className={className} />;
}

/* Interface glyphs drawn in the same 10x10 pixel grid. */

export function SlidersPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="2" width="8" height="1" opacity="0.55" />
        <rect x="3" y="1" width="2" height="3" />
        <rect x="1" y="5" width="8" height="1" opacity="0.55" />
        <rect x="6" y="4" width="2" height="3" opacity="0.85" />
        <rect x="1" y="8" width="8" height="1" opacity="0.55" />
        <rect x="2" y="7" width="2" height="3" opacity="0.7" />
      </g>
    </PixelIcon>
  );
}

export function InfoPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="4" y="1" width="2" height="2" />
        <rect x="4" y="4" width="2" height="5" />
        <rect x="2" y="4" width="1" height="1" opacity="0.4" />
        <rect x="7" y="4" width="1" height="1" opacity="0.4" />
      </g>
    </PixelIcon>
  );
}

export function ShieldPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="2" y="1" width="6" height="1" opacity="0.8" />
        <rect x="1" y="2" width="8" height="3" opacity="0.8" />
        <rect x="2" y="5" width="6" height="2" opacity="0.65" />
        <rect x="3" y="7" width="4" height="1" opacity="0.5" />
        <rect x="4" y="8" width="2" height="1" opacity="0.38" />
        <rect x="4" y="3" width="1" height="3" fill="#0a1f12" opacity="0.6" />
        <rect x="5" y="3" width="2" height="1" fill="#0a1f12" opacity="0.6" />
      </g>
    </PixelIcon>
  );
}

export function PalettePixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="3" y="1" width="4" height="1" />
        <rect x="1" y="2" width="1" height="5" />
        <rect x="2" y="2" width="2" height="1" opacity="0.75" />
        <rect x="8" y="3" width="1" height="2" opacity="0.7" />
        <rect x="2" y="7" width="4" height="1" opacity="0.7" />
        <rect x="5" y="6" width="2" height="1" opacity="0.55" />
        <rect x="6" y="7" width="1" height="2" opacity="0.45" />
        <rect x="3" y="3" width="1" height="1" fill="#0a1f12" opacity="0.65" />
        <rect x="6" y="3" width="1" height="1" fill="#0a1f12" opacity="0.5" />
        <rect x="3" y="5" width="1" height="1" fill="#0a1f12" opacity="0.6" />
      </g>
    </PixelIcon>
  );
}

export function CrosshairPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor" opacity="0.8">
        <rect x="4" y="1" width="2" height="1" />
        <rect x="4" y="8" width="2" height="1" />
        <rect x="1" y="4" width="1" height="2" />
        <rect x="8" y="4" width="1" height="2" />
      </g>
      <g fill="currentColor">
        <rect x="3" y="3" width="4" height="1" />
        <rect x="3" y="6" width="4" height="1" />
        <rect x="3" y="4" width="1" height="2" />
        <rect x="6" y="4" width="1" height="2" />
        <rect x="4" y="4" width="2" height="2" />
      </g>
    </PixelIcon>
  );
}

export function WifiOffPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor" opacity="0.7">
        <rect x="3" y="2" width="4" height="1" />
        <rect x="2" y="4" width="6" height="1" />
        <rect x="1" y="6" width="3" height="1" />
      </g>
      <g fill="currentColor">
        <rect x="4" y="7" width="2" height="2" />
        <rect x="2" y="1" width="1" height="1" opacity="0.85" />
        <rect x="3" y="2" width="1" height="1" opacity="0.85" />
        <rect x="4" y="3" width="1" height="1" opacity="0.85" />
        <rect x="5" y="4" width="1" height="1" opacity="0.85" />
        <rect x="6" y="5" width="1" height="1" opacity="0.85" />
        <rect x="7" y="6" width="1" height="1" opacity="0.85" />
        <rect x="8" y="7" width="1" height="1" opacity="0.85" />
      </g>
    </PixelIcon>
  );
}

export function CheckPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="7" y="2" width="2" height="2" />
        <rect x="6" y="3" width="1" height="1" opacity="0.8" />
        <rect x="5" y="4" width="1" height="1" opacity="0.7" />
        <rect x="3" y="5" width="2" height="2" />
        <rect x="2" y="4" width="1" height="1" opacity="0.7" />
        <rect x="1" y="3" width="1" height="1" opacity="0.5" />
      </g>
    </PixelIcon>
  );
}

export function AlertPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="4" y="1" width="2" height="1" />
        <rect x="4" y="2" width="2" height="3" />
        <rect x="3" y="3" width="4" height="1" opacity="0.7" />
        <rect x="2" y="4" width="6" height="2" opacity="0.8" />
        <rect x="4" y="6" width="2" height="1" />
        <rect x="4" y="8" width="2" height="1" opacity="0.6" />
      </g>
    </PixelIcon>
  );
}

export function SparklePixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="4" y="1" width="2" height="2" />
        <rect x="4" y="7" width="2" height="2" opacity="0.8" />
        <rect x="1" y="4" width="2" height="2" opacity="0.8" />
        <rect x="7" y="4" width="2" height="2" />
        <rect x="4" y="4" width="2" height="2" />
        <rect x="2" y="2" width="1" height="1" opacity="0.45" />
        <rect x="7" y="7" width="1" height="1" opacity="0.45" />
      </g>
    </PixelIcon>
  );
}

export function LinkPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor" opacity="0.85">
        <rect x="1" y="3" width="3" height="1" />
        <rect x="1" y="6" width="3" height="1" />
        <rect x="1" y="4" width="1" height="2" />
        <rect x="6" y="3" width="3" height="1" />
        <rect x="6" y="6" width="3" height="1" />
        <rect x="8" y="4" width="1" height="2" />
        <rect x="4" y="4" width="2" height="2" />
        <rect x="3" y="4" width="1" height="2" opacity="0.5" />
        <rect x="6" y="4" width="1" height="2" opacity="0.5" />
      </g>
    </PixelIcon>
  );
}

export function GripPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="3" y="2" width="1" height="1" />
        <rect x="6" y="2" width="1" height="1" />
        <rect x="3" y="5" width="1" height="1" />
        <rect x="6" y="5" width="1" height="1" />
        <rect x="3" y="8" width="1" height="1" />
        <rect x="6" y="8" width="1" height="1" />
      </g>
    </PixelIcon>
  );
}

export function GripHorizontalPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="4" width="2" height="2" />
        <rect x="4" y="4" width="2" height="2" />
        <rect x="7" y="4" width="2" height="2" />
      </g>
    </PixelIcon>
  );
}

export function ArrowPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="4" y="0" width="2" height="4" />
        <rect x="3" y="1" width="1" height="2" opacity="0.7" />
        <rect x="6" y="1" width="1" height="2" opacity="0.7" />
        <rect x="4" y="4" width="2" height="2" opacity="0.6" />
        <rect x="0" y="4" width="4" height="2" opacity="0.5" />
        <rect x="6" y="4" width="4" height="2" opacity="0.5" />
        <rect x="4" y="6" width="2" height="4" opacity="0.85" />
        <rect x="3" y="7" width="1" height="2" opacity="0.6" />
        <rect x="6" y="7" width="1" height="2" opacity="0.6" />
      </g>
    </PixelIcon>
  );
}

export function ScanPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="1" y="1" width="3" height="1" />
        <rect x="1" y="1" width="1" height="3" />
        <rect x="6" y="1" width="3" height="1" />
        <rect x="8" y="1" width="1" height="3" />
        <rect x="1" y="6" width="1" height="3" />
        <rect x="1" y="8" width="3" height="1" />
        <rect x="8" y="6" width="1" height="3" />
        <rect x="6" y="8" width="3" height="1" />
        <rect x="2" y="4" width="6" height="2" opacity="0.75" />
      </g>
    </PixelIcon>
  );
}

export function PencilPixelIcon(props: PixelIconProps) {
  return (
    <PixelIcon {...props}>
      <g fill="currentColor">
        <rect x="7" y="1" width="2" height="2" />
        <rect x="6" y="2" width="2" height="2" opacity="0.9" />
        <rect x="5" y="3" width="2" height="2" opacity="0.8" />
        <rect x="4" y="4" width="2" height="2" opacity="0.7" />
        <rect x="3" y="5" width="2" height="2" opacity="0.6" />
        <rect x="2" y="6" width="2" height="2" opacity="0.5" />
        <rect x="1" y="7" width="2" height="2" opacity="0.4" />
        <rect x="1" y="8" width="1" height="1" />
      </g>
    </PixelIcon>
  );
}
