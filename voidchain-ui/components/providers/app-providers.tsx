"use client";

import { ToastProvider } from "@/components/providers/toast-provider";
import { MiningProvider } from "@/components/providers/mining-provider";
import { WalletProvider } from "@/components/providers/wallet-provider";

export function AppProviders({ children }: { children: React.ReactNode }) {
  return (
    <ToastProvider>
      <WalletProvider>
        <MiningProvider>{children}</MiningProvider>
      </WalletProvider>
    </ToastProvider>
  );
}
