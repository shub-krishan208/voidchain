"use client";

import { ToastProvider } from "@/components/providers/toast-provider";
import { WalletProvider } from "@/components/providers/wallet-provider";

export function AppProviders({ children }: { children: React.ReactNode }) {
  return (
    <ToastProvider>
      <WalletProvider>{children}</WalletProvider>
    </ToastProvider>
  );
}
