import { useCallback, useEffect, useRef, useState, type Dispatch, type SetStateAction } from "react";

const STORAGE_PREFIX = "gayclient.stand.v1";

function isPlainObject(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

export function usePersistedState<T>(key: string, initialValue: T): [T, Dispatch<SetStateAction<T>>] {
  const storageKey = `${STORAGE_PREFIX}.${key}`;
  const [state, setState] = useState<T>(() => {
    try {
      const raw = window.localStorage.getItem(storageKey);
      if (!raw) return initialValue;
      const parsed: unknown = JSON.parse(raw);
      if (isPlainObject(initialValue) && isPlainObject(parsed)) {
        return { ...initialValue, ...parsed } as T;
      }
      if (parsed === null || parsed === undefined) return initialValue;
      return parsed as T;
    } catch {
      return initialValue;
    }
  });

  useEffect(() => {
    try {
      window.localStorage.setItem(storageKey, JSON.stringify(state));
    } catch {
      // Storage can be unavailable in private mode; the UI keeps working in memory.
    }
  }, [state, storageKey]);

  return [state, setState];
}

export function useFps(): number {
  const [fps, setFps] = useState(0);

  useEffect(() => {
    let frame = 0;
    let frames = 0;
    let windowStart = performance.now();
    const samples: number[] = [];

    const tick = (now: number) => {
      frames += 1;
      const elapsed = now - windowStart;
      if (elapsed >= 500) {
        const current = (frames * 1000) / elapsed;
        samples.push(current);
        if (samples.length > 2) samples.shift();
        const average = samples.reduce((sum, sample) => sum + sample, 0) / samples.length;
        setFps(Math.round(average));
        frames = 0;
        windowStart = now;
      }
      frame = window.requestAnimationFrame(tick);
    };

    const start = () => {
      if (frame !== 0) window.cancelAnimationFrame(frame);
      frames = 0;
      windowStart = performance.now();
      frame = window.requestAnimationFrame(tick);
    };

    const stop = () => {
      if (frame !== 0) window.cancelAnimationFrame(frame);
      frame = 0;
    };

    const handleVisibility = () => {
      if (document.hidden) {
        stop();
      } else {
        start();
      }
    };

    start();
    document.addEventListener("visibilitychange", handleVisibility);
    return () => {
      stop();
      document.removeEventListener("visibilitychange", handleVisibility);
    };
  }, []);

  return fps;
}

export type ToastTone = "success" | "info" | "warning";

export type ToastItem = {
  id: number;
  message: string;
  tone: ToastTone;
};

export type ToastApi = {
  toasts: ToastItem[];
  showToast: (message: string, tone?: ToastTone) => void;
  dismissToast: (id: number) => void;
};

export function useToasts(): ToastApi {
  const [toasts, setToasts] = useState<ToastItem[]>([]);
  const counter = useRef(0);
  const timers = useRef<number[]>([]);

  const dismissToast = useCallback((id: number) => {
    setToasts((previous) => previous.filter((toast) => toast.id !== id));
  }, []);

  const showToast = useCallback((message: string, tone: ToastTone = "success") => {
    counter.current += 1;
    const id = counter.current;
    setToasts((previous) => [...previous.slice(-2), { id, message, tone }]);
    const timer = window.setTimeout(() => {
      setToasts((previous) => previous.filter((toast) => toast.id !== id));
    }, 4500);
    timers.current.push(timer);
  }, []);

  useEffect(() => {
    const activeTimers = timers.current;
    return () => {
      activeTimers.forEach((timer) => window.clearTimeout(timer));
    };
  }, []);

  return { toasts, showToast, dismissToast };
}

export function usePrefersReducedMotion(): boolean {
  const [reduced, setReduced] = useState(() => window.matchMedia("(prefers-reduced-motion: reduce)").matches);

  useEffect(() => {
    const query = window.matchMedia("(prefers-reduced-motion: reduce)");
    const listener = (event: MediaQueryListEvent) => setReduced(event.matches);
    query.addEventListener("change", listener);
    return () => query.removeEventListener("change", listener);
  }, []);

  return reduced;
}
