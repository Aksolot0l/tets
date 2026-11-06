import {
  ArrowRight,
  Boxes,
  Check,
  CircleAlert,
  Clock3,
  FileText,
  Gamepad2,
  Leaf,
  LoaderCircle,
  Play,
  ShieldCheck,
  Sparkles,
} from "lucide-react";
import type { LaunchStatus, LauncherSettings, Profile } from "../lib/types";
import { cn } from "../utils/cn";
import { Bars, CreeperArt, PixelSparkles } from "./ui";
import { mascot, world } from "../lib/images";

const statusCopy: Record<
  LaunchStatus,
  { label: string; title: string; detail: string; tone: "neutral" | "progress" | "success" | "warning" }
> = {
  idle: {
    label: "Статус подключения",
    title: "Ожидание игры",
    detail: "Нажмите «Начать игру» - клиент подключится автоматически.",
    tone: "neutral",
  },
  detected: {
    label: "Статус подключения",
    title: "Minecraft найден",
    detail: "Ждём завершения запуска игры. Это может занять несколько секунд.",
    tone: "progress",
  },
  loading: {
    label: "Статус подключения",
    title: "Подключаем клиент",
    detail: "Загружаем GAYClientInternal.dll.",
    tone: "progress",
  },
  loaded: {
    label: "Загрузка DLL",
    title: "DLL загружена",
    detail: "Проверьте оверлей в игре. Не закрывайте лаунчер во время игры.",
    tone: "success",
  },
  error: {
    label: "Статус подключения",
    title: "Не удалось подключить клиент",
    detail: "Откройте журнал, чтобы увидеть подробности, и попробуйте снова.",
    tone: "warning",
  },
};

export function HomePage({
  profile,
  status,
  settings,
  onLaunch,
  onOpenLog,
  onOpenProfiles,
  onOpenMods,
  onOpenStats,
}: {
  profile?: Profile;
  status: LaunchStatus;
  settings: LauncherSettings;
  onLaunch: () => void;
  onOpenLog: () => void;
  onOpenProfiles: () => void;
  onOpenMods: () => void;
  onOpenStats: () => void;
}) {
  const copy = statusCopy[status];
  const launching = status === "detected" || status === "loading";
  const connected = status === "loaded";
  const hasGame = Boolean(profile?.aumid);
  const accentLabel = settings.accent === "lime" ? "Лайм" : settings.accent === "mint" ? "Мята" : "Изумруд";

  return (
    <div className="animate-fade-up flex min-h-full flex-col gap-3.5 pt-1">
      <section className="hero-card relative overflow-hidden rounded-[20px] border border-white/8">
        <div className="relative h-[clamp(380px,47vh,510px)]">
          <img
            src={world}
            alt="Ночной мир Minecraft с парящим островом, водопадами и авророй"
            className="absolute inset-0 h-full w-full object-cover object-[66%_center] sm:object-[68%_center]"
          />
          <div className="hero-shade absolute inset-0" />
          <div className="absolute inset-0 bg-[linear-gradient(180deg,rgba(5,15,9,0.62)_0%,rgba(5,15,9,0.55)_45%,rgba(4,11,7,0.88)_100%)] sm:hidden" />
          <div className="hero-glow absolute inset-0" />
          <div className="absolute inset-x-0 bottom-0 h-24 bg-gradient-to-t from-[#050f09]/70 to-transparent" />
          <PixelSparkles className="opacity-70" />

          <div className="relative z-10 flex h-full max-w-[620px] flex-col justify-center px-6 py-7 sm:px-9 xl:px-10">
            <div className="flex flex-wrap items-center gap-2">
              <span className="badge-accent px-3 py-1.5 text-[10.5px] font-bold tracking-[0.04em]">
                <Sparkles size={12} />
                Ваш мир ждёт
              </span>
              <span className="chip border-white/10 bg-black/25 text-[9.5px] text-[#b8d1bf]">
                Сезон - Аврора
              </span>
            </div>

            <h1 className="hero-title mt-4 font-display text-[32px] leading-[1.08] text-[#f7fff8] sm:text-[40px] xl:text-[46px]">
              Готов к новым
              <br />
              <span className="bg-gradient-to-r from-[#e6ffd8] via-[var(--accent-strong)] to-[var(--accent)] bg-clip-text text-transparent drop-shadow-[0_2px_18px_rgba(var(--accent-rgb),0.25)]">
                приключениям?
              </span>
            </h1>

            <p className="mt-3.5 max-w-[352px] text-[13px] leading-relaxed text-[#c3d8c9]/85 sm:text-[14px]">
              Запускай игру - и открывай для себя новые миры. Клиент подключится
              автоматически, без лишних окон.
            </p>

            <div className="mt-5 flex flex-wrap items-center gap-3">
              <button
                type="button"
                onClick={onLaunch}
                disabled={launching}
                className="btn-primary h-[50px] min-w-[204px] px-5 text-[14.5px] sm:min-w-[220px] sm:px-6"
              >
                {launching ? (
                  <LoaderCircle size={19} className="animate-spin" />
                ) : connected ? (
                  <Check size={19} strokeWidth={2.6} />
                ) : (
                  <span className="flex h-7 w-7 items-center justify-center rounded-full bg-[#05230f]/12">
                    <Play size={13} fill="currentColor" />
                  </span>
                )}
                <span>{launching ? "Подключаем клиент…" : connected ? "DLL загружена" : "Начать игру"}</span>
                {!connected && <ArrowRight size={16} className="ml-auto opacity-70 sm:ml-1" />}
              </button>

              <div className="hidden flex-col gap-0.5 sm:flex">
                <span className="flex items-center gap-1.5 text-[10.5px] font-semibold text-[#c7ddce]/85">
                  <ShieldCheck size={12} className="text-[var(--accent)]" />
                  Автоподключение включено
                </span>
                <span className="flex items-center gap-1.5 text-[10px] text-[#8eab97]">
                  <kbd className="rounded border border-white/15 bg-black/30 px-1.5 py-0.5 font-sans text-[9px] text-[#c2d8c9]">
                    Ctrl
                  </kbd>
                  <span>+</span>
                  <kbd className="rounded border border-white/15 bg-black/30 px-1.5 py-0.5 font-sans text-[9px] text-[#c2d8c9]">
                    ↵
                  </kbd>
                  <span>быстрый запуск</span>
                </span>
              </div>
            </div>

            <div className="mt-4 flex flex-wrap items-center gap-x-4 gap-y-2 text-[10px] text-[#9bb8a4]">
              <span className="flex items-center gap-1.5">
                <span className="h-1.5 w-1.5 rounded-full bg-[var(--accent)] shadow-[0_0_6px_1px_rgba(var(--accent-rgb),0.5)]" />
                Профиль: <strong className="font-bold text-[#d8eddf]">{profile?.label ?? "не выбран"}</strong>
              </span>
              <span className="h-3 w-px bg-white/10" />
              <span>Minecraft - 1.1.5</span>
              <span className="h-3 w-px bg-white/10" />
              <span>Останется на экране</span>
            </div>

            {!hasGame && (
              <button
                type="button"
                onClick={onOpenProfiles}
                className="mt-3 inline-flex w-fit items-center gap-2 rounded-lg border border-amber-300/25 bg-amber-400/10 px-2.5 py-1.5 text-[10.5px] font-semibold text-amber-200 transition-colors hover:bg-amber-400/15"
              >
                <CircleAlert size={12} />
                Игра не выбрана в профиле - настроить
              </button>
            )}
          </div>

          <div className="absolute right-4 bottom-4 z-10 hidden items-center gap-2 rounded-xl border border-white/12 bg-[#06120c]/65 py-1.5 pr-3.5 pl-1.5 backdrop-blur-md sm:flex">
            <img
              src={mascot}
              alt=""
              className="h-7 w-7 object-contain mascot-fade"
            />
            <span className="font-pixel text-[8px] leading-none text-[#e8fbea]">GAYCLIENT</span>
            <span className="ml-1 rounded border border-[rgba(var(--accent-rgb),0.3)] bg-[rgba(var(--accent-rgb),0.12)] px-1.5 py-0.5 font-mono text-[9px] font-bold text-[#c9ffd6]">
              v1.1.5
            </span>
          </div>

          <div className="absolute top-4 right-4 z-10 hidden items-center gap-2 rounded-full border border-white/12 bg-[#05110b]/55 px-3 py-1.5 text-[10px] text-[#c0d8c7] backdrop-blur-md xl:flex">
            <span className="relative flex h-1.5 w-1.5">
              <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-[var(--accent)] opacity-60" />
              <span className="relative inline-flex h-1.5 w-1.5 rounded-full bg-[var(--accent)]" />
            </span>
            Локально - {accentLabel}
          </div>
        </div>
      </section>

      <section className="panel relative overflow-hidden rounded-2xl px-4 py-4 sm:px-5">
        <div className="pointer-events-none absolute inset-y-0 right-0 w-72 bg-[radial-gradient(ellipse_at_right,rgba(var(--accent-rgb),0.08),transparent_70%)]" />
        <div className="relative z-10 flex flex-wrap items-center gap-4 sm:flex-nowrap sm:gap-5">
          <div
            className={cn(
              "flex h-[58px] w-[58px] shrink-0 flex-col justify-center gap-2 rounded-xl border bg-[#0a1810] px-3",
              copy.tone === "success"
                ? "border-[rgba(var(--accent-rgb),0.3)]"
                : copy.tone === "warning"
                  ? "border-amber-300/25"
                  : "border-white/8",
            )}
          >
            <Bars active={copy.tone === "success"} loading={copy.tone === "progress"} />
            <span className="font-mono text-[8px] tracking-[0.16em] text-[#698574] uppercase">signal</span>
          </div>

          <div className="min-w-[200px] flex-1">
            <div className="flex items-center gap-2">
              <span
                className={cn(
                  "relative flex h-1.5 w-1.5 rounded-full",
                  copy.tone === "success"
                    ? "bg-[var(--accent)]"
                    : copy.tone === "progress"
                      ? "bg-lime-300"
                      : copy.tone === "warning"
                        ? "bg-amber-300"
                        : "bg-[#6b8a75]",
                )}
              >
                {copy.tone === "progress" && (
                  <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-lime-300 opacity-70" />
                )}
              </span>
              <p className="text-[9px] font-extrabold tracking-[0.18em] text-[#779680] uppercase">
                {copy.label}
              </p>
              <span className="h-px flex-1 bg-white/5" />
            </div>
            <h3 className="mt-1.5 font-display text-[17px] text-[#eefbf1] sm:text-[18px]">{copy.title}</h3>
            <p className="mt-0.5 text-[11.5px] leading-relaxed text-[#8aa791] sm:text-xs">{copy.detail}</p>
          </div>

          <div className="hidden items-center gap-4 sm:flex">
            <div className="text-right">
              <p className="text-[9px] font-bold tracking-[0.14em] text-[#57745f] uppercase">Канал</p>
              <p className="mt-1 font-mono text-[10.5px] text-[#a7c4af]">local - auto</p>
            </div>

          </div>

          <CreeperArt className="pointer-events-none absolute right-[-18px] bottom-[-38px] hidden h-40 w-40 text-[#2f6d45] opacity-25 lg:block" />
        </div>

        {(status === "loading" || status === "detected") && (
          <div className="absolute inset-x-0 bottom-0 h-[2px] bg-white/5">
            <div className="progress-shimmer h-full w-2/3" />
          </div>
        )}
      </section>

      <div className="grid gap-2.5 sm:grid-cols-3">
        {[
          {
            icon: Gamepad2,
            label: "Активный профиль",
            value: profile?.label ?? "Не выбран",
            hint: "Профили и App ID",
            onClick: onOpenProfiles,
          },
          {
            icon: Boxes,
            label: "Библиотека модов",
            value: "Пока недоступно",
            hint: "Без подключения",
            onClick: onOpenMods,
          },
          {
            icon: Clock3,
            label: "В игре за месяц",
            value: "Нет данных",
            hint: "Открыть статистику",
            onClick: onOpenStats,
          },
        ].map((card) => {
          const Icon = card.icon;
          return (
            <button
              key={card.label}
              type="button"
              onClick={card.onClick}
              className="panel group flex items-center gap-3 rounded-xl px-3.5 py-3 text-left transition-colors hover:border-[rgba(var(--accent-rgb),0.26)]"
            >
              <span className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg border border-white/7 bg-white/4 text-[#8faf99] transition-colors group-hover:text-[var(--accent)]">
                <Icon size={15} />
              </span>
              <span className="min-w-0 flex-1">
                <span className="block text-[8.5px] font-extrabold tracking-[0.16em] text-[#5f7c6a] uppercase">
                  {card.label}
                </span>
                <span className="mt-0.5 flex items-center gap-1.5">
                  <span className="truncate text-[11.5px] font-bold text-[#dfeee4]">{card.value}</span>
                  <span className="truncate text-[9px] text-[#6c8977]">- {card.hint}</span>
                </span>
              </span>
              <ArrowRight
                size={13}
                className="shrink-0 text-[#4f6b58] transition-all group-hover:translate-x-0.5 group-hover:text-[var(--accent)]"
              />
            </button>
          );
        })}
      </div>

      <div className="flex flex-wrap items-center justify-between gap-3 px-0.5">
        <div className="flex flex-wrap items-center gap-2">
          <button type="button" onClick={onOpenLog} className="btn-secondary h-10 px-4 text-xs">
            <FileText size={14} />
            Открыть журнал
          </button>
          <button
            type="button"
            onClick={onOpenProfiles}
            className="btn-ghost h-10 px-3 text-[11px] font-semibold"
          >
            Профили
            <ArrowRight size={13} />
          </button>
        </div>
        <p className="flex items-center gap-2 text-[10.5px] font-medium text-[#7c9a86]">
          <Leaf size={13} className="text-[var(--accent)]/70" />
          Не закрывайте лаунчер во время игры
        </p>
      </div>
    </div>
  );
}
