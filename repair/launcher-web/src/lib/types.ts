export type Page = "home" | "profiles" | "settings" | "mods" | "stats";

export type WindowState = "normal" | "maximized" | "minimized" | "closed";

export type LaunchStatus = "idle" | "detected" | "loading" | "loaded" | "error";

export type ToastTone = "success" | "info" | "warning" | "error";

export interface ToastMessage {
  id: number;
  title: string;
  description?: string;
  tone: ToastTone;
}

export interface Profile {
  id: string;
  label: string;
  nickname: string;
  version: string;
  aumid: string;
  createdAt: string;
}

export interface LauncherSettings {
  accent: "emerald" | "lime" | "mint";
  animations: boolean;
  glow: boolean;
  autoConnect: boolean;
  autoUpdate: boolean;
  minimizeOnLaunch: boolean;
  closeToTray: boolean;
}

export interface ModItem {
  id: string;
  name: string;
  description: string;
  author: string;
  version: string;
  category: "Оптимизация" | "Графика" | "Геймплей" | "Интерфейс";
  size: string;
  downloads: string;
  rating: number;
  icon: "zap" | "palette" | "swords" | "layout" | "map" | "shield";
  from: string;
  to: string;
}
