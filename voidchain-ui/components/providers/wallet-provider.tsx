"use client";

import * as React from "react";
import { clearVault, provisionWalletVault, readVault, unlockWalletVault } from "@/lib/passkey-vault";
import {
  type SignedTxnPayload,
  type UnsignedSignedTxnPayload,
  voidchainClient,
} from "@/lib/voidchain/client";

type ActiveWallet = {
  address: string;
  secretKey: string;
};

type WalletContextValue = {
  wallet: ActiveWallet | null;
  hasVault: boolean;
  storedAddress: string | null;
  createWalletWithPasskey: (label?: string) => Promise<ActiveWallet>;
  recoverWalletWithPasskey: (
    secretKey: string,
    label?: string,
  ) => Promise<ActiveWallet>;
  connectWithPasskey: () => Promise<ActiveWallet>;
  disconnect: () => void;
  forgetWallet: () => void;
  transactSigned: (payload: UnsignedSignedTxnPayload) => Promise<unknown>;
};

const WalletContext = React.createContext<WalletContextValue | null>(null);

export function WalletProvider({ children }: { children: React.ReactNode }) {
  const [wallet, setWallet] = React.useState<ActiveWallet | null>(null);
  const [hasVault, setHasVault] = React.useState(false);
  const [storedAddress, setStoredAddress] = React.useState<string | null>(null);

  React.useEffect(() => {
    const vault = readVault();
    setHasVault(Boolean(vault));
    setStoredAddress(vault?.address || null);
  }, []);

  const syncVaultState = React.useCallback(() => {
    const vault = readVault();
    setHasVault(Boolean(vault));
    setStoredAddress(vault?.address || null);
  }, []);

  const createWalletWithPasskey = React.useCallback(
    async (label?: string) => {
      const materials = await voidchainClient.walletGenerate();
      await provisionWalletVault(materials.address, materials.secretKey, label);
      const active = { address: materials.address, secretKey: materials.secretKey };
      setWallet(active);
      syncVaultState();
      return active;
    },
    [syncVaultState],
  );

  const recoverWalletWithPasskey = React.useCallback(
    async (secretKey: string, label?: string) => {
      const recovered = await voidchainClient.walletRecover(secretKey);
      await provisionWalletVault(recovered.address, secretKey, label);
      const active = { address: recovered.address, secretKey };
      setWallet(active);
      syncVaultState();
      return active;
    },
    [syncVaultState],
  );

  const connectWithPasskey = React.useCallback(async () => {
    const unlocked = await unlockWalletVault();
    setWallet(unlocked);
    syncVaultState();
    return unlocked;
  }, [syncVaultState]);

  const disconnect = React.useCallback(() => {
    setWallet(null);
  }, []);

  const forgetWallet = React.useCallback(() => {
    clearVault();
    setWallet(null);
    syncVaultState();
  }, [syncVaultState]);

  const transactSigned = React.useCallback(
    async (payload: UnsignedSignedTxnPayload) => {
      if (!wallet) {
        throw new Error("Connect wallet first.");
      }
      return voidchainClient.transactSigned({
        ...payload,
        secretKey: wallet.secretKey,
      } as SignedTxnPayload);
    },
    [wallet],
  );

  return (
    <WalletContext.Provider
      value={{
        wallet,
        hasVault,
        storedAddress,
        createWalletWithPasskey,
        recoverWalletWithPasskey,
        connectWithPasskey,
        disconnect,
        forgetWallet,
        transactSigned,
      }}
    >
      {children}
    </WalletContext.Provider>
  );
}

export function useWallet() {
  const ctx = React.useContext(WalletContext);
  if (!ctx) {
    throw new Error("useWallet must be used inside WalletProvider");
  }
  return ctx;
}
