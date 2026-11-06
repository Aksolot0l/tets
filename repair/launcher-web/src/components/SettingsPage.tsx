import { Check, FolderOpen, Moon, Palette, RotateCcw, WandSparkles } from 'lucide-react';
import { accentThemes } from '../lib/data';
import type { LauncherSettings, Profile, ToastTone } from '../lib/types';
import { PageHeading, Panel, SectionTitle, SettingRow } from './ui';

export function SettingsPage({ settings, onChange, profiles, onResetSettings, notify }: {
  settings: LauncherSettings; onChange: (settings: LauncherSettings) => void; profiles: Profile[];
  onResetSettings: () => void; notify: (title: string, description?: string, tone?: ToastTone) => void;
}) {
  return <div className="animate-fade-up space-y-4 pt-1">
    <PageHeading eyebrow="Персонализация" title="Настройки лаунчера" description="Настройки оформления сохраняются локально на этом компьютере." />
    <div className="grid gap-4 xl:grid-cols-2">
      <Panel className="p-5"><SectionTitle title="Оформление" hint="Измените цвет и анимацию интерфейса." icon={<Palette size={15}/>} />
        <div className="mt-4 grid grid-cols-3 gap-2">{accentThemes.map(theme => <button key={theme.id} type="button" onClick={() => onChange({ ...settings, accent: theme.id })} className="panel-soft relative rounded-xl p-3 text-left">
          <span className="block h-7 rounded-lg" style={{ background: theme.swatch }}/><span className="mt-2 block text-xs font-bold">{theme.name}</span>{settings.accent === theme.id && <Check size={14} className="absolute right-2 top-2 text-white"/>}
        </button>)}</div>
        <div className="mt-3 divide-y divide-white/5"><SettingRow icon={<WandSparkles size={14}/>} title="Плавные анимации" description="Переходы и движение маскота." checked={settings.animations} onChange={value => onChange({ ...settings, animations: value })}/><SettingRow icon={<Moon size={14}/>} title="Фоновая аврора" description="Мягкое свечение на фоне." checked={settings.glow} onChange={value => onChange({ ...settings, glow: value })}/></div>
        <button type="button" className="btn-secondary mt-4 h-9 px-3 text-xs" onClick={() => { onResetSettings(); notify('Оформление сброшено', undefined, 'success'); }}><RotateCcw size={13}/> Сбросить оформление</button>
      </Panel>
      <Panel className="p-5"><SectionTitle title="Профили и подключение" hint="Данные на этом устройстве." icon={<FolderOpen size={15}/>} />
        <div className="mt-4 rounded-xl border border-white/7 bg-[#06110b]/70 p-4 text-xs leading-relaxed text-[#9bb8a4]"><p>Профилей: <strong className="text-[#e8f7ec]">{profiles.length}</strong></p><p className="mt-2 break-all">%LOCALAPPDATA%\\GAYClientLauncher\\profiles.json</p><p className="mt-3">Клиент автоматически ожидает одно окно Minecraft. Не закрывайте лаунчер во время игры.</p><p className="mt-3">Обновление выполняется отдельным установщиком. Автоматическая проверка обновлений и сворачивание в трей пока не реализованы.</p></div>
      </Panel>
    </div>
  </div>;
}
