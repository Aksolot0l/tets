import {
  ChartNoAxesColumn,
  Check,
  ChevronDown,
  Copy,
  Minus,
  Puzzle,
  Settings2,
  Sparkles,
  Square,
  UserRound,
  Users,
  X,
} from "lucide-react";
import { useEffect, useRef, useState } from "react";
import { navItems } from "./Sidebar";
import type { Page, Profile, WindowState } from "../lib/types";
import { cn } from "../utils/cn";
import { mascot } from "../lib/images";

const pageMeta: Record<Page, { label: string; icon: typeof Sparkles; tag: string }> = {
  home: { label: "Ваш лаунчер", icon: Sparkles, tag: "Rust core" },
  profiles: { label: "Профили", icon: Users, tag: "Локальное хранилище" },
  settings: { label: "Настройки", icon: Settings2, tag: "Персонализация" },
  mods: { label: "Моды", icon: Puzzle, tag: "Локальная библиотека" },
  stats: { label: "Статистика", icon: ChartNoAxesColumn, tag: "Сессии и активность" },
};

function ProfileMenu({
  profiles,
  selectedId,
  onSelect,
  onManage,
}: {
  profiles: Profile[];
  selectedId: string;
  onSelect: (id: string) => void;
  onManage: () => void;
}) {
  const [open, setOpen] = useState(false);
  const boxRef = useRef<HTMLDivElement>(null);
  const current = profiles.find((profile) => profile.id === selectedId) ?? profiles[0];

  useEffect(() => {
    if (!open) return;
    const onClick = (event: MouseEvent) => {
      if (!boxRef.current?.contains(event.target as Node)) setOpen(false);
    };
    const onKey = (event: KeyboardEvent) => {
      if (event.key === "Escape") setOpen(false);
    };
    document.addEventListener("mousedown", onClick);
    document.addEventListener("keydown", onKey);
    return () => {
      document.removeEventListener("mousedown", onClick);
      document.removeEventListener("keydown", onKey);
    };
  }, [open]);

  if (!current) return null;

  return (
    <div ref={boxRef} className="relative">
      <button
        type="button"
        onClick={() => setOpen((value) => !value)}
        aria-haspopup="menu"
        aria-expanded={open}
        className={cn(
          "group flex items-center gap-2.5 rounded-xl border py-1.5 pr-2.5 pl-2 transition-colors",
          open
            ? "border-[rgba(var(--accent-rgb),0.32)] bg-[rgba(var(--accent-rgb),0.08)]"
            : "border-transparent hover:border-white/8 hover:bg-white/4",
        )}
      >
        <span className="flex h-7 w-7 items-center justify-center rounded-lg border border-[rgba(var(--accent-rgb),0.25)] bg-[rgba(var(--accent-rgb),0.12)] text-[var(--accent)]">
          <UserRound size={14} />
        </span>
        <span className="hidden text-left min-[720px]:block">
          <span className="block text-[9px] font-bold tracking-[0.14em] text-[#698574] uppercase">
            Профиль
          </span>
          <span className="block max-w-[128px] truncate text-xs font-bold text-[#e6f5ea]">
            {current.label}
          </span>
        </span>
        <ChevronDown
          size={14}
          className={cn("text-[#6d8b77] transition-transform", open && "rotate-180 text-[var(--accent)]")}
        />
      </button>

      {open && (
        <div
          role="menu"
          aria-label="Выбор профиля"
          className="animate-fade-up absolute right-0 z-40 mt-2 w-[272px] overflow-hidden rounded-xl border border-white/10 bg-[linear-gradient(165deg,#12231a,#07110c)] p-1.5 shadow-[0_30px_80px_-20px_#000]"
        >
          <p className="px-2.5 pt-1.5 pb-2 text-[9px] font-extrabold tracking-[0.18em] text-[#5f7d69] uppercase">
            Профили лаунчера
          </p>
          <div className="max-h-64 space-y-0.5 overflow-y-auto scroll-area">
            {profiles.map((profile) => {
              const selected = profile.id === current.id;
              return (
                <button
                  key={profile.id}
                  type="button"
                  role="menuitemradio"
                  aria-checked={selected}
                  onClick={() => {
                    onSelect(profile.id);
                    setOpen(false);
                  }}
                  className={cn(
                    "flex w-full items-center gap-2.5 rounded-lg px-2.5 py-2 text-left transition-colors",
                    selected ? "bg-[rgba(var(--accent-rgb),0.1)]" : "hover:bg-white/5",
                  )}
                >
                  <span
                    className={cn(
                      "flex h-8 w-8 items-center justify-center rounded-lg border text-[11px] font-extrabold",
                      selected
                        ? "border-[rgba(var(--accent-rgb),0.35)] bg-[rgba(var(--accent-rgb),0.14)] text-[var(--accent)]"
                        : "border-white/8 bg-white/4 text-[#8ba993]",
                    )}
                  >
                    {profile.label.slice(0, 1).toUpperCase()}
                  </span>
                  <span className="min-w-0 flex-1">
                    <span className="block truncate text-xs font-bold text-[#e7f6eb]">{profile.label}</span>
                    <span className="mt-0.5 block truncate text-[10px] text-[#718d7b]">
                      {profile.nickname ? `@${profile.nickname}` : "Ник не указан"} - v{profile.version}
                    </span>
                  </span>
                  {selected && <Check size={14} className="text-[var(--accent)]" />}
                </button>
              );
            })}
          </div>
          <div className="mt-1 border-t border-white/6 pt-1">
            <button
              type="button"
              onClick={() => {
                onManage();
                setOpen(false);
              }}
              className="flex w-full items-center gap-2 rounded-lg px-2.5 py-2 text-[11.5px] font-semibold text-[#a7c6b0] transition-colors hover:bg-white/5 hover:text-white"
            >
              <Settings2 size={13} />
              Управлять профилями
            </button>
          </div>
        </div>
      )}
    </div>
  );
}

export function TopBar({
  page,
  profiles,
  selectedId,
  onSelectProfile,
  onManageProfiles,
  windowState,
  onMinimize,
  onMaximize,
  onClose,
}: {
  page: Page;
  profiles: Profile[];
  selectedId: string;
  onSelectProfile: (id: string) => void;
  onManageProfiles: () => void;
  windowState: WindowState;
  onMinimize: () => void;
  onMaximize: () => void;
  onClose: () => void;
}) {
  const meta = pageMeta[page];
  const Icon = meta.icon;
  const navItem = navItems.find((item) => item.id === page);

  return (
    <header className="relative flex h-16 shrink-0 items-center justify-between gap-3 border-b border-white/6 px-3 sm:px-5 lg:h-[72px] lg:px-7">
      <div className="flex min-w-0 items-center gap-3">
        <div className="flex items-center gap-2 lg:hidden">
          <img
            src={mascot}
            alt=""
            className="h-8 w-8 rounded-lg object-contain mascot-fade"
          />
          <span className="font-pixel text-[11px] text-[#f0fff4]">GAYCLIENT</span>
        </div>

        <div className="hidden min-w-0 items-center gap-3 lg:flex">
          <span className="flex h-8 w-8 items-center justify-center rounded-lg border border-[rgba(var(--accent-rgb),0.18)] bg-[rgba(var(--accent-rgb),0.08)] text-[var(--accent)]">
            <Icon size={16} />
          </span>
          <div className="min-w-0">
            <div className="flex items-center gap-2">
              <h2 className="truncate text-[13.5px] font-bold text-[#e8f7ec]">{meta.label}</h2>
              <span className="hidden rounded-md border border-white/7 bg-white/4 px-1.5 py-0.5 text-[9px] font-bold tracking-wider text-[#7f9e89] uppercase xl:inline">
                {navItem?.hint}
              </span>
            </div>
            <p className="mt-0.5 flex items-center gap-1.5 text-[10px] text-[#617d6b]">
              <Sparkles size={9} className="text-[#7fa48a]" />
              {meta.tag} - v1.1.5
            </p>
          </div>
        </div>
      </div>

      <div className="flex shrink-0 items-center gap-1 sm:gap-2">
        <ProfileMenu
          profiles={profiles}
          selectedId={selectedId}
          onSelect={onSelectProfile}
          onManage={onManageProfiles}
        />
        <div className="mx-1 hidden h-6 w-px bg-white/8 sm:block" />
        <div className="flex shrink-0 items-center">
          <button
            type="button"
            onClick={onMinimize}
            title="Свернуть"
            aria-label="Свернуть окно"
            className="btn-ghost h-8 w-8 p-0"
          >
            <Minus size={15} />
          </button>
          <button
            type="button"
            onClick={onMaximize}
            title={windowState === "maximized" ? "Восстановить" : "Развернуть"}
            aria-label={windowState === "maximized" ? "Восстановить окно" : "Развернуть окно"}
            className="btn-ghost hidden h-8 w-8 p-0 sm:flex"
          >
            {windowState === "maximized" ? <Copy size={13} /> : <Square size={12} />}
          </button>
          <button
            type="button"
            onClick={onClose}
            title="Закрыть"
            aria-label="Закрыть лаунчер"
            className="ml-0.5 flex h-8 w-8 items-center justify-center rounded-lg text-[#8aa793] transition-colors hover:bg-red-500/15 hover:text-red-300"
          >
            <X size={16} />
          </button>
        </div>
      </div>
    </header>
  );
}
