// Bridge to the real in-game menu: a localhost WebSocket to the adapter's
// bridge server (menu_rml.cpp, 127.0.0.1:39731). While the game is running,
// toggles and sliders here drive the exact same verified setters the RmlUi
// in-game menu uses. Without the game the UI stays in local demo mode.

export type BridgeStatus = "demo" | "connecting" | "connected";

export type BridgeSnapshot = {
  switches: Record<string, boolean>;
  sliders: Record<string, number>;
};

// web id -> game element id (kSwitches / kSliders in menu_rml.cpp)
const SWITCH_IDS: Record<string, string> = {
  fps: "sw-fps",
  cps: "sw-cps",
  clock: "sw-clock",
  keys: "sw-keys",
  watermark: "sw-watermark",
  coords: "sw-coords",
  resolution: "sw-res",
  performance: "sw-perf",
  moduleList: "sw-modlist",
  chinaHat: "sw-chinahat",
  trail: "sw-trail",
  jumpRings: "sw-jump",
  katana: "sw-katana",
  crown: "sw-crown",
  "chinaHat.enabled": "sw-gfx-chinahat",
  "chinaHat.fill": "sw-ch-fill",
  "chinaHat.rainbow": "sw-ch-rainbow",
  "trail.enabled": "sw-gfx-trail",
  "trail.fill": "sw-tr-fill",
  "trail.rainbow": "sw-tr-rainbow",
  "jumpRings.enabled": "sw-gfx-jump",
  "jumpRings.rainbow": "sw-jr-rainbow",
};

const SLIDER_IDS: Record<string, string> = {
  "chinaHat.rainbowSpeed": "sl-ch-rainbow",
  "chinaHat.opacity": "sl-ch-opacity",
  "chinaHat.radius": "sl-ch-radius",
  "chinaHat.height": "sl-ch-height",
  "chinaHat.headOffset": "sl-ch-offset",
  "chinaHat.floatAmplitude": "sl-ch-amp",
  "chinaHat.floatSpeed": "sl-ch-floatspeed",
  "chinaHat.colorR": "sl-ch-r",
  "chinaHat.colorG": "sl-ch-g",
  "chinaHat.colorB": "sl-ch-b",
  "trail.rainbowSpeed": "sl-tr-rainbow",
  "trail.opacity": "sl-tr-opacity",
  "trail.height": "sl-tr-height",
  "trail.lifetime": "sl-tr-life",
  "trail.colorR": "sl-tr-r",
  "trail.colorG": "sl-tr-g",
  "trail.colorB": "sl-tr-b",
  "jumpRings.rainbowSpeed": "sl-jr-rainbow",
  "jumpRings.radius": "sl-jr-radius",
  "jumpRings.opacity": "sl-jr-opacity",
  "jumpRings.duration": "sl-jr-duration",
  "jumpRings.colorR": "sl-jr-r",
  "jumpRings.colorG": "sl-jr-g",
  "jumpRings.colorB": "sl-jr-b",
  "katana.scale": "sl-kt-scale",
  "katana.tilt": "sl-kt-tilt",
  "katana.side": "sl-kt-side",
  "katana.handHeight": "sl-kt-height",
  "katana.forward": "sl-kt-forward",
  "katana.opacity": "sl-kt-opacity",
  "katana.firstPersonOffset": "sl-kt-fpside",
  "katana.firstPersonForward": "sl-kt-fpforward",
  "crown.scale": "sl-cr-scale",
  "crown.offset": "sl-cr-offset",
  "crown.opacity": "sl-cr-opacity",
  "crown.colorR": "sl-cr-r",
  "crown.colorG": "sl-cr-g",
  "crown.colorB": "sl-cr-b",
};

const REVERSE_SWITCH: Record<string, string> = {};
for (const [web, game] of Object.entries(SWITCH_IDS)) REVERSE_SWITCH[game] = web;
const REVERSE_SLIDER: Record<string, string> = {};
for (const [web, game] of Object.entries(SLIDER_IDS)) REVERSE_SLIDER[game] = web;

export type GameBridge = {
  sendSwitch: (webId: string, on: boolean) => void;
  sendSlider: (webId: string, value: number) => void;
};

export function connectBridge(
  onSnapshot: (snapshot: BridgeSnapshot) => void,
  onStatus: (status: BridgeStatus) => void,
): GameBridge {
  let socket: WebSocket | null = null;
  let retryTimer: number | null = null;
  let disposed = false;

  const scheduleReconnect = () => {
    if (disposed || retryTimer !== null) return;
    retryTimer = window.setTimeout(() => {
      retryTimer = null;
      if (!disposed && document.visibilityState === "visible") connect();
    }, 2500);
  };

  const connect = () => {
    if (disposed) return;
    onStatus("connecting");
    try {
      socket = new WebSocket("ws://127.0.0.1:39731/");
    } catch {
      onStatus("demo");
      scheduleReconnect();
      return;
    }
    socket.onopen = () => {
      onStatus("connected");
      socket?.send("state\n");
    };
    socket.onmessage = (event) => {
      if (typeof event.data !== "string") return;
      const snapshot: BridgeSnapshot = { switches: {}, sliders: {} };
      for (const raw of event.data.split("\n")) {
        const line = raw.trim();
        if (line.startsWith("sw ")) {
          const parts = line.split(" ");
          const webId = REVERSE_SWITCH[parts[1]];
          if (webId) snapshot.switches[webId] = parts[2] === "1";
        } else if (line.startsWith("sl ")) {
          const parts = line.split(" ");
          const webId = REVERSE_SLIDER[parts[1]];
          const value = Number.parseFloat(parts[2]);
          if (webId && Number.isFinite(value)) snapshot.sliders[webId] = value;
        }
      }
      if (Object.keys(snapshot.switches).length || Object.keys(snapshot.sliders).length)
        onSnapshot(snapshot);
    };
    socket.onclose = () => {
      onStatus("demo");
      scheduleReconnect();
    };
    socket.onerror = () => {
      /* onclose follows */
    };
  };

  const sendLine = (line: string) => {
    if (socket && socket.readyState === WebSocket.OPEN) socket.send(line);
  };

  connect();

  return {
    sendSwitch: (webId, on) => {
      const game = SWITCH_IDS[webId];
      if (game) sendLine(`set ${game} ${on ? 1 : 0}\n`);
    },
    sendSlider: (webId, value) => {
      const game = SLIDER_IDS[webId];
      if (game) sendLine(`set ${game} ${Number.isFinite(value) ? value : 0}\n`);
    },
  };
}
