"use client";

import * as React from "react";
import { useWallet } from "@/components/providers/wallet-provider";
import { useToast } from "@/components/providers/toast-provider";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { JsonPreview } from "@/components/ui/json-preview";
import { SectionHeader } from "@/components/ui/section-header";
import { voidchainClient } from "@/lib/voidchain/client";
import type { MineResponse } from "@/lib/voidchain/types";

export default function MiningPage() {
  const { wallet } = useWallet();
  const { pushToast } = useToast();
  const [isMining, setIsMining] = React.useState(false);
  const [minerAddress, setMinerAddress] = React.useState("");
  const [mineResult, setMineResult] = React.useState<MineResponse | null>(null);
  const [healthStatus, setHealthStatus] = React.useState("unknown");

  React.useEffect(() => {
    if (wallet?.address) {
      setMinerAddress(wallet.address);
    }
  }, [wallet?.address]);

  const refreshHealth = React.useCallback(async () => {
    try {
      const status = await voidchainClient.health();
      setHealthStatus(status.status);
    } catch {
      setHealthStatus("DOWN");
    }
  }, []);

  React.useEffect(() => {
    refreshHealth();
  }, [refreshHealth]);

  return (
    <div className="space-y-6">
      <SectionHeader
        title="Mining Console"
        subtitle="Mine blocks with optional reward target address."
      />

      <Card>
        <CardTitle>Node Health</CardTitle>
        <CardDescription>Status: {healthStatus}</CardDescription>
      </Card>

      <Card>
        <CardTitle>Mine Block</CardTitle>
        <CardDescription>
          If minerAddress is omitted, the node wallet receives the reward.
        </CardDescription>
        <div className="mt-4 flex flex-col gap-3">
          <Input
            placeholder="Miner address (PEM)"
            value={minerAddress}
            onChange={(event) => setMinerAddress(event.target.value)}
          />
          <div className="flex flex-wrap items-center gap-2">
            <Button
              disabled={isMining}
              onClick={async () => {
                setIsMining(true);
                try {
                  const result = await voidchainClient.mine(
                    minerAddress.trim() || undefined,
                  );
                  setMineResult(result);
                  pushToast({
                    title: "Block mined",
                    description: "New block appended and broadcasted.",
                  });
                  refreshHealth();
                } catch (error) {
                  pushToast({
                    title: "Mine failed",
                    description:
                      error instanceof Error ? error.message : "Unknown error",
                    variant: "danger",
                  });
                } finally {
                  setIsMining(false);
                }
              }}
            >
              {isMining ? "Mining..." : "Mine now"}
            </Button>
            {wallet?.address ? (
              <Button
                variant="secondary"
                onClick={() => setMinerAddress(wallet.address)}
              >
                Use connected wallet address
              </Button>
            ) : null}
          </div>
        </div>
      </Card>

      <Card>
        <CardTitle>Mining Result</CardTitle>
        <CardDescription>Last mined block payload.</CardDescription>
        <JsonPreview className="mt-4" data={mineResult || { status: "idle" }} />
      </Card>
    </div>
  );
}
