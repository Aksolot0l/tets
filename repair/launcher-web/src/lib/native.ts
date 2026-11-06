import type { LaunchStatus, Profile } from './types';

export type NativeState = { profiles: Profile[]; selectedId: string; writable: boolean; status: LaunchStatus };
type Envelope = { id?: number; ok?: boolean; result?: unknown; error?: string; event?: string; status?: LaunchStatus };
type WebView = { postMessage: (message: object) => void; addEventListener: (event: 'message', handler: (e: MessageEvent<Envelope>) => void) => void };
declare global { interface Window { chrome?: { webview?: WebView } } }
const bridge = window.chrome?.webview;
const pending = new Map<number, { resolve: (value: unknown) => void; reject: (error: Error) => void; timer: number }>();
let serial = 0;
const subscribers = new Set<(status: LaunchStatus) => void>();
bridge?.addEventListener('message', (event) => {
  const message = event.data;
  if (!message || typeof message !== 'object') return;
  if (message.event === 'status' && message.status) { subscribers.forEach(fn => fn(message.status!)); return; }
  if (typeof message.id !== 'number') return;
  const request = pending.get(message.id);
  if (!request) return;
  window.clearTimeout(request.timer); pending.delete(message.id);
  if (message.ok) request.resolve(message.result); else request.reject(new Error(message.error || 'Ошибка клиента'));
});
export function nativeCall<T>(method: string, args: object = {}): Promise<T> {
  if (!bridge) return Promise.reject(new Error('Откройте интерфейс через client.exe.'));
  const id = ++serial;
  return new Promise<T>((resolve, reject) => {
    const timer = window.setTimeout(() => { pending.delete(id); reject(new Error('Клиент не ответил.')); }, 15000);
    pending.set(id, { resolve: value => resolve(value as T), reject, timer });
    try { bridge.postMessage({ id, method, args }); } catch { window.clearTimeout(timer); pending.delete(id); reject(new Error('Не удалось связаться с клиентом.')); }
  });
}
export function onNativeStatus(handler: (status: LaunchStatus) => void) { subscribers.add(handler); return () => { subscribers.delete(handler); }; }
