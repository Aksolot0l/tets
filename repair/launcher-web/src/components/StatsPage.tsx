import {
  Award,
  Clock3,
  Compass,
  Flame,
  Gamepad2,
  Moon,
  PlayCircle,
  Rocket,
  TrendingUp,
  Trophy,
  Users,
} from "lucide-react";
import { useMemo, useState } from "react";
import { achievements, recentSessions, statsByRange } from "../lib/data";
import type { Profile } from "../lib/types";
import { cn } from "../utils/cn";
import { PageHeading, Panel, SectionTitle } from "./ui";

const ranges = ["7", "30", "90"] as const;
type Range = (typeof ranges)[number];

const icons = {
  rocket: Rocket,
  compass: Compass,
  moon: Moon,
  layout: Trophy,
};

function ActivityChart({ values, labels }: { values: number[]; labels: string[] }) {
  const [hover, setHover] = useState<number | null>(null);
  const width = 640;
  const height = 196;
  const padding = { left: 14, right: 14, top: 18, bottom: 30 };
  const max = Math.max(...values) * 1.15;

  const points = useMemo(
    () =>
      values.map((value, index) => ({
        x: padding.left + (index * (width - padding.left - padding.right)) / (values.length - 1),
        y: padding.top + (1 - value / max) * (height - padding.top - padding.bottom),
        value,
      })),
    [values, max],
  );

  const line = points.reduce((path, point, index) => {
    if (index === 0) return `M ${point.x} ${point.y}`;
    const prev = points[index - 1];
    const midX = (prev.x + point.x) / 2;
    return `${path} C ${midX} ${prev.y}, ${midX} ${point.y}, ${point.x} ${point.y}`;
  }, "");

  const area = `${line} L ${points[points.length - 1].x} ${height - padding.bottom} L ${points[0].x} ${height - padding.bottom} Z`;

  return (
    <div className="relative mt-4">
      <svg
        viewBox={`0 0 ${width} ${height}`}
        className="h-[190px] w-full overflow-visible sm:h-[212px]"
        role="img"
        aria-label="График игровой активности по дням"
        onMouseLeave={() => setHover(null)}
      >
        <defs>
          <linearGradient id="stats-fill" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stopColor="var(--accent)" stopOpacity="0.32" />
            <stop offset="100%" stopColor="var(--accent)" stopOpacity="0" />
          </linearGradient>
          <linearGradient id="stats-line" x1="0" y1="0" x2="1" y2="0">
            <stop offset="0%" stopColor="var(--accent-deep)" />
            <stop offset="60%" stopColor="var(--accent)" />
            <stop offset="100%" stopColor="var(--accent-strong)" />
          </linearGradient>
        </defs>
        {[0.25, 0.5, 0.75, 1].map((step) => (
          <line
            key={step}
            x1={padding.left}
            x2={width - padding.right}
            y1={padding.top + step * (height - padding.top - padding.bottom)}
            y2={padding.top + step * (height - padding.top - padding.bottom)}
            stroke="rgba(255,255,255,0.055)"
            strokeDasharray="4 6"
            strokeWidth="1"
          />
        ))}
        <path d={area} fill="url(#stats-fill)" />
        <path d={line} fill="none" stroke="url(#stats-line)" strokeWidth="2.4" strokeLinecap="round" />
        {points.map((point, index) => (
          <g key={index}>
            <line
              x1={point.x}
              x2={point.x}
              y1={padding.top}
              y2={height - padding.bottom}
              stroke="rgba(var(--accent-rgb),0.16)"
              strokeWidth="1"
              opacity={hover === index ? 1 : 0}
            />
            <circle
              cx={point.x}
              cy={point.y}
              r={hover === index ? 4.5 : 2.6}
              fill="#07120c"
              stroke="var(--accent)"
              strokeWidth="2"
              style={{ transition: "r .12s ease" }}
            />
            <circle
              cx={point.x}
              cy={point.y}
              r="14"
              fill="transparent"
              onMouseEnter={() => setHover(index)}
              className="cursor-crosshair"
            />
            <text
              x={point.x}
              y={height - 9}
              textAnchor="middle"
              fontSize="9"
              fill={hover === index ? "var(--accent)" : "#5f7c6a"}
            >
              {labels[index] ?? ""}
            </text>
          </g>
        ))}
      </svg>
      {hover !== null && points[hover] && (
        <div
          className="pointer-events-none absolute z-10 -translate-x-1/2 -translate-y-full rounded-lg border border-[rgba(var(--accent-rgb),0.3)] bg-[#0a1a10] px-2.5 py-1.5 text-center shadow-xl"
          style={{
            left: `${(points[hover].x / width) * 100}%`,
            top: `${(points[hover].y / height) * 100}%`,
          }}
        >
          <p className="font-mono text-[10px] font-bold text-[#dfffe6]">{points[hover].value} мин</p>
          <p className="text-[8.5px] text-[#77937f]">сессия - {labels[hover]}</p>
        </div>
      )}
    </div>
  );
}

export function StatsPage({ profiles }: { profiles: Profile[] }) {
  const [range, setRange] = useState<Range>("30");
  const data = statsByRange[range];

  const summary = [
    {
      label: "Время в игре",
      value: data.total,
      hint: `${data.delta} к прошлому периоду`,
      icon: Clock3,
      bars: true,
    },
    {
      label: "Игровые сессии",
      value: String(data.sessions),
      hint: `в среднем ${data.avg}`,
      icon: PlayCircle,
      bars: false,
    },
    {
      label: "Профили",
      value: String(profiles.length),
      hint: "локальных наборов",
      icon: Users,
      bars: false,
    },
    {
      label: "Серия запусков",
      value: `${data.streak} дн.`,
      hint: "лучший день - суббота",
      icon: Flame,
      bars: false,
    },
  ];

  return (
    <div className="animate-fade-up space-y-4 pt-1">
      <PageHeading
        eyebrow="Активность"
        title="Ваша статистика"
        description="Сводка сессий и профилей лаунчера. Данные демонстрационные и не читают реальный журнал игры."
        action={
          <div className="flex rounded-lg border border-white/8 bg-black/25 p-0.5" role="group" aria-label="Период статистики">
            {ranges.map((item) => (
              <button
                key={item}
                type="button"
                onClick={() => setRange(item)}
                aria-pressed={range === item}
                className={cn(
                  "rounded-md px-3 py-1.5 text-[10.5px] font-bold transition-colors",
                  range === item
                    ? "bg-[rgba(var(--accent-rgb),0.15)] text-[#dfffe6] shadow-[inset_0_0_0_1px_rgba(var(--accent-rgb),0.25)]"
                    : "text-[#75927f] hover:text-[#d6e9dc]",
                )}
              >
                {statsByRange[item].label}
              </button>
            ))}
          </div>
        }
      />

      <div className="grid gap-3 sm:grid-cols-2 xl:grid-cols-4">
        {summary.map((item) => {
          const Icon = item.icon;
          return (
            <Panel key={item.label} className="relative overflow-hidden p-4">
              <div className="flex items-start justify-between">
                <div>
                  <p className="text-[9px] font-extrabold tracking-[0.16em] text-[#6e8b77] uppercase">{item.label}</p>
                  <p className="mt-1.5 font-display text-[22px] leading-none text-[#f0fbf3]">{item.value}</p>
                </div>
                <span className="flex h-8 w-8 items-center justify-center rounded-lg border border-[rgba(var(--accent-rgb),0.18)] bg-[rgba(var(--accent-rgb),0.07)] text-[var(--accent)]">
                  <Icon size={14} />
                </span>
              </div>
              <div className="mt-3 flex items-end justify-between gap-2">
                <p className="flex items-center gap-1 text-[9.5px] font-semibold text-[#7f9e89]">
                  <TrendingUp size={10} className="text-[var(--accent)]/80" />
                  {item.hint}
                </p>
                {item.bars && (
                  <div className="flex h-5 items-end gap-[2px]">
                    {[40, 65, 48, 80, 62, 95].map((height, index) => (
                      <span
                        key={index}
                        className="w-[3px] rounded-[1px] bg-[var(--accent)]/70"
                        style={{ height: `${height / 4}px`, opacity: 0.35 + index * 0.1 }}
                      />
                    ))}
                  </div>
                )}
              </div>
              <span className="pointer-events-none absolute -right-8 -bottom-8 h-20 w-20 rounded-full bg-[rgba(var(--accent-rgb),0.05)] blur-xl" />
            </Panel>
          );
        })}
      </div>

      <div className="grid gap-4 xl:grid-cols-[minmax(0,1.7fr)_minmax(0,1fr)]">
        <Panel className="p-5">
          <SectionTitle
            title="Игровая активность"
            hint={`${data.total} за последние ${data.label.toLowerCase()} - предыдущий период ${data.previous}`}
            icon={<Gamepad2 size={15} />}
            action={<span className="chip border-[rgba(var(--accent-rgb),0.22)] text-[var(--accent)]">+{data.delta}</span>}
          />
          <ActivityChart values={data.chart} labels={data.labels} />
        </Panel>

        <Panel className="p-5">
          <SectionTitle title="Достижения" hint="Прогресс лаунчера, не игры." icon={<Award size={15} />} />
          <div className="mt-4 space-y-3">
            {achievements.map((achievement) => {
              const Icon = icons[achievement.icon as keyof typeof icons] ?? Trophy;
              const complete = achievement.progress >= 100;
              return (
                <div key={achievement.title} className="rounded-lg border border-white/5 bg-white/[0.02] p-3">
                  <div className="flex items-center gap-2.5">
                    <span
                      className={cn(
                        "flex h-8 w-8 shrink-0 items-center justify-center rounded-lg border",
                        complete
                          ? "border-[rgba(var(--accent-rgb),0.3)] bg-[rgba(var(--accent-rgb),0.12)] text-[var(--accent)]"
                          : "border-white/8 bg-white/4 text-[#7d9a86]",
                      )}
                    >
                      <Icon size={14} />
                    </span>
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center justify-between gap-2">
                        <p className="truncate text-[11.5px] font-bold text-[#dbece0]">{achievement.title}</p>
                        <span className="font-mono text-[9px] text-[#708d7a]">{achievement.progress}%</span>
                      </div>
                      <p className="mt-0.5 truncate text-[9.5px] text-[#6f8b79]">{achievement.detail}</p>
                    </div>
                  </div>
                  <div className="mt-2 h-1 overflow-hidden rounded-full bg-white/6">
                    <div
                      className="h-full rounded-full bg-[var(--accent)] transition-all duration-500"
                      style={{ width: `${achievement.progress}%`, opacity: complete ? 1 : 0.55 }}
                    />
                  </div>
                </div>
              );
            })}
          </div>
        </Panel>
      </div>

      <div className="grid gap-4 xl:grid-cols-[minmax(0,1.7fr)_minmax(0,1fr)]">
        <Panel className="overflow-hidden">
          <div className="border-b border-white/6 px-5 py-4">
            <SectionTitle title="Последние сессии" hint="История подключений к клиенту." icon={<Clock3 size={15} />} />
          </div>
          <div className="overflow-x-auto">
            <table className="w-full min-w-[540px] text-left">
              <thead>
                <tr className="border-b border-white/5 text-[8.5px] tracking-[0.16em] text-[#5f7c6a] uppercase">
                  <th className="px-5 py-2.5 font-bold">Мир</th>
                  <th className="px-3 py-2.5 font-bold">Профиль</th>
                  <th className="px-3 py-2.5 font-bold">Дата</th>
                  <th className="px-3 py-2.5 font-bold">Время</th>
                  <th className="px-5 py-2.5 text-right font-bold">Состояние</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-white/4">
                {recentSessions.map((session, index) => (
                  <tr key={index} className="transition-colors hover:bg-white/[0.018]">
                    <td className="px-5 py-3">
                      <div className="flex items-center gap-2.5">
                        <span className="flex h-7 w-7 items-center justify-center rounded-md border border-white/7 bg-white/4 text-[#82a18c]">
                          <Compass size={12} />
                        </span>
                        <span className="text-[11.5px] font-semibold text-[#e0f0e5]">{session.world}</span>
                      </div>
                    </td>
                    <td className="px-3 py-3 text-[10.5px] text-[#9bb8a4]">{session.profile}</td>
                    <td className="px-3 py-3 text-[10.5px] text-[#789582]">{session.date}</td>
                    <td className="px-3 py-3 font-mono text-[10px] text-[#a8c5b0]">{session.time}</td>
                    <td className="px-5 py-3 text-right">
                      <span
                        className={cn(
                          "inline-flex items-center gap-1 rounded-full border px-2 py-0.5 text-[8.5px] font-bold",
                          session.status === "Завершена"
                            ? "border-[rgba(var(--accent-rgb),0.22)] bg-[rgba(var(--accent-rgb),0.07)] text-[#b6e6c2]"
                            : "border-amber-300/20 bg-amber-400/8 text-amber-200",
                        )}
                      >
                        <span className="h-1 w-1 rounded-full bg-current" />
                        {session.status}
                      </span>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Panel>

        <Panel className="p-5">
          <SectionTitle title="Профили по сессиям" hint="За выбранный период." icon={<Users size={15} />} />
          <div className="mt-4 space-y-3.5">
            {profiles.slice(0, 4).map((profile, index) => {
              const share = [62, 26, 8, 4][index] ?? 4;
              return (
                <div key={profile.id}>
                  <div className="flex items-center justify-between gap-2 text-[10.5px]">
                    <span className="flex min-w-0 items-center gap-2 truncate font-semibold text-[#c7dccd]">
                      <span className="flex h-5 w-5 shrink-0 items-center justify-center rounded-md border border-white/8 bg-white/4 text-[8px] font-extrabold text-[#8eab97]">
                        {profile.label.slice(0, 1).toUpperCase()}
                      </span>
                      {profile.label}
                    </span>
                    <span className="shrink-0 font-mono text-[9.5px] text-[#75927f]">{share}%</span>
                  </div>
                  <div className="mt-1.5 h-1.5 overflow-hidden rounded-full bg-white/6">
                    <div
                      className="h-full rounded-full bg-gradient-to-r from-[var(--accent-deep)] to-[var(--accent)]"
                      style={{ width: `${share}%`, opacity: 1 - index * 0.16 }}
                    />
                  </div>
                </div>
              );
            })}
            {profiles.length === 0 && <p className="text-[11px] text-[#75927f]">Профилей пока нет.</p>}
          </div>

          <div className="mt-4 rounded-xl border border-white/6 bg-[#06110b]/70 p-3.5">
            <div className="flex items-center justify-between">
              <p className="text-[10px] font-bold text-[#b9d4c0]">Цель недели</p>
              <span className="font-mono text-[9px] text-[var(--accent)]">68%</span>
            </div>
            <div className="mt-2 h-1.5 overflow-hidden rounded-full bg-white/6">
              <div className="h-full w-[68%] rounded-full bg-[var(--accent)] shadow-[0_0_12px_rgba(var(--accent-rgb),0.5)]" />
            </div>
            <p className="mt-2 text-[9px] leading-relaxed text-[#6e8a77]">3 ч 12 мин из 5 часов игрового времени.</p>
          </div>

          <div className="mt-3 flex items-center gap-2 rounded-xl border border-white/6 bg-white/[0.02] px-3 py-2.5">
            <Trophy size={14} className="text-[#c7b86a]" />
            <p className="text-[9.5px] leading-relaxed text-[#789582]">
              Лучший день: <strong className="text-[#c3dcc9]">{data.bestDay}</strong> - {data.avg} средняя сессия
            </p>
          </div>
        </Panel>
      </div>
    </div>
  );
}
