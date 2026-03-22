"use client";

import * as React from "react";
import * as Toast from "@radix-ui/react-toast";
import { cn } from "@/lib/cn";

type ToastVariant = "default" | "danger";
type ToastItem = {
  id: string;
  title: string;
  description?: string;
  variant?: ToastVariant;
};

type ToastContextValue = {
  pushToast: (toast: Omit<ToastItem, "id">) => void;
};

const ToastContext = React.createContext<ToastContextValue | null>(null);

function makeId() {
  return `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

export function ToastProvider({ children }: { children: React.ReactNode }) {
  const [items, setItems] = React.useState<ToastItem[]>([]);

  const pushToast = React.useCallback((toast: Omit<ToastItem, "id">) => {
    const id = makeId();
    setItems((prev) => [...prev, { ...toast, id }]);
  }, []);

  return (
    <ToastContext.Provider value={{ pushToast }}>
      <Toast.Provider swipeDirection="right">
        {children}
        {items.map((item) => (
          <Toast.Root
            key={item.id}
            open
            duration={3600}
            onOpenChange={(open) => {
              if (!open) {
                setItems((prev) => prev.filter((x) => x.id !== item.id));
              }
            }}
            className={cn(
              "mb-3 w-[340px] rounded-md border px-4 py-3 shadow-lg",
              item.variant === "danger"
                ? "border-danger bg-surface-1 text-danger"
                : "border-border bg-surface-2 text-foreground",
            )}
          >
            <Toast.Title className="text-sm font-semibold">{item.title}</Toast.Title>
            {item.description ? (
              <Toast.Description className="mt-1 text-xs text-muted">
                {item.description}
              </Toast.Description>
            ) : null}
          </Toast.Root>
        ))}
        <Toast.Viewport className="fixed right-4 top-4 z-50 flex max-w-[360px] flex-col outline-none" />
      </Toast.Provider>
    </ToastContext.Provider>
  );
}

export function useToast() {
  const ctx = React.useContext(ToastContext);
  if (!ctx) {
    throw new Error("useToast must be used inside ToastProvider");
  }
  return ctx;
}
