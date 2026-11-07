import { X } from "lucide-react";
import { AlertPixelIcon, CheckPixelIcon, InfoPixelIcon } from "./PixelIcons";
import type { ToastItem, ToastTone } from "../hooks/useClientState";

const TONE_ICONS: Record<ToastTone, typeof CheckPixelIcon> = {
  success: CheckPixelIcon,
  info: InfoPixelIcon,
  warning: AlertPixelIcon,
};

export function ToastHost({
  toasts,
  onDismiss,
}: {
  toasts: ToastItem[];
  onDismiss: (id: number) => void;
}) {
  return (
    <div className="toast-host" aria-live="polite" aria-relevant="additions">
      {toasts.map((toast) => {
        const Icon = TONE_ICONS[toast.tone];
        return (
          <div key={toast.id} className={`toast toast--${toast.tone}`} role="status">
            <span className="toast-icon" aria-hidden="true">
              <Icon size={15} />
            </span>
            <p className="toast-message">{toast.message}</p>
            <button
              type="button"
              className="toast-close"
              onClick={() => onDismiss(toast.id)}
              aria-label="Закрыть уведомление"
            >
              <X size={13} strokeWidth={2.4} />
            </button>
            <span className="toast-life" aria-hidden="true" />
          </div>
        );
      })}
    </div>
  );
}
