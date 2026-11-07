import { cn } from "../utils/cn";

/**
 * Code-drawn pixel landscapes. Every shape is a rect on an integer grid,
 * every color comes from the locked theme palette and there are no rasters,
 * labels or decorative frames inside the artwork.
 */
export function AbstractBand({ className }: { className?: string }) {
  return (
    <div className={cn("art-band", className)} aria-hidden="true">
      <svg viewBox="0 0 160 44" preserveAspectRatio="xMidYMid slice" shapeRendering="crispEdges">
        <rect width="160" height="44" fill="#06110b" />

        <g fill="#89a792" opacity="0.38">
          <rect x="9" y="5" width="1" height="1" className="pixel-star" />
          <rect x="22" y="12" width="1" height="1" opacity="0.7" />
          <rect x="38" y="4" width="1" height="1" className="pixel-star" />
          <rect x="52" y="14" width="1" height="1" opacity="0.55" />
          <rect x="72" y="6" width="1" height="1" opacity="0.7" />
          <rect x="86" y="11" width="1" height="1" className="pixel-star" />
          <rect x="103" y="4" width="1" height="1" opacity="0.5" />
          <rect x="118" y="14" width="1" height="1" opacity="0.65" />
          <rect x="146" y="7" width="1" height="1" opacity="0.55" />
          <rect x="154" y="15" width="1" height="1" className="pixel-star" />
        </g>

        <g fill="#c4ff62">
          <rect x="134" y="7" width="8" height="2" />
          <rect x="132" y="9" width="12" height="4" />
          <rect x="134" y="13" width="8" height="2" />
          <rect x="136" y="9" width="4" height="4" fill="#06110b" />
          <rect x="132" y="9" width="2" height="2" fill="#c4ff62" opacity="0.45" />
        </g>

        <g fill="#2fa960">
          <rect x="16" y="20" width="1" height="10" />
          <rect x="13" y="22" width="7" height="2" />
          <rect x="14" y="18" width="5" height="2" />
          <rect x="15" y="16" width="3" height="2" />
          <rect x="12" y="26" width="9" height="2" />
          <rect x="10" y="30" width="13" height="2" />
          <rect x="28" y="26" width="1" height="8" />
          <rect x="25" y="27" width="7" height="2" />
          <rect x="26" y="24" width="5" height="2" />
          <rect x="27" y="22" width="3" height="2" />
          <rect x="23" y="30" width="11" height="2" />
          <rect x="48" y="22" width="1" height="12" />
          <rect x="45" y="24" width="7" height="2" />
          <rect x="46" y="20" width="5" height="2" />
          <rect x="47" y="18" width="3" height="2" />
          <rect x="43" y="28" width="11" height="2" />
          <rect x="41" y="32" width="15" height="2" />
        </g>

        <g fill="#6ef58c" opacity="0.82">
          <rect x="15" y="16" width="1" height="1" />
          <rect x="14" y="18" width="2" height="1" />
          <rect x="13" y="22" width="2" height="1" />
          <rect x="27" y="22" width="1" height="1" />
          <rect x="26" y="24" width="2" height="1" />
          <rect x="47" y="18" width="1" height="1" />
          <rect x="46" y="20" width="2" height="1" />
          <rect x="45" y="24" width="2" height="1" />
        </g>

        <g fill="#101e15">
          <rect x="0" y="32" width="160" height="12" />
          <rect x="0" y="30" width="22" height="3" />
          <rect x="28" y="31" width="16" height="2" />
          <rect x="50" y="30" width="26" height="3" />
          <rect x="82" y="31" width="14" height="2" />
          <rect x="100" y="29" width="28" height="4" />
          <rect x="134" y="31" width="26" height="2" />
        </g>

        <g fill="#2fa960" opacity="0.78">
          <rect x="0" y="30" width="22" height="2" />
          <rect x="28" y="31" width="16" height="1" />
          <rect x="50" y="30" width="26" height="1" />
          <rect x="82" y="31" width="14" height="1" />
          <rect x="100" y="29" width="28" height="1" />
          <rect x="134" y="31" width="26" height="1" />
        </g>

        <g fill="#0a1f12">
          <rect x="24" y="36" width="6" height="2" />
          <rect x="39" y="34" width="3" height="1" />
          <rect x="59" y="37" width="10" height="1" />
          <rect x="91" y="35" width="4" height="2" />
          <rect x="112" y="37" width="8" height="1" />
          <rect x="142" y="34" width="5" height="2" />
          <rect x="72" y="32" width="2" height="1" />
          <rect x="130" y="33" width="2" height="1" />
        </g>

        <g fill="#6ef58c" opacity="0.55">
          <rect x="52" y="28" width="1" height="1" />
          <rect x="57" y="25" width="1" height="1" />
          <rect x="61" y="22" width="1" height="1" opacity="0.4" />
          <rect x="68" y="26" width="1" height="1" opacity="0.3" />
        </g>

        <g fill="#89a792" opacity="0.5">
          <rect x="75" y="35" width="3" height="1" />
          <rect x="80" y="35" width="1" height="1" />
          <rect x="126" y="34" width="2" height="1" />
          <rect x="120" y="34" width="1" height="1" />
        </g>
      </svg>
    </div>
  );
}

export function ClosedBackdrop() {
  return (
    <div className="closed-art" aria-hidden="true">
      <svg viewBox="0 0 240 140" preserveAspectRatio="xMidYMid slice" shapeRendering="crispEdges">
        <rect width="240" height="140" fill="#040a06" />
        <rect y="86" width="240" height="54" fill="#06110b" />

        <g fill="#89a792" opacity="0.42">
          <rect x="14" y="12" width="1" height="1" className="pixel-star" />
          <rect x="28" y="28" width="1" height="1" opacity="0.7" />
          <rect x="46" y="10" width="1" height="1" className="pixel-star" />
          <rect x="62" y="34" width="1" height="1" opacity="0.6" />
          <rect x="84" y="16" width="1" height="1" opacity="0.8" />
          <rect x="103" y="30" width="1" height="1" className="pixel-star" />
          <rect x="126" y="8" width="1" height="1" opacity="0.55" />
          <rect x="148" y="22" width="1" height="1" opacity="0.65" />
          <rect x="166" y="38" width="1" height="1" className="pixel-star" />
          <rect x="188" y="12" width="1" height="1" opacity="0.5" />
          <rect x="206" y="30" width="1" height="1" opacity="0.7" />
          <rect x="226" y="16" width="1" height="1" className="pixel-star" />
          <rect x="36" y="52" width="1" height="1" opacity="0.4" />
          <rect x="196" y="54" width="1" height="1" opacity="0.45" />
        </g>

        <g fill="#c4ff62" opacity="0.8">
          <rect x="186" y="16" width="12" height="3" />
          <rect x="183" y="19" width="18" height="7" />
          <rect x="186" y="26" width="12" height="3" />
          <rect x="190" y="19" width="7" height="7" fill="#040a06" />
          <rect x="183" y="19" width="3" height="3" fill="#c4ff62" opacity="0.4" />
        </g>

        <g fill="#0a1f12">
          <rect x="0" y="86" width="240" height="54" />
          <rect x="0" y="82" width="34" height="6" />
          <rect x="40" y="84" width="22" height="4" />
          <rect x="68" y="80" width="38" height="8" />
          <rect x="114" y="83" width="20" height="5" />
          <rect x="140" y="78" width="42" height="10" />
          <rect x="190" y="82" width="24" height="6" />
          <rect x="218" y="80" width="22" height="8" />
        </g>

        <g fill="#2fa960" opacity="0.7">
          <rect x="0" y="82" width="34" height="2" />
          <rect x="68" y="80" width="38" height="2" />
          <rect x="140" y="78" width="42" height="2" />
          <rect x="218" y="80" width="22" height="2" />
        </g>

        <g fill="#2fa960">
          <rect x="24" y="52" width="2" height="32" />
          <rect x="16" y="58" width="18" height="4" />
          <rect x="18" y="50" width="14" height="4" />
          <rect x="20" y="46" width="10" height="4" />
          <rect x="14" y="66" width="22" height="4" />
          <rect x="10" y="74" width="30" height="4" />
          <rect x="56" y="62" width="2" height="22" />
          <rect x="50" y="66" width="14" height="3" />
          <rect x="52" y="60" width="10" height="3" />
          <rect x="48" y="74" width="18" height="3" />
          <rect x="204" y="56" width="2" height="28" />
          <rect x="198" y="60" width="14" height="3" />
          <rect x="200" y="54" width="10" height="3" />
          <rect x="194" y="72" width="22" height="4" />
        </g>

        <g fill="#6ef58c" opacity="0.65">
          <rect x="20" y="46" width="3" height="2" />
          <rect x="18" y="50" width="3" height="2" />
          <rect x="52" y="60" width="2" height="2" />
          <rect x="50" y="66" width="2" height="2" />
          <rect x="200" y="54" width="2" height="2" />
          <rect x="198" y="60" width="2" height="2" />
        </g>

        <g fill="#101e15">
          <rect x="0" y="96" width="240" height="2" />
          <rect x="34" y="92" width="8" height="2" />
          <rect x="112" y="94" width="14" height="2" />
          <rect x="178" y="92" width="10" height="2" />
          <rect x="62" y="104" width="18" height="2" />
          <rect x="150" y="108" width="22" height="2" />
          <rect x="90" y="118" width="16" height="2" />
        </g>

        <g fill="#2fa960" opacity="0.35">
          <rect x="40" y="94" width="2" height="2" />
          <rect x="120" y="96" width="2" height="2" />
          <rect x="180" y="94" width="2" height="2" />
          <rect x="66" y="106" width="2" height="2" />
          <rect x="154" y="110" width="2" height="2" />
          <rect x="94" y="120" width="2" height="2" />
        </g>
      </svg>
    </div>
  );
}
