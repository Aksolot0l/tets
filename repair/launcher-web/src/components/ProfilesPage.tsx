import {
  AtSign,
  CircleAlert,
  Copy,
  CopyPlus,
  FolderOpen,
  Gamepad2,
  Hash,
  Info,
  Plus,
  Radar,
  Save,
  ShieldCheck,
  Sparkles,
  Trash2,
  UserRound,
} from "lucide-react";
import { useEffect, useRef, useState } from "react";
import { APP_VERSION } from "../lib/data";
import { copyText, formatProfileDate } from "../lib/hooks";
import type { Profile, ToastTone } from "../lib/types";
import { cn } from "../utils/cn";
import { Modal, PageHeading, Panel } from "./ui";

type Notify = (title: string, description?: string, tone?: ToastTone) => void;

const validAppId = (value: string) =>
  value === "" || (/^[A-Za-z0-9._-]+![A-Za-z0-9._-]+$/.test(value) && value.length <= 512);

export function ProfilesPage({
  profiles,
  selectedId,
  onSelect,
  onSave,
  onDelete,
  onDetect,
  notify,
}: {
  profiles: Profile[];
  selectedId: string;
  onSelect: (id: string) => Promise<boolean>;
  onSave: (profile: Profile, isNew: boolean) => Promise<boolean>;
  onDelete: (id: string) => Promise<boolean>;
  onDetect: () => Promise<string>;
  notify: Notify;
}) {
  const current = profiles.find((profile) => profile.id === selectedId) ?? profiles[0];
  const [isNew, setIsNew] = useState(false);
  const [form, setForm] = useState({ label: current?.label ?? "", nickname: current?.nickname ?? "", aumid: current?.aumid ?? "" });
  const [error, setError] = useState<string | null>(null);
  const [deleteId, setDeleteId] = useState<string | null>(null);
  const deleteTarget = profiles.find(profile => profile.id === deleteId);
  const labelRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (isNew) return;
    const active = profiles.find((profile) => profile.id === selectedId) ?? profiles[0];
    if (active) {
      setForm({ label: active.label, nickname: active.nickname, aumid: active.aumid });
      setError(null);
    }
  }, [selectedId, profiles, isNew]);

  const dirty =
    isNew ||
    !current ||
    form.label !== current.label ||
    form.nickname !== current.nickname ||
    form.aumid !== current.aumid;

  const appIdValid = validAppId(form.aumid.trim());

  const resetForm = () => {
    if (!current) return;
    setIsNew(false);
    setForm({ label: current.label, nickname: current.nickname, aumid: current.aumid });
    setError(null);
  };

  const startNew = () => {
    setIsNew(true);
    setForm({ label: "", nickname: "", aumid: current?.aumid ?? "" });
    setError(null);
    window.setTimeout(() => labelRef.current?.focus(), 60);
  };

  const duplicate = () => {
    setIsNew(true);
    setForm({
      label: `${form.label || "Профиль"} - копия`,
      nickname: form.nickname,
      aumid: form.aumid,
    });
    setError(null);
    notify("Создана копия профиля", "Не забудьте сохранить изменения.");
    window.setTimeout(() => labelRef.current?.focus(), 60);
  };

  const save = async () => {
    const label = form.label.trim();
    const nickname = form.nickname.trim();
    const aumid = form.aumid.trim();
    if (!label) {
      setError("Укажите название профиля - оно отображается в шапке лаунчера.");
      labelRef.current?.focus();
      return;
    }
    if (new TextEncoder().encode(label).length > 256 || new TextEncoder().encode(nickname).length > 256) {
      setError("Название профиля и ник не должны превышать 256 байт в UTF-8.");
      return;
    }
    if (!validAppId(aumid)) {
      setError("App ID должен содержать ровно один символ «!» и буквы, цифры, точку, дефис или подчёркивание.");
      return;
    }
    const next: Profile = {
      id: isNew || !current ? `profile-${Date.now()}` : current.id,
      label,
      nickname,
      version: current?.version ?? APP_VERSION,
      aumid,
      createdAt: isNew || !current ? new Date().toISOString().slice(0, 10) : current.createdAt,
    };
    if (await onSave(next, isNew || !current)) { setIsNew(false); setError(null); }
  };

  return (
    <div className="animate-fade-up space-y-4 pt-1">
      <PageHeading
        eyebrow="Локальные профили"
        title="Ваши профили"
        description="Разные наборы настроек для игры. Имя профиля и ник - локальные заметки, они не меняют учётную запись."
        action={
          <button type="button" onClick={startNew} className="btn-primary h-10 px-4 text-xs">
            <Plus size={15} strokeWidth={2.6} />
            Новый профиль
          </button>
        }
      />

      <div className="grid gap-4 xl:grid-cols-[296px_minmax(0,1fr)]">
        <div className="space-y-3">
          <Panel className="p-3">
            <div className="flex items-center justify-between px-1 pb-2.5">
              <p className="text-[10px] font-extrabold tracking-[0.18em] text-[#718e7a] uppercase">
                Сохранённые - {profiles.length}
              </p>
              <button
                type="button"
                onClick={startNew}
                title="Создать профиль"
                aria-label="Создать профиль"
                className="flex h-6 w-6 items-center justify-center rounded-md border border-white/8 bg-white/4 text-[#9dbca6] transition-colors hover:border-[rgba(var(--accent-rgb),0.35)] hover:text-[var(--accent)]"
              >
                <Plus size={13} />
              </button>
            </div>

            <div className="scroll-area max-h-[380px] space-y-1.5 overflow-y-auto pr-0.5">
              {profiles.map((profile) => {
                const selected = profile.id === selectedId && !isNew;
                return (
                  <div
                    key={profile.id}
                    className={cn(
                      "group relative rounded-xl border transition-colors",
                      selected
                        ? "border-[rgba(var(--accent-rgb),0.35)] bg-[linear-gradient(100deg,rgba(var(--accent-rgb),0.13),rgba(var(--accent-rgb),0.03))]"
                        : "border-white/6 bg-white/[0.018] hover:border-white/12 hover:bg-white/4",
                    )}
                  >
                    <button
                      type="button"
                      onClick={async () => {
                        if (await onSelect(profile.id)) setIsNew(false);
                      }}
                      className="flex w-full items-center gap-2.5 rounded-xl p-2.5 pr-9 text-left"
                      aria-pressed={selected}
                    >
                      <span
                        className={cn(
                          "relative flex h-9 w-9 shrink-0 items-center justify-center overflow-hidden rounded-lg border text-[12px] font-extrabold",
                          selected
                            ? "border-[rgba(var(--accent-rgb),0.3)] bg-[rgba(var(--accent-rgb),0.14)] text-[var(--accent)]"
                            : "border-white/8 bg-[#0c1a11] text-[#84a18d]",
                        )}
                      >
                        {profile.label.slice(0, 1).toUpperCase()}
                        {selected && (
                          <span className="absolute right-0.5 bottom-0.5 h-1 w-1 rounded-full bg-[var(--accent)] shadow-[0_0_5px_1px_rgba(var(--accent-rgb),0.7)]" />
                        )}
                      </span>
                      <span className="min-w-0 flex-1">
                        <span className="flex items-center gap-1.5">
                          <span className="truncate text-xs font-bold text-[#e8f7ec]">{profile.label}</span>
                          {profile.id === current?.id && !isNew && (
                            <span className="rounded border border-[rgba(var(--accent-rgb),0.25)] bg-[rgba(var(--accent-rgb),0.08)] px-1 py-px text-[8px] font-bold tracking-wide text-[#b8f0c5] uppercase">
                              актив
                            </span>
                          )}
                        </span>
                        <span className="mt-0.5 block truncate text-[10px] text-[#789582]">
                          {profile.nickname ? `@${profile.nickname}` : "Ник не указан"}
                        </span>
                        <span className="mt-1 flex items-center gap-1 text-[9px] text-[#658172]">
                          <span
                            className={cn(
                              "h-1 w-1 rounded-full",
                              profile.aumid ? "bg-[var(--accent)]/80" : "bg-amber-300/80",
                            )}
                          />
                          {profile.aumid ? `Minecraft - v${profile.version}` : "App ID не задан"}
                        </span>
                      </span>
                    </button>
                    <button
                      type="button"
                      onClick={() => setDeleteId(profile.id)}
                      aria-label={`Удалить профиль ${profile.label}`}
                      title="Удалить профиль"
                      className="absolute top-2 right-2 flex h-6 w-6 items-center justify-center rounded-md text-[#627e6c] opacity-0 transition-all group-hover:opacity-100 hover:bg-red-500/15 hover:text-red-300 focus:opacity-100"
                    >
                      <Trash2 size={12} />
                    </button>
                  </div>
                );
              })}
            </div>

            <button
              type="button"
              onClick={startNew}
              className="mt-2.5 flex w-full items-center justify-center gap-1.5 rounded-xl border border-dashed border-white/10 py-2.5 text-[11px] font-semibold text-[#7c9987] transition-colors hover:border-[rgba(var(--accent-rgb),0.35)] hover:bg-[rgba(var(--accent-rgb),0.05)] hover:text-[#c6e9d0]"
            >
              <Plus size={13} />
              Добавить профиль
            </button>
          </Panel>

          <div className="rounded-xl border border-white/6 bg-white/[0.02] p-3">
            <div className="flex items-center gap-2 text-[10px] font-bold tracking-wide text-[#7e9c88] uppercase">
              <FolderOpen size={12} />
              Хранилище
            </div>
            <p className="mt-1.5 font-mono text-[9.5px] leading-relaxed break-all text-[#5f7c6a]">
              %LOCALAPPDATA%\GAYClientLauncher\profiles.json
            </p>
            <button
              type="button"
              onClick={() => {
                copyText("%LOCALAPPDATA%\\GAYClientLauncher\\profiles.json")
                  .then(() => notify("Путь скопирован", "Откройте его в проводнике Windows.", "success"))
                  .catch(() => notify("Не удалось скопировать путь", "Выделите путь вручную.", "warning"));
              }}
              className="btn-ghost mt-2 h-7 px-2 text-[10px]"
            >
              <Copy size={11} />
              Скопировать путь
            </button>
            <p className="mt-2 flex items-start gap-1.5 text-[9.5px] leading-relaxed text-[#5f7c6a]">
              <ShieldCheck size={11} className="mt-px shrink-0 text-[var(--accent)]/60" />
              Старые настройки читаются, но не перезаписываются.
            </p>
          </div>
        </div>

        <Panel className="overflow-hidden">
          <div className="flex flex-wrap items-center justify-between gap-3 border-b border-white/6 px-5 py-4">
            <div className="flex items-center gap-3">
              <span className="flex h-9 w-9 items-center justify-center rounded-xl border border-[rgba(var(--accent-rgb),0.22)] bg-[rgba(var(--accent-rgb),0.08)] text-[var(--accent)]">
                <UserRound size={17} />
              </span>
              <div>
                <div className="flex items-center gap-2">
                  <h3 className="text-sm font-bold text-[#edf9f1]">
                    {isNew ? "Новый профиль" : form.label || "Без названия"}
                  </h3>
                  {dirty && (
                    <span className="rounded border border-amber-300/25 bg-amber-400/10 px-1.5 py-0.5 text-[8.5px] font-bold tracking-wide text-amber-200 uppercase">
                      не сохранено
                    </span>
                  )}
                </div>
                <p className="mt-0.5 text-[10.5px] text-[#76947f]">
                  {isNew ? "Заполните поля и сохраните профиль" : `${current?.createdAt ? `Создан ${formatProfileDate(current.createdAt)} - ` : "Локальный профиль - "}v${current?.version ?? APP_VERSION}`}
                </p>
              </div>
            </div>
            <span className="chip">
              <Gamepad2 size={11} className="text-[var(--accent)]" />
              Minecraft Windows
            </span>
          </div>

          <div className="p-5 sm:p-6">
            <div className="grid gap-4 sm:grid-cols-2">
              <label className="block">
                <span className="field-label">
                  Название профиля
                  <span className="text-[var(--accent)]">*</span>
                </span>
                <span className="relative mt-2 block">
                  <Sparkles size={13} className="pointer-events-none absolute top-1/2 left-3 -translate-y-1/2 text-[#58745f]" />
                  <input
                    ref={labelRef}
                    value={form.label}
                    onChange={(event) => {
                      setForm({ ...form, label: event.target.value });
                      setError(null);
                    }}
                    placeholder="Например, Основной"
                    maxLength={256}
                    className="input-base input-with-icon"
                    aria-invalid={Boolean(error && !form.label.trim())}
                  />
                </span>
                <span className="mt-1.5 block text-[10px] text-[#678271]">Только для лаунчера</span>
              </label>

              <label className="block">
                <span className="field-label">
                  Ник
                  <span className="rounded border border-white/8 bg-white/4 px-1 py-px text-[8px] tracking-wider text-[#7d9a86]">
                    заметка
                  </span>
                </span>
                <span className="relative mt-2 block">
                  <AtSign size={13} className="pointer-events-none absolute top-1/2 left-3 -translate-y-1/2 text-[#58745f]" />
                  <input
                    value={form.nickname}
                    onChange={(event) => setForm({ ...form, nickname: event.target.value })}
                    placeholder="Ваш ник в игре"
                    maxLength={256}
                    className="input-base input-with-icon"
                  />
                </span>
                <span className="mt-1.5 block text-[10px] text-[#678271]">Не меняет имя в игре</span>
              </label>
            </div>

            <div className="mt-4">
              <label className="block">
                <span className="field-label">
                  App ID игры
                  <span className={cn("ml-1 rounded px-1 py-px text-[8px] font-bold", form.aumid ? "bg-[rgba(var(--accent-rgb),0.12)] text-[#b8f0c5]" : "bg-amber-400/10 text-amber-200")}>
                    {form.aumid ? "задан" : "не задан"}
                  </span>
                </span>
                <span className="mt-2 flex flex-col gap-2 sm:flex-row">
                  <span className="relative flex-1">
                    <Hash size={13} className="pointer-events-none absolute top-1/2 left-3 -translate-y-1/2 text-[#58745f]" />
                    <input
                      value={form.aumid}
                      onChange={(event) => {
                        setForm({ ...form, aumid: event.target.value });
                        setError(null);
                      }}
                      placeholder="Package_xxxxx!App"
                      maxLength={512}
                      spellCheck={false}
                      className={cn("input-base input-with-icon font-mono text-[11.5px]", !appIdValid && "border-red-400/50")}
                      aria-invalid={!appIdValid}
                    />
                  </span>
                  <button
                    type="button"
                    onClick={async () => {
                      const aumid = await onDetect();
                      if (aumid) { setForm(current => ({ ...current, aumid })); notify("App ID найден", "Нажмите «Сохранить», чтобы применить его к профилю.", "success"); }
                    }}
                    className="btn-secondary h-[40px] shrink-0 px-3.5 text-[11px]"
                  >
                    <Radar size={14} />
                    Найти игру
                  </button>
                </span>
                <span className="mt-1.5 flex items-start gap-1.5 text-[10px] leading-relaxed text-[#678271]">
                  <Info size={11} className="mt-px shrink-0" />
                  Запустите Minecraft через меню «Пуск» и нажмите «Найти игру», чтобы определить App ID окна.
                </span>
              </label>
            </div>

            <div className="mt-4 rounded-xl border border-white/6 bg-[#07130c]/60 p-3.5">
              <p className="flex items-start gap-2 text-[10.5px] leading-relaxed text-[#8aa791]">
                <Info size={13} className="mt-px shrink-0 text-[#71937c]" />
                Ник сохраняется только как заметка профиля и не влияет на авторизацию. Смена имени
                персонажа в игре и запуск нескольких игровых окон пока не реализованы.
              </p>
            </div>

            {error && (
              <div
                role="alert"
                className="mt-3 flex items-start gap-2 rounded-lg border border-red-400/25 bg-red-500/8 px-3 py-2.5 text-[11px] text-red-200"
              >
                <CircleAlert size={13} className="mt-px shrink-0" />
                {error}
              </div>
            )}
          </div>

          <div className="flex flex-wrap items-center gap-2 border-t border-white/6 bg-black/15 px-5 py-3.5 sm:px-6">
            <button
              type="button"
              onClick={save}
              disabled={!dirty}
              className="btn-primary h-9 px-4 text-[11.5px] disabled:opacity-40"
            >
              <Save size={13} />
              Сохранить
            </button>
            <button type="button" onClick={resetForm} disabled={!dirty} className="btn-secondary h-9 px-3.5 text-[11.5px] disabled:opacity-40">
              Отменить
            </button>
            <button type="button" onClick={duplicate} className="btn-ghost h-9 px-2.5 text-[11px] font-semibold">
              <CopyPlus size={13} />
              Дублировать
            </button>
            <button
              type="button"
              onClick={() => setDeleteId(current?.id ?? null)}
              disabled={profiles.length <= 1}
              className="btn-danger ml-auto h-9 px-3 text-[11px] disabled:cursor-not-allowed disabled:opacity-40"
              title={profiles.length <= 1 ? "Оставьте хотя бы один профиль" : "Удалить профиль"}
            >
              <Trash2 size={12.5} />
              Удалить
            </button>
          </div>
        </Panel>
      </div>

      <Modal
        open={deleteId !== null}
        onClose={() => setDeleteId(null)}
        title="Удалить профиль?"
        description="Аккаунт Minecraft и файлы игры не изменятся. Удаляется только локальная запись профиля."
        icon={<Trash2 size={18} />}
        footer={
          <>
            <button type="button" onClick={() => setDeleteId(null)} className="btn-secondary h-9 px-4 text-xs">
              Отмена
            </button>
            <button
              type="button"
              disabled={profiles.length <= 1 || !deleteTarget}
              onClick={async () => {
                if (deleteTarget && await onDelete(deleteTarget.id)) setDeleteId(null);
              }}
              className="btn-danger h-9 px-4 text-xs"
            >
              <Trash2 size={13} />
              Удалить профиль
            </button>
          </>
        }
      >
        <div className="rounded-xl border border-white/7 bg-black/25 p-3">
          <p className="text-[9px] font-bold tracking-[0.16em] text-[#6d8977] uppercase">Будет удалён</p>
          <p className="mt-1 text-sm font-bold text-[#e8f7ec]">{deleteTarget?.label}</p>
          <p className="mt-0.5 font-mono text-[10px] break-all text-[#718d7b]">{deleteTarget?.aumid || "App ID не задан"}</p>
        </div>
        {profiles.length <= 1 && (
          <p className="mt-3 flex items-center gap-1.5 text-[11px] text-amber-200">
            <CircleAlert size={12} />
            Нужно оставить хотя бы один профиль.
          </p>
        )}
      </Modal>


    </div>
  );
}
