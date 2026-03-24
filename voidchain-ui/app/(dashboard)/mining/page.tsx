"use client";

import * as React from "react";
import { useMining } from "@/components/providers/mining-provider";
import { useWallet } from "@/components/providers/wallet-provider";
import { useToast } from "@/components/providers/toast-provider";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { JsonPreview } from "@/components/ui/json-preview";
import { SectionHeader } from "@/components/ui/section-header";
import { voidchainClient } from "@/lib/voidchain/client";

function normalizeAddressInput(value: string) {
  const normalized = value
    .replaceAll("\\r\\n", "\n")
    .replaceAll("\\n", "\n")
    .replaceAll("\r\n", "\n")
    .replaceAll("\r", "\n")
    .trim();

  const begin = "-----BEGIN PUBLIC KEY-----";
  const end = "-----END PUBLIC KEY-----";
  const beginPos = normalized.indexOf(begin);
  const endPos = normalized.indexOf(end, beginPos + begin.length);
  if (beginPos === -1 || endPos === -1 || endPos <= beginPos) {
    return normalized;
  }

  const bodyRaw = normalized.slice(beginPos + begin.length, endPos);
  const body = bodyRaw.replace(/\s+/g, "");
  if (!body) {
    return normalized;
  }

  const lines = body.match(/.{1,64}/g) || [];
  return `${begin}\n${lines.join("\n")}\n${end}`;
}

export default function MiningPage() {
  const { wallet } = useWallet();
  const { miningHistory, addMineResult } = useMining();
  const { pushToast } = useToast();
  const [isMining, setIsMining] = React.useState(false);
  const [minerAddress, setMinerAddress] = React.useState("");
  const [healthStatus, setHealthStatus] = React.useState("unknown");
  const latestMineResult = miningHistory[0] || null;

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
                    normalizeAddressInput(minerAddress) || undefined,
                  );
                  addMineResult(result);
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
        <JsonPreview
          className="mt-4"
          data={latestMineResult || { status: "idle" }}
        />
      </Card>

      <Card>
        <CardTitle>Mining History</CardTitle>
        <CardDescription>Session history, newest mined block first.</CardDescription>
        <JsonPreview
          className="mt-4"
          data={miningHistory.length > 0 ? miningHistory : { status: "idle" }}
        />
      </Card>
    </div>
  );
}
