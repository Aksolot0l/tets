export type TabId = "modules" | "graphics" | "hud" | "info";

export type AccentTheme = {
  id: string;
  label: string;
  accent: string;
  strong: string;
  deep: string;
  rgb: string;
};

export const ACCENT_THEMES: AccentTheme[] = [
  { id: "emerald", label: "Изумруд", accent: "#6ef58c", strong: "#c4ff62", deep: "#2fa960", rgb: "110, 245, 140" },
  { id: "lime", label: "Лайм", accent: "#a3e635", strong: "#d9f99d", deep: "#65a30d", rgb: "163, 230, 53" },
  { id: "mint", label: "Мята", accent: "#34d399", strong: "#a7f3d0", deep: "#059669", rgb: "52, 211, 153" },
  { id: "cyan", label: "Циан", accent: "#22d3ee", strong: "#a5f3fc", deep: "#0891b2", rgb: "34, 211, 238" },
  { id: "amber", label: "Янтарь", accent: "#fbbf24", strong: "#fde68a", deep: "#d97706", rgb: "251, 191, 36" },
];

export type ModuleGroup = "hud" | "effects";

export type ModuleItem = {
  id: string;
  group: ModuleGroup;
  name: string;
  description: string;
};

export const MODULES: ModuleItem[] = [
  { id: "fps", group: "hud", name: "FPS", description: "Счётчик кадров в секунду" },
  { id: "cps", group: "hud", name: "CPS", description: "Клики в секунду" },
  { id: "clock", group: "hud", name: "Часы", description: "Текущее время" },
  { id: "keys", group: "hud", name: "Клавиши", description: "Отображение нажатых клавиш" },
  { id: "watermark", group: "hud", name: "Водяной знак", description: "Надпись GAYClient на экране" },
  { id: "coords", group: "hud", name: "Координаты", description: "Позиция в мире" },
  { id: "resolution", group: "hud", name: "Разрешение", description: "Размер окна игры" },
  { id: "performance", group: "hud", name: "Производительность", description: "Загрузка CPU и RAM" },
  { id: "moduleList", group: "hud", name: "Список модулей", description: "Активные модули на экране" },
  { id: "chinaHat", group: "effects", name: "China Hat", description: "Конус над головой" },
  { id: "trail", group: "effects", name: "След", description: "Цветной след за игроком" },
  { id: "jumpRings", group: "effects", name: "Кольца прыжка", description: "Круги при прыжке" },
  { id: "katana", group: "effects", name: "Катана", description: "Замена алмазного меча" },
  { id: "crown", group: "effects", name: "Корона", description: "3D-корона над головой" },
];

export const HUD_MODULES = MODULES.filter((item) => item.group === "hud");
export const EFFECT_MODULES = MODULES.filter((item) => item.group === "effects");

export type ToggleDef = {
  id: string;
  label: string;
  defaultValue: boolean;
};

export type SliderDef = {
  id: string;
  label: string;
  short?: string;
  min: number;
  max: number;
  step: number;
  defaultValue: number;
};

export type GraphicsGroup = {
  id: string;
  title: string;
  symbol: string;
  description: string;
  toggles: ToggleDef[];
  sliders: SliderDef[];
  colorIds: [string, string, string];
};

export const GRAPHICS_GROUPS: GraphicsGroup[] = [
  {
    id: "chinaHat",
    title: "CHINA HAT",
    symbol: "▼",
    description: "Конус над головой",
    colorIds: ["chinaHat.colorR", "chinaHat.colorG", "chinaHat.colorB"],
    toggles: [
      { id: "chinaHat.enabled", label: "Включение эффекта", defaultValue: true },
      { id: "chinaHat.fill", label: "Заливка конуса", defaultValue: true },
      { id: "chinaHat.rainbow", label: "Радужный режим", defaultValue: false },
    ],
    sliders: [
      { id: "chinaHat.rainbowSpeed", label: "Скорость радуги", min: 0, max: 100, step: 1, defaultValue: 30 },
      { id: "chinaHat.opacity", label: "Прозрачность", min: 0, max: 100, step: 1, defaultValue: 70 },
      { id: "chinaHat.radius", label: "Радиус", min: 0.2, max: 3, step: 0.01, defaultValue: 0.85 },
      { id: "chinaHat.height", label: "Высота конуса", min: 0.2, max: 3, step: 0.01, defaultValue: 0.6 },
      { id: "chinaHat.headOffset", label: "Смещение над головой", min: 0, max: 2, step: 0.01, defaultValue: 0.1 },
      { id: "chinaHat.floatAmplitude", label: "Амплитуда плавания", min: 0, max: 1, step: 0.01, defaultValue: 0.05 },
      { id: "chinaHat.floatSpeed", label: "Скорость плавания", min: 0, max: 3, step: 0.01, defaultValue: 1 },
      { id: "chinaHat.colorR", label: "Цвет: красный", min: 0, max: 1, step: 0.01, defaultValue: 0.16 },
      { id: "chinaHat.colorG", label: "Цвет: зелёный", min: 0, max: 1, step: 0.01, defaultValue: 1 },
      { id: "chinaHat.colorB", label: "Цвет: синий", min: 0, max: 1, step: 0.01, defaultValue: 0.21 },
    ],
  },
  {
    id: "trail",
    title: "СЛЕД",
    symbol: "≈",
    description: "Цветной след за игроком",
    colorIds: ["trail.colorR", "trail.colorG", "trail.colorB"],
    toggles: [
      { id: "trail.enabled", label: "Включение эффекта", defaultValue: false },
      { id: "trail.fill", label: "Заливка", defaultValue: false },
      { id: "trail.rainbow", label: "Радужный режим", defaultValue: false },
    ],
    sliders: [
      { id: "trail.rainbowSpeed", label: "Скорость радуги", min: 0, max: 100, step: 1, defaultValue: 30 },
      { id: "trail.opacity", label: "Прозрачность", min: 0, max: 100, step: 1, defaultValue: 60 },
      { id: "trail.height", label: "Высота следа", min: 0.2, max: 3, step: 0.01, defaultValue: 1 },
      { id: "trail.lifetime", label: "Время жизни следа", min: 0.2, max: 4, step: 0.05, defaultValue: 1.5 },
      { id: "trail.colorR", label: "Цвет: красный", min: 0, max: 1, step: 0.01, defaultValue: 0.16 },
      { id: "trail.colorG", label: "Цвет: зелёный", min: 0, max: 1, step: 0.01, defaultValue: 1 },
      { id: "trail.colorB", label: "Цвет: синий", min: 0, max: 1, step: 0.01, defaultValue: 0.21 },
    ],
  },
  {
    id: "jumpRings",
    title: "КОЛЬЦА ПРЫЖКА",
    symbol: "O",
    description: "Круги при прыжке",
    colorIds: ["jumpRings.colorR", "jumpRings.colorG", "jumpRings.colorB"],
    toggles: [
      { id: "jumpRings.enabled", label: "Включение", defaultValue: false },
      { id: "jumpRings.rainbow", label: "Радужный режим", defaultValue: false },
    ],
    sliders: [
      { id: "jumpRings.rainbowSpeed", label: "Скорость радуги", min: 0, max: 100, step: 1, defaultValue: 30 },
      { id: "jumpRings.radius", label: "Радиус", min: 0.2, max: 3, step: 0.01, defaultValue: 0.7 },
      { id: "jumpRings.opacity", label: "Прозрачность", min: 0, max: 100, step: 1, defaultValue: 60 },
      { id: "jumpRings.duration", label: "Длительность", min: 0.1, max: 3, step: 0.05, defaultValue: 0.8 },
      { id: "jumpRings.colorR", label: "Цвет: красный", min: 0, max: 1, step: 0.01, defaultValue: 0.16 },
      { id: "jumpRings.colorG", label: "Цвет: зелёный", min: 0, max: 1, step: 0.01, defaultValue: 1 },
      { id: "jumpRings.colorB", label: "Цвет: синий", min: 0, max: 1, step: 0.01, defaultValue: 0.21 },
    ],
  },
  {
    id: "katana",
    title: "КАТАНА",
    symbol: "⚔",
    description: "Замена алмазного меча",
    colorIds: ["chinaHat.colorR", "chinaHat.colorG", "chinaHat.colorB"],
    toggles: [],
    sliders: [
      { id: "katana.scale", label: "Размер", short: "Размер", min: 0.4, max: 1, step: 0.01, defaultValue: 0.7 },
      { id: "katana.tilt", label: "Наклон клинка", short: "Наклон", min: 0.2, max: 1.55, step: 0.01, defaultValue: 0.9 },
      { id: "katana.side", label: "Смещение в сторону", short: "Сторона", min: 0, max: 0.8, step: 0.01, defaultValue: 0.25 },
      { id: "katana.handHeight", label: "Высота в руке", short: "Высота", min: 0.6, max: 1.8, step: 0.01, defaultValue: 1.1 },
      { id: "katana.forward", label: "Смещение вперёд", short: "Вперёд", min: -0.5, max: 1, step: 0.01, defaultValue: 0.2 },
      { id: "katana.opacity", label: "Непрозрачность", short: "Непрозр.", min: 0.3, max: 1, step: 0.01, defaultValue: 1 },
      { id: "katana.firstPersonOffset", label: "Отведение в 1-м лице", min: 0, max: 0.8, step: 0.01, defaultValue: 0.15 },
      { id: "katana.firstPersonForward", label: "Смещение вперёд в 1-м лице", min: -0.5, max: 1, step: 0.01, defaultValue: 0.3 },
    ],
  },
  {
    id: "crown",
    title: "КОРОНА",
    symbol: "★",
    description: "3D-корона над головой",
    colorIds: ["crown.colorR", "crown.colorG", "crown.colorB"],
    toggles: [],
    sliders: [
      { id: "crown.scale", label: "Размер", short: "Размер", min: 0, max: 2, step: 0.01, defaultValue: 1 },
      { id: "crown.offset", label: "Смещение", short: "Смещение", min: 0, max: 2, step: 0.01, defaultValue: 0.5 },
      { id: "crown.opacity", label: "Непрозрачность", short: "Непрозр.", min: 0, max: 1, step: 0.01, defaultValue: 1 },
      { id: "crown.colorR", label: "Цвет: красный", short: "Красный", min: 0, max: 1, step: 0.01, defaultValue: 1 },
      { id: "crown.colorG", label: "Цвет: зелёный", short: "Зелёный", min: 0, max: 1, step: 0.01, defaultValue: 0.8 },
      { id: "crown.colorB", label: "Цвет: синий", short: "Синий", min: 0, max: 1, step: 0.01, defaultValue: 0.2 },
    ],
  },
];

export const ALL_SLIDERS: SliderDef[] = GRAPHICS_GROUPS.flatMap((group) => group.sliders);
export const ALL_GRAPHICS_TOGGLES: ToggleDef[] = GRAPHICS_GROUPS.flatMap((group) => group.toggles);

export const MODULE_ACCENTS: Record<string, string> = {
  fps: "#6ef58c",
  cps: "#c4ff62",
  clock: "#fbbf24",
  keys: "#89a792",
  watermark: "#c4ff62",
  coords: "#22d3ee",
  resolution: "#22d3ee",
  performance: "#fbbf24",
  moduleList: "#6ef58c",
  chinaHat: "#6ef58c",
  trail: "#22d3ee",
  jumpRings: "#22d3ee",
  katana: "#e9f5ec",
  crown: "#fbbf24",
};

export const DEFAULT_MODULES: Record<string, boolean> = Object.fromEntries(
  MODULES.map((item) => [item.id, item.id === "fps" || item.id === "chinaHat"]),
);

export const DEFAULT_GRAPHICS_VALUES: Record<string, number> = Object.fromEntries(
  ALL_SLIDERS.map((slider) => [slider.id, slider.defaultValue]),
);

export const DEFAULT_GRAPHICS_TOGGLES: Record<string, boolean> = Object.fromEntries(
  ALL_GRAPHICS_TOGGLES.map((toggle) => [toggle.id, toggle.defaultValue]),
);

export const DEFAULT_COLLAPSED: Record<string, boolean> = {
  "modules.hud": false,
  "modules.effects": false,
  "graphics.chinaHat": false,
  "graphics.trail": true,
  "graphics.jumpRings": true,
  "graphics.katana": true,
  "graphics.crown": true,
  "hud.modules": false,
};

export type TestToolGroup = {
  id: string;
  title: string;
  sliderIds: string[];
};

export const TEST_TOOL_GROUPS: TestToolGroup[] = [
  {
    id: "katana",
    title: "КАТАНА",
    sliderIds: ["katana.scale", "katana.tilt", "katana.side", "katana.handHeight", "katana.forward", "katana.opacity"],
  },
  {
    id: "crown",
    title: "КОРОНА",
    sliderIds: ["crown.scale", "crown.offset", "crown.opacity", "crown.colorR", "crown.colorG", "crown.colorB"],
  },
];

export function findSlider(id: string): SliderDef {
  const slider = ALL_SLIDERS.find((item) => item.id === id);
  if (!slider) {
    throw new Error(`Unknown slider: ${id}`);
  }
  return slider;
}

export function formatSliderValue(slider: SliderDef, value: number): string {
  return slider.step < 1 ? value.toFixed(2) : String(Math.round(value));
}

export function groupColor(values: Record<string, number>, group: GraphicsGroup): string {
  const r = Math.round((values[group.colorIds[0]] ?? 0) * 255);
  const g = Math.round((values[group.colorIds[1]] ?? 0) * 255);
  const b = Math.round((values[group.colorIds[2]] ?? 0) * 255);
  return `rgb(${r}, ${g}, ${b})`;
}

export const NAV_ITEMS: { id: TabId; label: string; icon: "modules" | "graphics" | "hud" | "info"; hotkey: string }[] = [
  { id: "modules", label: "Модули", icon: "modules", hotkey: "1" },
  { id: "graphics", label: "Графика", icon: "graphics", hotkey: "2" },
  { id: "hud", label: "HUD", icon: "hud", hotkey: "3" },
  { id: "info", label: "Инфо", icon: "info", hotkey: "4" },
];
