import {
  Boxes,
  Check,
  ChevronRight,
  Download,
  Info,
  LayoutGrid,
  Map,
  Palette,
  Search,
  Shield,
  Sparkles,
  Star,
  Swords,
  X,
  Zap,
} from "lucide-react";
import { useMemo, useState } from "react";
import { modCatalog } from "../lib/data";
import type { ModItem, ToastTone } from "../lib/types";
import { cn } from "../utils/cn";
import { EmptyState, PageHeading, Panel, PixelLeaves } from "./ui";

type Notify = (title: string, description?: string, tone?: ToastTone) => void;
type Category = "Все" | ModItem["category"];

const categories: Category[] = ["Все", "Оптимизация", "Графика", "Геймплей", "Интерфейс"];

const iconMap = {
  zap: Zap,
  palette: Palette,
  swords: Swords,
  layout: LayoutGrid,
  map: Map,
  shield: Shield,
};

export function ModsPage({
  modState,
  onChange,
  notify,
}: {
  modState: Record<string, boolean>;
  onChange: (next: Record<string, boolean>) => void;
  notify: Notify;
}) {
  const [query, setQuery] = useState("");
  const [category, setCategory] = useState<Category>("Все");

  const enabledCount = modCatalog.filter((mod) => modState[mod.id]).length;

  const filtered = useMemo(() => {
    const normalized = query.trim().toLowerCase();
    return modCatalog.filter((mod) => {
      const matchesCategory = category === "Все" || mod.category === category;
      const matchesQuery =
        !normalized ||
        mod.name.toLowerCase().includes(normalized) ||
        mod.description.toLowerCase().includes(normalized) ||
        mod.author.toLowerCase().includes(normalized);
      return matchesCategory && matchesQuery;
    });
  }, [query, category]);

  const toggle = (mod: ModItem) => {
    const next = { ...modState, [mod.id]: !modState[mod.id] };
    onChange(next);
    notify(
      `${mod.name} ${next[mod.id] ? "включён" : "выключен"}`,
      next[mod.id] ? "Пресет отмечен в локальной библиотеке." : "Пресет снят с профиля лаунчера.",
      next[mod.id] ? "success" : "info",
    );
  };

  const enableRecommended = () => {
    onChange({ ...modState, "aurora-core": true, "fps-pilot": true, "waypoint-atlas": true });
    notify("Рекомендуемый набор включён", "Aurora Core, FPS Pilot и Waypoint Atlas отмечены.", "success");
  };

  return (
    <div className="animate-fade-up space-y-4 pt-1">
      <PageHeading
        eyebrow="Локальная библиотека"
        title="Моды и дополнения"
        description="Наборы интерфейса, оптимизации и навигации для вашего профиля. Включённые пресеты сохраняются вместе с лаунчером."
        action={
          <div className="flex items-center gap-2">
            <span className="chip border-[rgba(var(--accent-rgb),0.24)] bg-[rgba(var(--accent-rgb),0.07)] text-[#bce8c7]">
              <Boxes size={11} />
              Включено {enabledCount} из {modCatalog.length}
            </span>
          </div>
        }
      />

      <section className="relative overflow-hidden rounded-2xl border border-[rgba(var(--accent-rgb),0.22)] bg-[linear-gradient(100deg,#0d2517_0%,#0b2115_45%,#07150e_100%)]">
        <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_88%_50%,rgba(var(--accent-rgb),0.18),transparent_55%)]" />
        <PixelLeaves className="absolute right-4 bottom-[-16px] h-32 w-32 text-[#3a8154] opacity-30" />
        <div className="absolute top-3 right-24 hidden grid-cols-4 gap-1 opacity-30 sm:grid">
          {Array.from({ length: 16 }).map((_, index) => (
            <span
              key={index}
              className="h-2.5 w-2.5 rounded-[1px] bg-[var(--accent)]"
              style={{ opacity: 0.2 + ((index * 7) % 9) / 12 }}
            />
          ))}
        </div>
        <div className="relative z-10 flex flex-wrap items-center justify-between gap-4 p-4 sm:p-5">
          <div className="flex items-center gap-3">
            <span className="flex h-11 w-11 items-center justify-center rounded-xl border border-[rgba(var(--accent-rgb),0.3)] bg-[rgba(var(--accent-rgb),0.12)] text-[var(--accent)]">
              <Sparkles size={19} />
            </span>
            <div>
              <p className="flex items-center gap-2 text-[9px] font-extrabold tracking-[0.18em] text-[var(--accent)] uppercase">
                Рекомендуемый набор
                <span className="rounded border border-[rgba(var(--accent-rgb),0.28)] bg-[rgba(var(--accent-rgb),0.08)] px-1 py-px text-[8px]">
                  под ваш профиль
                </span>
              </p>
              <h3 className="mt-1 font-display text-[17px] text-[#effff3]">Aurora Valley Starter</h3>
              <p className="mt-0.5 text-[10.5px] text-[#8aa791]">
                Мягкий интерфейс, стабильный FPS и навигация по мирам.
              </p>
            </div>
          </div>
          <div className="flex items-center gap-2">
            <span className="hidden items-center gap-1.5 rounded-lg border border-white/8 bg-black/20 px-2.5 py-1.5 text-[10px] text-[#9bb8a4] sm:flex">
              <Download size={11} />
              41.8k установок
            </span>
            <button type="button" onClick={enableRecommended} className="btn-primary h-9 px-3.5 text-[11px]">
              <Check size={13} />
              Включить набор
            </button>
          </div>
        </div>
      </section>

      <div className="flex flex-wrap items-center gap-2.5">
        <div className="relative min-w-[200px] flex-1">
          <Search size={14} className="pointer-events-none absolute top-1/2 left-3 -translate-y-1/2 text-[#5f7c6a]" />
          <input
            value={query}
            onChange={(event) => setQuery(event.target.value)}
            placeholder="Поиск по названию, автору или описанию…"
            aria-label="Поиск модов"
            className="input-base input-with-icon input-with-clear h-10 text-xs"
          />
          {query && (
            <button
              type="button"
              onClick={() => setQuery("")}
              aria-label="Очистить поиск"
              className="absolute top-1/2 right-2 flex h-6 w-6 -translate-y-1/2 items-center justify-center rounded-md text-[#6e8b77] hover:bg-white/6 hover:text-white"
            >
              <X size={13} />
            </button>
          )}
        </div>
        <div className="scroll-area flex max-w-full items-center gap-1.5 overflow-x-auto">
          {categories.map((item) => {
            const count =
              item === "Все" ? modCatalog.length : modCatalog.filter((mod) => mod.category === item).length;
            return (
              <button
                key={item}
                type="button"
                onClick={() => setCategory(item)}
                aria-pressed={category === item}
                className={cn(
                  "flex shrink-0 items-center gap-1.5 rounded-lg border px-2.5 py-2 text-[10.5px] font-semibold transition-colors",
                  category === item
                    ? "border-[rgba(var(--accent-rgb),0.35)] bg-[rgba(var(--accent-rgb),0.11)] text-[#dbffe3]"
                    : "border-white/7 bg-white/[0.02] text-[#7f9c88] hover:bg-white/5 hover:text-[#d7ebdd]",
                )}
              >
                {item}
                <span className={cn("font-mono text-[8.5px]", category === item ? "text-[var(--accent)]" : "text-[#5c7866]")}>
                  {count}
                </span>
              </button>
            );
          })}
        </div>
      </div>

      {filtered.length === 0 ? (
        <Panel className="p-4">
          <EmptyState
            icon={<Search size={20} />}
            title="Ничего не найдено"
            description="Попробуйте изменить запрос или выбрать другую категорию."
            action={
              <button
                type="button"
                onClick={() => {
                  setQuery("");
                  setCategory("Все");
                }}
                className="btn-secondary h-8 px-3 text-[11px]"
              >
                Сбросить фильтры
              </button>
            }
          />
        </Panel>
      ) : (
        <div className="grid gap-3 sm:grid-cols-2 2xl:grid-cols-3">
          {filtered.map((mod) => {
            const enabled = Boolean(modState[mod.id]);
            const Icon = iconMap[mod.icon];
            return (
              <article
                key={mod.id}
                className={cn(
                  "panel group relative flex flex-col overflow-hidden rounded-xl transition-all duration-200",
                  enabled
                    ? "border-[rgba(var(--accent-rgb),0.3)] shadow-[0_0_28px_-22px_rgba(var(--accent-rgb),0.8)]"
                    : "hover:border-white/12",
                )}
              >
                {enabled && (
                  <span className="absolute inset-x-0 top-0 h-px bg-gradient-to-r from-transparent via-[rgba(var(--accent-rgb),0.7)] to-transparent" />
                )}
                <div className="flex flex-1 items-start gap-3 p-3.5">
                  <span
                    className="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl border border-white/10 text-white shadow-inner"
                    style={{ background: `linear-gradient(145deg, ${mod.from}, ${mod.to})` }}
                  >
                    <Icon size={19} />
                  </span>
                  <div className="min-w-0 flex-1">
                    <div className="flex items-start justify-between gap-2">
                      <div className="min-w-0">
                        <h3 className="truncate text-[12.5px] font-bold text-[#eaf8ee]">{mod.name}</h3>
                        <p className="mt-0.5 truncate text-[9.5px] text-[#708d7a]">
                          {mod.author} - v{mod.version}
                        </p>
                      </div>
                      <span className="flex shrink-0 items-center gap-0.5 rounded border border-white/7 bg-white/4 px-1.5 py-0.5 text-[9px] font-bold text-[#c7b86a]">
                        <Star size={8} fill="currentColor" />
                        {mod.rating}
                      </span>
                    </div>
                    <p className="mt-2 line-clamp-2 min-h-[30px] text-[10.5px] leading-relaxed text-[#87a490]">
                      {mod.description}
                    </p>
                    <div className="mt-2 flex flex-wrap items-center gap-1.5">
                      <span className="chip px-1.5 py-0.5 text-[8.5px]">{mod.category}</span>
                      <span className="text-[8.5px] text-[#617d6b]">{mod.size}</span>
                      <span className="text-[8.5px] text-[#617d6b]">- {mod.downloads}</span>
                    </div>
                  </div>
                </div>
                <div className="flex items-center justify-between gap-2 border-t border-white/5 bg-black/15 px-3.5 py-2.5">
                  <span
                    className={cn(
                      "flex items-center gap-1.5 text-[9.5px] font-semibold",
                      enabled ? "text-[var(--accent)]" : "text-[#6f8c79]",
                    )}
                  >
                    <span
                      className={cn(
                        "h-1.5 w-1.5 rounded-full",
                        enabled ? "bg-[var(--accent)] shadow-[0_0_6px_1px_rgba(var(--accent-rgb),0.6)]" : "bg-[#4e6a57]",
                      )}
                    />
                    {enabled ? "Включён в профиле" : "Не активен"}
                  </span>
                  <button
                    type="button"
                    onClick={() => toggle(mod)}
                    aria-pressed={enabled}
                    className={cn(
                      "h-7 rounded-lg px-3 text-[10px] font-bold transition-all",
                      enabled
                        ? "border border-white/10 bg-white/5 text-[#c6ddcd] hover:border-red-400/30 hover:bg-red-500/10 hover:text-red-200"
                        : "btn-primary shadow-none",
                    )}
                  >
                    {enabled ? "Выключить" : "Включить"}
                  </button>
                </div>
              </article>
            );
          })}
        </div>
      )}

      <div className="flex flex-wrap items-center justify-between gap-3 rounded-xl border border-white/6 bg-white/[0.02] px-4 py-3">
        <p className="flex items-center gap-2 text-[10px] leading-relaxed text-[#6f8b79]">
          <Info size={12} className="shrink-0" />
          Каталог оформлен как локальная библиотека. Загрузка модов из сети и установка файлов пока
          не подключены.
        </p>
        <button
          type="button"
          onClick={() => notify("Каталог обновлён", "Локальный список актуален. Сетевого обновления нет.", "info")}
          className="btn-ghost h-7 px-2 text-[10px] font-semibold"
        >
          Проверить обновления
          <ChevronRight size={11} />
        </button>
      </div>
    </div>
  );
}
