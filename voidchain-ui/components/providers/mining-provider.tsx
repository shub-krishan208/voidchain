"use client";

import * as React from "react";
import type { MineResponse } from "@/lib/voidchain/types";

const MINING_HISTORY_STORAGE_KEY = "voidchain.mining.history.v1";

type MiningContextValue = {
  miningHistory: MineResponse[];
  addMineResult: (result: MineResponse) => void;
  clearMiningHistory: () => void;
};

const MiningContext = React.createContext<MiningContextValue | null>(null);

function parseHistory(raw: string): MineResponse[] {
  try {
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed)) {
      return [];
    }
    return parsed as MineResponse[];
  } catch {
    return [];
  }
}

export function MiningProvider({ children }: { children: React.ReactNode }) {
  const [miningHistory, setMiningHistory] = React.useState<MineResponse[]>([]);
  const [isHydrated, setIsHydrated] = React.useState(false);

  React.useEffect(() => {
    const stored = sessionStorage.getItem(MINING_HISTORY_STORAGE_KEY);
    if (stored) {
      setMiningHistory(parseHistory(stored));
    }
    setIsHydrated(true);
  }, []);

  React.useEffect(() => {
    if (!isHydrated) {
      return;
    }
    sessionStorage.setItem(
      MINING_HISTORY_STORAGE_KEY,
      JSON.stringify(miningHistory),
    );
  }, [isHydrated, miningHistory]);

  const addMineResult = React.useCallback((result: MineResponse) => {
    setMiningHistory((previous) => [result, ...previous]);
  }, []);

  const clearMiningHistory = React.useCallback(() => {
    setMiningHistory([]);
    sessionStorage.removeItem(MINING_HISTORY_STORAGE_KEY);
  }, []);

  return (
    <MiningContext.Provider
      value={{ miningHistory, addMineResult, clearMiningHistory }}
    >
      {children}
    </MiningContext.Provider>
  );
}

export function useMining() {
  const ctx = React.useContext(MiningContext);
  if (!ctx) {
    throw new Error("useMining must be used inside MiningProvider");
  }
  return ctx;
}
