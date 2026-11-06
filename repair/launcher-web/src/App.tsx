import { useCallback, useEffect, useRef, useState } from 'react';
import { AlertTriangle, CheckCircle2, Info, X, XCircle } from 'lucide-react';
import { HomePage } from './components/HomePage';
import { ProfilesPage } from './components/ProfilesPage';
import { SettingsPage } from './components/SettingsPage';
import { MobileNav, navItems, Sidebar } from './components/Sidebar';
import { TopBar } from './components/TopBar';
import { PageHeading, Panel } from './components/ui';
import { APP_VERSION, accentThemes, defaultSettings } from './lib/data';
import { usePersistedState } from './lib/hooks';
import { nativeCall, onNativeStatus, type NativeState } from './lib/native';
import type { LauncherSettings, Page, Profile, ToastMessage, ToastTone } from './lib/types';

const toastIcons = { success: CheckCircle2, info: Info, warning: AlertTriangle, error: XCircle };
function ComingSoon({ kind }: { kind: 'mods' | 'stats' }) {
  return <div className="animate-fade-up space-y-4 pt-1">
    <PageHeading eyebrow={kind === 'mods' ? 'Библиотека' : 'Активность'} title={kind === 'mods' ? 'Моды' : 'Статистика'} description={kind === 'mods' ? 'Управление модами пока не подключено к игре.' : 'Учёт игровых сессий пока не подключён.'} />
    <Panel className="p-6"><p className="text-sm font-bold text-[#d8f5e1]">Пока недоступно</p><p className="mt-2 text-xs text-[#89a792]">Здесь не показываются вымышленные данные. Эта функция появится после отдельной реализации.</p></Panel>
  </div>;
}
export default function App() {
  const [page, setPage] = useState<Page>('home');
  const [native, setNative] = useState<NativeState | null>(null);
  const [loadError, setLoadError] = useState('');
  const [settings, setSettings] = usePersistedState<LauncherSettings>('gayclient.visual.v1', defaultSettings);
  const [toast, setToast] = useState<ToastMessage | null>(null);
  const notify = useCallback((title: string, description?: string, tone: ToastTone = 'info') => setToast({ id: Date.now(), title, description, tone }), []);
  const errorText = (error: unknown) => error instanceof Error ? error.message : 'Неизвестная ошибка';
  useEffect(() => {
    let mounted = true;
    const unsubscribe = onNativeStatus(status => setNative(current => current ? { ...current, status } : current));
    nativeCall<NativeState>('ready').then(value => { if (mounted) setNative(value); }).catch(error => { if (mounted) setLoadError(errorText(error)); });
    return () => { mounted = false; unsubscribe(); };
  }, []);
  useEffect(() => { if (!toast) return; const timer = window.setTimeout(() => setToast(null), 4500); return () => window.clearTimeout(timer); }, [toast]);
  useEffect(() => {
    const theme = accentThemes.find(item => item.id === settings.accent) ?? accentThemes[0];
    for (const [key, value] of Object.entries({ '--accent': theme.accent, '--accent-strong': theme.strong, '--accent-deep': theme.deep, '--accent-rgb': theme.rgb })) document.documentElement.style.setProperty(key, value);
    document.documentElement.dataset.motion = settings.animations ? 'on' : 'off';
  }, [settings.accent, settings.animations]);
  const run = useCallback(async <T,>(method: string, args: object = {}): Promise<T | null> => {
    try { return await nativeCall<T>(method, args); } catch (error) { notify('Не удалось выполнить действие', errorText(error), 'error'); return null; }
  }, [notify]);
  const mutationInFlight = useRef(false);
  const mutate = useCallback(async (method: string, args: object) => {
    if (mutationInFlight.current) return null;
    mutationInFlight.current = true;
    try { return await run<NativeState>(method, args); } finally { mutationInFlight.current = false; }
  }, [run]);
  const launchGame = useCallback(() => { void run('launch'); }, [run]);
  useEffect(() => {
    const key = (event: KeyboardEvent) => {
      if (['INPUT', 'TEXTAREA', 'SELECT'].includes((event.target as HTMLElement | null)?.tagName ?? '') || document.querySelector('[role="dialog"]')) return;
      if (event.ctrlKey && event.key === 'Enter') { event.preventDefault(); launchGame(); }
      else if (!event.ctrlKey && !event.metaKey && !event.altKey && /^[1-5]$/.test(event.key)) { event.preventDefault(); setPage(navItems[Number(event.key) - 1].id); }
    };
    window.addEventListener('keydown', key); return () => window.removeEventListener('keydown', key);
  }, [launchGame]);
  const selectProfile = useCallback(async (id: string) => { const value = await mutate('selectProfile', { id }); if (!value) return false; setNative(value); return true; }, [mutate]);
  const saveProfile = useCallback(async (profile: Profile, isNew: boolean) => { const value = await mutate('saveProfile', { profile, isNew }); if (!value) return false; setNative(value); notify('Профиль сохранён', 'Данные сохранены на компьютере.', 'success'); return true; }, [mutate, notify]);
  const deleteProfile = useCallback(async (id: string) => { const value = await mutate('deleteProfile', { id }); if (!value) return false; setNative(value); notify('Профиль удалён', undefined, 'success'); return true; }, [mutate, notify]);
  const detectGame = useCallback(async () => { const value = await run<{ aumid: string }>('detectGame'); return value?.aumid ?? ''; }, [run]);
  const close = () => { void run('close'); };
  const minimize = () => { void run('minimize'); };
  const maximize = () => { void run('maximize'); };
  if (!native) return <div className="launcher-bg flex min-h-screen items-center justify-center p-6"><Panel className="max-w-md p-8 text-center"><p className="text-lg font-bold text-[#e8f7ec]">{loadError ? 'Не удалось подключиться к клиенту' : 'Подключаем интерфейс...'}</p><p className="mt-3 text-sm text-[#9bb8a4]">{loadError || 'Загружаем локальные профили.'}</p></Panel></div>;
  const activeProfile = native.profiles.find(p => p.id === native.selectedId) ?? native.profiles[0];
  const ToastIcon = toast ? toastIcons[toast.tone] : Info;
  return <div className="launcher-bg relative flex min-h-[100dvh] w-full items-center justify-center overflow-hidden p-2 sm:p-3 lg:p-4">
    {settings.glow && <><div className="aurora top-[-14%] left-[-8%] h-[520px] w-[560px] opacity-70" aria-hidden="true" /><div className="aurora right-[-10%] bottom-[-18%] h-[560px] w-[620px] opacity-55" aria-hidden="true" /></>}
    <div className="shell relative z-10 flex h-[calc(100dvh-20px)] w-full max-w-[1660px] flex-col overflow-hidden rounded-2xl border border-white/8 lg:h-[calc(100dvh-32px)] lg:flex-row">
      <Sidebar page={page} onNavigate={setPage} version={APP_VERSION} online={native.status !== 'error'} />
      <main className="flex min-w-0 flex-1 flex-col overflow-hidden">
        <TopBar page={page} profiles={native.profiles} selectedId={native.selectedId} onSelectProfile={id => void selectProfile(id)} onManageProfiles={() => setPage('profiles')} windowState="normal" onMinimize={minimize} onMaximize={maximize} onClose={close} />
        <MobileNav page={page} onNavigate={setPage} />
        {!native.writable && <div role="alert" className="border-b border-amber-300/25 bg-amber-400/10 px-4 py-2 text-xs text-amber-200">Не удалось прочитать профили. Сохранение отключено, существующие файлы не изменены.</div>}
        <div className="scroll-area min-h-0 flex-1 overflow-y-auto px-3 pb-4 sm:px-5 lg:px-6 lg:pb-5">
          {page === 'home' && <HomePage profile={activeProfile} status={native.status} settings={settings} onLaunch={launchGame} onOpenLog={() => { void run('openLog'); }} onOpenProfiles={() => setPage('profiles')} onOpenMods={() => setPage('mods')} onOpenStats={() => setPage('stats')} />}
          {page === 'profiles' && <ProfilesPage profiles={native.profiles} selectedId={native.selectedId} onSelect={selectProfile} onSave={saveProfile} onDelete={deleteProfile} onDetect={detectGame} notify={notify} />}
          {page === 'settings' && <SettingsPage settings={settings} onChange={setSettings} profiles={native.profiles} onResetSettings={() => setSettings(defaultSettings)} notify={notify} />}
          {page === 'mods' && <ComingSoon kind="mods" />}{page === 'stats' && <ComingSoon kind="stats" />}
        </div>
      </main>
    </div>
    {toast && <div role="status" aria-live="polite" className="fixed right-4 bottom-4 z-[80] flex w-[min(360px,calc(100vw-2rem))] items-start gap-3 rounded-xl border border-white/10 bg-[#132418] p-3.5 shadow-xl"><ToastIcon size={18} className="shrink-0 text-[var(--accent)]" /><div className="flex-1"><p className="text-xs font-bold">{toast.title}</p><p className="mt-1 text-xs text-[#87a490]">{toast.description}</p></div><button onClick={() => setToast(null)} aria-label="Скрыть уведомление"><X size={14} /></button></div>}
  </div>;
}
