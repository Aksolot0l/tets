import {
  ChartNoAxesColumn,
  Home,
  Puzzle,
  Settings2,
  UserRound,
  Wifi,
} from "lucide-react";
import type { Page } from "../lib/types";
import { cn } from "../utils/cn";
import { PixelLeaves, PixelSparkles } from "./ui";
import { mascot } from "../lib/images";

export const navItems: { id: Page; label: string; icon: typeof Home; hint: string }[] = [
  { id: "home", label: "Главная", icon: Home, hint: "Запуск и статус" },
  { id: "profiles", label: "Профили", icon: UserRound, hint: "Игровые профили" },
  { id: "settings", label: "Настройки", icon: Settings2, hint: "Поведение лаунчера" },
  { id: "mods", label: "Моды", icon: Puzzle, hint: "Локальная библиотека" },
  { id: "stats", label: "Статистика", icon: ChartNoAxesColumn, hint: "Активность" },
];

export function Sidebar({
  page,
  onNavigate,
  version,
  online,
}: {
  page: Page;
  onNavigate: (page: Page) => void;
  version: string;
  online: boolean;
}) {
  return (
    <aside className="sidebar-surface relative hidden w-[248px] shrink-0 flex-col overflow-hidden border-r border-white/6 lg:flex xl:w-[266px]">
      <PixelSparkles className="opacity-50" />
      <div className="pointer-events-none absolute -top-16 left-1/2 h-64 w-64 -translate-x-1/2 rounded-full bg-[rgba(var(--accent-rgb),0.13)] blur-3xl" />
      <PixelLeaves className="absolute right-[-14px] bottom-[-6px] h-32 w-32 opacity-35" />
      <PixelLeaves className="absolute right-16 bottom-2 h-16 w-16 rotate-12 opacity-20" />

      <div className="relative px-5 pt-6 pb-4 xl:px-6 xl:pt-7">
        <div className="flex items-center justify-center py-1">
          <div className="avatar-glow relative flex h-[158px] w-[174px] items-center justify-center rounded-[44px]">
            <div className="absolute inset-x-2 inset-y-4 rounded-full bg-[rgba(var(--accent-rgb),0.16)] blur-2xl" />
            <img
              src={mascot}
              alt="Пиксельный маскот GAYCLIENT - кот в зелёном капюшоне"
              className="mascot-glow mascot-fade animate-float relative z-10 h-[156px] w-[156px] object-contain"
            />
            <span className="pixel-spark left-[10px] top-[30%] h-[5px] w-[5px] rounded-[1px]" />
            <span className="pixel-spark right-[2px] top-[54%] h-[4px] w-[4px] rounded-[1px] [animation-delay:1.1s]" />
            <span className="pixel-spark right-[20%] top-[6%] h-[3px] w-[3px] rounded-[1px] [animation-delay:2s]" />
            <span className="pixel-spark left-[26%] bottom-[8%] h-[3px] w-[3px] rounded-[1px] [animation-delay:1.6s]" />
          </div>
        </div>

        <div className="mt-1 text-center">
          <h1 className="font-pixel text-[16px] leading-none tracking-[0.02em] text-[#f0fff4] [text-shadow:0_0_22px_rgba(var(--accent-rgb),0.35)] xl:text-[17px]">
            GAYCLIENT
          </h1>
          <p className="mt-2 text-[9.5px] font-bold tracking-[0.34em] text-[#71917c] uppercase">
            Game Launcher
          </p>
        </div>
      </div>

      <nav aria-label="Основная навигация" className="relative px-3">
        <p className="mb-2.5 px-3 text-[9px] font-extrabold tracking-[0.24em] text-[#526e5b] uppercase">
          Навигация
        </p>
        <ul className="space-y-1">
          {navItems.map((item, index) => {
            const Icon = item.icon;
            const active = page === item.id;
            return (
              <li key={item.id}>
                <button
                  type="button"
                  onClick={() => onNavigate(item.id)}
                  aria-current={active ? "page" : undefined}
                  className={cn("nav-item group", active && "nav-item-active")}
                >
                  <span
                    className={cn(
                      "flex h-7 w-7 items-center justify-center rounded-lg transition-colors",
                      active
                        ? "bg-[rgba(var(--accent-rgb),0.12)]"
                        : "bg-white/3 group-hover:bg-white/6",
                    )}
                  >
                    <Icon size={16} strokeWidth={active ? 2.4 : 1.9} />
                  </span>
                  <span className="flex-1 text-left">{item.label}</span>
                  <kbd
                    className={cn(
                      "rounded border px-1.5 py-0.5 font-sans text-[9px] font-bold transition-opacity",
                      active
                        ? "border-[rgba(var(--accent-rgb),0.25)] text-[#b6dfc1]"
                        : "border-white/8 text-[#5f7c68] opacity-0 group-hover:opacity-100",
                    )}
                  >
                    {index + 1}
                  </kbd>
                </button>
              </li>
            );
          })}
        </ul>
      </nav>

      <div className="relative mt-auto px-3 pt-5 pb-3.5">
        <div className="rounded-xl border border-white/8 bg-[linear-gradient(155deg,rgba(29,51,36,0.72),rgba(8,19,12,0.85))] p-3.5">
          <div className="flex items-center gap-2">
            <span className="relative flex h-2 w-2">
              {online && (
                <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-[var(--accent)] opacity-60" />
              )}
              <span className="relative inline-flex h-2 w-2 rounded-full bg-[var(--accent)]" />
            </span>
            <p className="text-xs font-bold text-[#ddf4e3]">{online ? "Лаунчер запущен" : "Ошибка подключения"}</p>
            <Wifi size={12} className="ml-auto text-[#57745f]" />
          </div>
          <div className="mt-2 flex items-center justify-between text-[10.5px] text-[#7f9e89]">
            <span>Версия</span>
            <span className="font-mono font-semibold text-[#b6d3be]">{version}</span>
          </div>
          <div className="mt-2 h-px bg-white/6" />
          <p className="mt-2 flex items-center gap-1.5 text-[10px] text-[#6e8c77]">
            <span className="h-1 w-1 rounded-full bg-[var(--accent)] opacity-70" />
            Готов к подключению
          </p>
        </div>
        <p className="mt-2.5 px-1 text-center text-[9.5px] tracking-wide text-[#4e6a57]">
          1-5 - переключение разделов
        </p>
      </div>
    </aside>
  );
}

export function MobileNav({
  page,
  onNavigate,
}: {
  page: Page;
  onNavigate: (page: Page) => void;
}) {
  return (
    <nav aria-label="Мобильная навигация" className="border-b border-white/6 lg:hidden">
      <ul className="scroll-area flex gap-1 overflow-x-auto px-3 py-2">
        {navItems.map((item) => {
          const Icon = item.icon;
          const active = page === item.id;
          return (
            <li key={item.id} className="shrink-0">
              <button
                type="button"
                onClick={() => onNavigate(item.id)}
                aria-current={active ? "page" : undefined}
                className={cn(
                  "flex items-center gap-2 rounded-lg px-3 py-2 text-xs font-bold transition-colors",
                  active
                    ? "border border-[rgba(var(--accent-rgb),0.25)] bg-[rgba(var(--accent-rgb),0.12)] text-[#eafff0]"
                    : "border border-transparent text-[#84a08d] hover:bg-white/4 hover:text-white",
                )}
              >
                <Icon size={15} className={active ? "text-[var(--accent)]" : undefined} />
                {item.label}
              </button>
            </li>
          );
        })}
      </ul>
    </nav>
  );
}
