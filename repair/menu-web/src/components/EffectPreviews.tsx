import type { ReactNode } from "react";

type PreviewProps = {
  values: Record<string, number>;
};

function liveColor(values: Record<string, number>, prefix: string): string {
  const r = Math.round((values[`${prefix}.colorR`] ?? 0) * 255);
  const g = Math.round((values[`${prefix}.colorG`] ?? 0) * 255);
  const b = Math.round((values[`${prefix}.colorB`] ?? 0) * 255);
  return `rgb(${r}, ${g}, ${b})`;
}

function PreviewShell({ label, children }: { label: string; children: ReactNode }) {
  return (
    <span className="effect-preview" role="img" aria-label={label} data-effect="preview">
      <svg viewBox="0 0 120 56" width="120" height="56" shapeRendering="crispEdges" aria-hidden="true">
        <rect width="120" height="56" fill="#06110b" />
        <g opacity="0.28" fill="#89a792">
          <rect x="4" y="6" width="1" height="1" />
          <rect x="18" y="3" width="1" height="1" />
          <rect x="33" y="9" width="1" height="1" />
          <rect x="88" y="4" width="1" height="1" />
          <rect x="104" y="9" width="1" height="1" />
          <rect x="113" y="2" width="1" height="1" />
          <rect x="9" y="48" width="1" height="1" />
          <rect x="109" y="45" width="1" height="1" />
        </g>
        {children}
      </svg>
    </span>
  );
}

function ChinaHatPreview({ values }: PreviewProps) {
  const color = liveColor(values, "chinaHat");
  const opacity = (values["chinaHat.opacity"] ?? 70) / 100;
  const radius = values["chinaHat.radius"] ?? 0.85;
  const height = values["chinaHat.height"] ?? 0.6;
  const spread = Math.min(1.7, Math.max(0.55, 0.45 + radius * 0.45));
  const lift = Math.min(1.6, Math.max(0.55, 0.5 + height * 0.55));

  return (
    <PreviewShell label="Превью эффекта China Hat, цвет и форма меняются по настройкам">
      <g fill="#2fa960" opacity={0.32}>
        <rect x="28" y="49" width="64" height="2" />
        <rect x="41" y="47" width="38" height="1" />
      </g>
      <g className="hat-float">
        <g transform={`translate(60 35) scale(${spread} ${lift}) translate(-60 -35)`}>
          <g fill="#0a1f12" opacity="0.7">
            <polygon points="60,14 84,27 60,40 36,27" />
          </g>
          <polygon points="60,10 84,23 60,36 36,23" fill={color} opacity={0.26 * opacity + 0.2} />
          <polygon points="36,23 60,36 60,39 36,26" fill="#0a1f12" opacity="0.34" />
          <polygon points="84,23 60,36 60,39 84,26" fill={color} opacity={0.3 * opacity + 0.18} />

          <g>
            <polygon points="60,17 74,24 60,31 46,24" fill={color} opacity={Math.max(0.3, opacity)} />
            <polygon points="46,24 60,31 60,33 46,26" fill="#0a1f12" opacity="0.36" />
            <polygon points="74,24 60,31 60,33 74,26" fill={color} opacity={Math.max(0.18, opacity * 0.55)} />
          </g>
          <g>
            <polygon points="60,14 78,23 60,32 42,23" fill={color} opacity={Math.max(0.25, opacity * 0.86)} />
            <polygon points="42,23 60,32 60,34 42,25" fill="#0a1f12" opacity="0.3" />
            <polygon points="78,23 60,32 60,34 78,25" fill={color} opacity={Math.max(0.15, opacity * 0.45)} />
          </g>
          <g>
            <polygon points="60,10 82,20 60,30 38,20" fill={color} opacity={Math.max(0.2, opacity * 0.72)} />
            <polygon points="38,20 60,30 60,32 38,22" fill="#0a1f12" opacity="0.26" />
            <polygon points="82,20 60,30 60,32 82,22" fill={color} opacity={Math.max(0.14, opacity * 0.38)} />
          </g>
          <g>
            <polygon points="60,6 85,18 60,30 35,18" fill={color} opacity={Math.max(0.18, opacity * 0.58)} />
            <polygon points="35,18 60,30 60,32 35,20" fill="#0a1f12" opacity="0.24" />
            <polygon points="85,18 60,30 60,32 85,20" fill={color} opacity={Math.max(0.12, opacity * 0.32)} />
            <rect x="58" y="4" width="4" height="2" fill="#c4ff62" opacity="0.65" />
          </g>
          <rect x="52" y="17" width="3" height="2" fill="#c4ff62" opacity="0.4" />
        </g>
      </g>
    </PreviewShell>
  );
}

function TrailPreview({ values }: PreviewProps) {
  const color = liveColor(values, "trail");
  const opacity = (values["trail.opacity"] ?? 60) / 100;
  const lifetime = values["trail.lifetime"] ?? 1.5;
  const blocks = [0, 1, 2, 3, 4, 5, 6];

  return (
    <PreviewShell label="Превью эффекта След, блоки смещаются по настройкам">
      <g fill="#2fa960" opacity="0.22">
        <rect x="12" y="42" width="96" height="2" />
        <rect x="24" y="38" width="4" height="4" />
        <rect x="56" y="38" width="4" height="4" />
        <rect x="88" y="38" width="4" height="4" />
      </g>
      <g>
        {blocks.map((block) => (
          <g
            key={block}
            className="trail-block"
            style={{
              animationDelay: `${block * 0.16}s`,
              animationDuration: `${Math.max(1.4, 2.2 + lifetime * 0.35)}s`,
            }}
          >
            <rect x={14 + block * 13} y={31 - block} width="8" height="8" fill="#0a1f12" opacity={0.65} />
            <rect
              x={15 + block * 13}
              y={28 - block}
              width="7"
              height="7"
              fill={color}
              opacity={Math.max(0.12, opacity - block * 0.075)}
            />
            <rect x={15 + block * 13} y={28 - block} width="2" height="2" fill="#c4ff62" opacity="0.55" />
            <rect x={20 + block * 13} y={33 - block} width="2" height="2" fill="#0a1f12" opacity="0.35" />
          </g>
        ))}
      </g>
    </PreviewShell>
  );
}

function JumpRingsPreview({ values }: PreviewProps) {
  const color = liveColor(values, "jumpRings");
  const opacity = (values["jumpRings.opacity"] ?? 60) / 100;
  const radius = values["jumpRings.radius"] ?? 0.7;
  const rings = [
    { size: 9 + radius * 3, delay: 0 },
    { size: 15 + radius * 4, delay: 0.55 },
    { size: 21 + radius * 5, delay: 1.1 },
  ];

  return (
    <PreviewShell label="Превью эффекта Кольца прыжка, анимация расширения">
      <g fill="#2fa960" opacity="0.28">
        <rect x="48" y="45" width="24" height="2" />
        <rect x="42" y="48" width="36" height="2" />
      </g>
      <rect x="57" y="26" width="6" height="3" fill={color} opacity={Math.max(0.45, opacity)} />
      <rect x="55" y="24" width="10" height="2" fill={color} opacity={Math.max(0.3, opacity * 0.7)} />
      {rings.map((ring, index) => {
        const half = ring.size;
        return (
          <g
            key={index}
            className="ring-wave"
            style={{ animationDelay: `${ring.delay}s`, color }}
            opacity={Math.max(0.2, opacity * 0.8)}
          >
            <rect x={60 - half} y={27 - half} width={half * 2} height="2" fill="currentColor" />
            <rect x={60 - half} y={27 + half - 2} width={half * 2} height="2" fill="currentColor" />
            <rect x={60 - half} y={27 - half + 2} width="2" height={half * 2 - 4} fill="currentColor" />
            <rect x={60 + half - 2} y={27 - half + 2} width="2" height={half * 2 - 4} fill="currentColor" />
            <rect x={60 - half + 3} y={27 - half + 2} width="2" height="1" fill="#c4ff62" opacity="0.6" />
            <rect x={60 + half - 5} y={27 + half - 3} width="2" height="1" fill="#0a1f12" opacity="0.5" />
          </g>
        );
      })}
      <g className="ring-core" style={{ color }}>
        <rect x="55" y="15" width="2" height="2" fill="currentColor" opacity="0.6" />
        <rect x="63" y="19" width="1" height="1" fill="currentColor" opacity="0.4" />
        <rect x="51" y="25" width="1" height="1" fill="currentColor" opacity="0.35" />
      </g>
    </PreviewShell>
  );
}

function KatanaPreview({ values }: PreviewProps) {
  const slope = values["katana.tilt"] ?? 0.9;
  const angle = (slope * 180) / Math.PI;
  const scale = values["katana.scale"] ?? 0.7;
  const motionOpacity = values["katana.opacity"] ?? 1;

  return (
    <PreviewShell label="Превью Катаны, наклон и размер меняются по настройкам">
      <g fill="#2fa960" opacity="0.2">
        <rect x="8" y="47" width="104" height="1" />
        <rect x="20" y="44" width="6" height="3" />
        <rect x="94" y="44" width="6" height="3" />
      </g>
      <g transform={`rotate(${angle} 60 28) scale(${0.72 + scale * 0.3}) translate(${(1 - scale) * 10} ${(1 - scale) * 4})`}>
        <g fill="#0a1f12" opacity="0.55">
          <rect x="31" y="31" width="56" height="2" />
          <rect x="38" y="35" width="42" height="1" />
        </g>
        <g opacity={Math.max(0.28, motionOpacity)}>
          <rect x="24" y="27" width="10" height="3" fill="#2fa960" />
          <rect x="25" y="27" width="2" height="1" fill="#c4ff62" />
          <rect x="28" y="29" width="2" height="1" fill="#0a1f12" />
          <rect x="34" y="23" width="3" height="11" fill="#89a792" />
          <rect x="34" y="24" width="1" height="4" fill="#c4ff62" opacity="0.55" />
          <rect x="37" y="26" width="36" height="3" fill="#e9f5ec" />
          <rect x="39" y="26" width="27" height="1" fill="#c4ff62" opacity="0.75" />
          <rect x="70" y="26" width="5" height="2" fill="#e9f5ec" />
          <rect x="75" y="26" width="3" height="1" fill="#e9f5ec" />
          <rect x="37" y="29" width="30" height="1" fill="#89a792" />
          <rect x="47" y="23" width="1" height="1" fill="#c4ff62" opacity="0.5" />
          <rect x="49" y="24" width="1" height="1" fill="#c4ff62" opacity="0.3" />
        </g>
      </g>
    </PreviewShell>
  );
}

function CrownPreview({ values }: PreviewProps) {
  const color = liveColor(values, "crown");
  const size = values["crown.scale"] ?? 1;
  const offset = values["crown.offset"] ?? 0.5;
  const opacity = values["crown.opacity"] ?? 1;
  const scale = Math.max(0.04, size);

  return (
    <PreviewShell label="Превью Короны, размер и цвет меняются по настройкам">
      <g fill="#2fa960" opacity="0.25">
        <rect x="24" y="47" width="72" height="2" />
        <rect x="40" y="44" width="40" height="1" />
      </g>
      <g className="crown-float">
        <g
          opacity={Math.max(0.08, opacity)}
          transform={`translate(60 ${28 + offset * 4}) scale(${scale}) translate(-60 -28)`}
        >
        <g fill={color}>
          <rect x="37" y="28" width="6" height="7" />
          <rect x="44" y="24" width="6" height="11" />
          <rect x="51" y="20" width="7" height="15" />
          <rect x="59" y="24" width="6" height="11" />
          <rect x="67" y="20" width="7" height="15" />
          <rect x="75" y="24" width="6" height="11" />
          <rect x="83" y="28" width="6" height="7" />
          <rect x="36" y="34" width="54" height="7" />
        </g>
        <g fill="#c4ff62" opacity="0.7">
          <rect x="37" y="28" width="2" height="2" />
          <rect x="53" y="20" width="2" height="2" />
          <rect x="69" y="20" width="2" height="2" />
          <rect x="85" y="28" width="2" height="2" />
          <rect x="37" y="34" width="52" height="1" />
        </g>
        <g fill="#0a1f12" opacity="0.72">
          <rect x="43" y="36" width="3" height="3" />
          <rect x="52" y="36" width="3" height="3" />
          <rect x="61" y="36" width="3" height="3" />
          <rect x="70" y="36" width="3" height="3" />
          <rect x="79" y="36" width="3" height="3" />
            <rect x="39" y="30" width="1" height="2" />
            <rect x="89" y="30" width="1" height="2" />
          </g>
        </g>
      </g>
    </PreviewShell>
  );
}

export function EffectPreview({ id, values }: { id: string; values: Record<string, number> }) {
  switch (id) {
    case "chinaHat":
      return <ChinaHatPreview values={values} />;
    case "trail":
      return <TrailPreview values={values} />;
    case "jumpRings":
      return <JumpRingsPreview values={values} />;
    case "katana":
      return <KatanaPreview values={values} />;
    case "crown":
      return <CrownPreview values={values} />;
    default:
      return null;
  }
}
