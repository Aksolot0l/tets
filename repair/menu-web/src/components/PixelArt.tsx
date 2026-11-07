import type { CSSProperties } from "react";

type ArtProps = {
  className?: string;
  style?: CSSProperties;
};

export function PixelLogo({ className, style }: ArtProps) {
  return (
    <svg
      className={className}
      style={style}
      viewBox="0 0 8 8"
      width="100%"
      height="100%"
      shapeRendering="crispEdges"
      role="img"
      aria-label="GAYClient"
      focusable="false"
    >
      <rect width="8" height="8" fill="#0a1f12" />
      <g fill="currentColor">
        <rect x="1" y="0" width="6" height="1" />
        <rect x="0" y="1" width="1" height="6" />
        <rect x="7" y="1" width="1" height="1" />
        <rect x="4" y="3" width="4" height="2" />
        <rect x="7" y="5" width="1" height="2" />
        <rect x="1" y="7" width="6" height="1" />
      </g>
      <g fill="#c4ff62">
        <rect x="1" y="0" width="1" height="1" />
        <rect x="0" y="1" width="1" height="1" />
      </g>
    </svg>
  );
}

export function PixelAvatar({ className, style }: ArtProps) {
  return (
    <svg
      className={className}
      style={style}
      viewBox="0 0 16 16"
      width="64"
      height="64"
      shapeRendering="crispEdges"
      role="img"
      aria-label="Пиксельный аватар"
      focusable="false"
    >
      <rect x="6" y="0" width="4" height="1" fill="#c4ff62" />
      <rect x="5" y="1" width="6" height="1" fill="#6ef58c" />
      <rect x="3" y="2" width="10" height="1" fill="#2fa960" />
      <rect x="2" y="3" width="12" height="1" fill="#2fa960" />
      <rect x="1" y="4" width="14" height="8" fill="#2fa960" />
      <rect x="2" y="12" width="12" height="2" fill="#2fa960" />
      <rect x="3" y="14" width="10" height="1" fill="#0a1f12" />
      <rect x="4" y="15" width="8" height="1" fill="#0a1f12" />
      <rect x="2" y="4" width="3" height="1" fill="#6ef58c" />
      <rect x="1" y="5" width="2" height="3" fill="#6ef58c" />
      <rect x="2" y="5" width="12" height="5" fill="#0a1f12" />
      <rect x="13" y="5" width="2" height="3" fill="#6ef58c" />
      <rect x="11" y="4" width="3" height="1" fill="#c4ff62" />
      <rect x="4" y="6" width="2" height="2" fill="#6ef58c" />
      <rect x="4" y="6" width="1" height="1" fill="#e9f5ec" />
      <rect x="10" y="6" width="2" height="2" fill="#6ef58c" />
      <rect x="10" y="6" width="1" height="1" fill="#e9f5ec" />
      <rect x="2" y="10" width="12" height="1" fill="#101e15" />
      <rect x="5" y="12" width="2" height="1" fill="#6ef58c" />
      <rect x="9" y="12" width="2" height="1" fill="#6ef58c" />
      <rect x="6" y="13" width="4" height="1" fill="#0a1f12" />
      <rect x="0" y="7" width="1" height="2" fill="#c4ff62" />
      <rect x="15" y="7" width="1" height="2" fill="#c4ff62" />
    </svg>
  );
}

export function PixelAxolotl({ className, style }: ArtProps) {
  return (
    <svg
      className={className}
      style={style}
      viewBox="0 0 26 20"
      width="120"
      height="92"
      shapeRendering="crispEdges"
      role="img"
      aria-label="Пиксельный аксолотль"
      focusable="false"
    >
      {/* tail */}
      <g fill="#2fa960">
        <rect x="0" y="9" width="5" height="4" />
        <rect x="1" y="8" width="3" height="1" />
        <rect x="1" y="13" width="3" height="1" />
        <rect x="4" y="10" width="2" height="2" />
      </g>
      <g fill="#6ef58c">
        <rect x="1" y="9" width="4" height="3" />
        <rect x="2" y="10" width="3" height="1" />
      </g>
      <rect x="0" y="10" width="1" height="1" fill="#c4ff62" />

      {/* body */}
      <g fill="#2fa960">
        <rect x="5" y="6" width="12" height="10" />
        <rect x="6" y="5" width="9" height="1" />
        <rect x="5" y="16" width="3" height="2" />
        <rect x="11" y="16" width="3" height="2" />
      </g>
      <g fill="#6ef58c">
        <rect x="6" y="7" width="10" height="8" />
        <rect x="7" y="6" width="8" height="1" />
        <rect x="6" y="15" width="9" height="1" />
        <rect x="5" y="17" width="3" height="1" />
        <rect x="11" y="17" width="3" height="1" />
      </g>
      <g fill="#c4ff62">
        <rect x="6" y="7" width="6" height="1" />
        <rect x="6" y="8" width="2" height="1" />
      </g>

      {/* belly stripe */}
      <rect x="8" y="12" width="6" height="3" fill="#89a792" opacity="0.55" />
      <rect x="9" y="13" width="4" height="1" fill="#e9f5ec" opacity="0.25" />

      {/* head */}
      <g fill="#2fa960">
        <rect x="14" y="5" width="8" height="9" />
        <rect x="16" y="4" width="5" height="1" />
        <rect x="22" y="7" width="2" height="5" />
        <rect x="14" y="14" width="6" height="1" />
      </g>
      <g fill="#6ef58c">
        <rect x="15" y="6" width="7" height="7" />
        <rect x="17" y="5" width="4" height="1" />
        <rect x="22" y="8" width="1" height="3" />
        <rect x="15" y="13" width="5" height="1" />
      </g>
      <g fill="#c4ff62">
        <rect x="17" y="5" width="3" height="1" />
        <rect x="15" y="6" width="3" height="1" />
        <rect x="22" y="8" width="1" height="1" />
      </g>

      {/* gills left and right, three branches each */}
      <g fill="#2fa960">
        <rect x="12" y="2" width="2" height="4" />
        <rect x="13" y="1" width="1" height="1" />
        <rect x="11" y="3" width="1" height="1" />
        <rect x="12" y="5" width="1" height="1" />
        <rect x="22" y="2" width="2" height="4" />
        <rect x="22" y="1" width="1" height="1" />
        <rect x="24" y="3" width="1" height="1" />
        <rect x="23" y="5" width="1" height="1" />
      </g>
      <g fill="#c4ff62">
        <rect x="12" y="2" width="1" height="2" />
        <rect x="13" y="1" width="1" height="1" />
        <rect x="22" y="2" width="1" height="2" />
        <rect x="22" y="1" width="1" height="1" />
      </g>
      <g fill="#6ef58c">
        <rect x="11" y="3" width="1" height="1" />
        <rect x="12" y="4" width="1" height="1" />
        <rect x="24" y="3" width="1" height="1" />
        <rect x="23" y="4" width="1" height="1" />
      </g>

      {/* face */}
      <rect x="18" y="7" width="2" height="2" fill="#0a1f12" />
      <rect x="18" y="7" width="1" height="1" fill="#e9f5ec" />
      <rect x="21" y="7" width="1" height="2" fill="#0a1f12" opacity="0.75" />
      <rect x="18" y="11" width="4" height="1" fill="#0a1f12" opacity="0.65" />
      <rect x="19" y="12" width="2" height="1" fill="#0a1f12" opacity="0.4" />
      <rect x="17" y="10" width="1" height="1" fill="#2fa960" />
      <rect x="22" y="10" width="1" height="1" fill="#2fa960" />

      {/* toes and ground shadow */}
      <g fill="#0a1f12" opacity="0.85">
        <rect x="5" y="18" width="1" height="1" />
        <rect x="7" y="18" width="1" height="1" />
        <rect x="11" y="18" width="1" height="1" />
        <rect x="13" y="18" width="1" height="1" />
      </g>
      <rect x="2" y="19" width="18" height="1" fill="#2fa960" opacity="0.22" />
    </svg>
  );
}

export function PixelChest({ className, style }: ArtProps) {
  return (
    <svg
      className={className}
      style={style}
      viewBox="0 0 16 16"
      width="80"
      height="80"
      shapeRendering="crispEdges"
      role="img"
      aria-label="Пустой сундук"
      focusable="false"
    >
      <rect x="4" y="2" width="8" height="1" fill="#101e15" />
      <rect x="3" y="3" width="10" height="1" fill="#89a792" />
      <rect x="2" y="4" width="12" height="1" fill="#89a792" />
      <rect x="2" y="5" width="12" height="3" fill="#89a792" />
      <rect x="2" y="6" width="12" height="1" fill="#0a1f12" opacity="0.55" />
      <rect x="2" y="8" width="12" height="6" fill="#89a792" />
      <rect x="3" y="9" width="10" height="4" fill="#0a1f12" opacity="0.45" />
      <rect x="3" y="8" width="10" height="1" fill="#101e15" />
      <rect x="2" y="13" width="12" height="1" fill="#101e15" />
      <rect x="7" y="6" width="2" height="4" fill="#2fa960" />
      <rect x="7" y="6" width="2" height="1" fill="#c4ff62" opacity="0.75" />
      <rect x="7" y="8" width="1" height="1" fill="#0a1f12" />
      <rect x="4" y="5" width="2" height="1" fill="#c4ff62" opacity="0.3" />
      <rect x="11" y="10" width="2" height="2" fill="#0a1f12" opacity="0.4" />
      <rect x="1" y="5" width="1" height="8" fill="#101e15" />
      <rect x="14" y="5" width="1" height="8" fill="#101e15" />
      <rect x="4" y="14" width="2" height="1" fill="#101e15" />
      <rect x="10" y="14" width="2" height="1" fill="#101e15" />
    </svg>
  );
}
