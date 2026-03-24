"use client";

import * as React from "react";
import { useMining } from "@/components/providers/mining-provider";
import { useWallet } from "@/components/providers/wallet-provider";
import { useToast } from "@/components/providers/toast-provider";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { DataField, DataFieldGroup } from "@/components/ui/data-field";
import { EmptyState } from "@/components/ui/empty-state";
import { Input } from "@/components/ui/input";
import { SectionHeader } from "@/components/ui/section-header";
import { truncateHash, formatTimestamp } from "@/lib/format";
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

  const isHealthy =
    healthStatus.toLowerCase() === "ok" ||
    healthStatus.toLowerCase() === "healthy";

  return (
    <div className="space-y-6">
      <SectionHeader
        title="Mining Console"
        subtitle="Mine blocks with optional reward target address."
      />

      <Card>
        <div className="flex items-center justify-between gap-3">
          <div>
            <CardTitle>Node Health</CardTitle>
            <CardDescription>Current node status</CardDescription>
          </div>
          <Badge variant={isHealthy ? "success" : "danger"}>
            {healthStatus}
          </Badge>
        </div>
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

        {latestMineResult ? (
          <div className="mt-4 space-y-3">
            <p className="text-sm font-medium text-success">
              {latestMineResult.message}
            </p>
            <DataFieldGroup>
              <DataField
                label="Hash"
                value={truncateHash(
                  latestMineResult.new_block.hash,
                  14,
                  10,
                )}
                mono
              />
              <DataField
                label="Previous"
                value={truncateHash(
                  latestMineResult.new_block.last_hash,
                  14,
                  10,
                )}
                mono
              />
              <DataField
                label="Merkle Root"
                value={truncateHash(
                  latestMineResult.new_block.merkle_root,
                  14,
                  10,
                )}
                mono
              />
              <DataField
                label="Nonce"
                value={latestMineResult.new_block.nonce.toLocaleString()}
              />
              <DataField
                label="Difficulty"
                value={latestMineResult.new_block.difficulty}
              />
              <DataField
                label="Transactions"
                value={latestMineResult.new_block.transactions.length}
              />
              <DataField
                label="Timestamp"
                value={formatTimestamp(latestMineResult.new_block.timestamp)}
              />
            </DataFieldGroup>
          </div>
        ) : (
          <EmptyState
            className="mt-4"
            message="No blocks mined yet this session."
          />
        )}
      </Card>

      <Card>
        <CardTitle>Mining History</CardTitle>
        <CardDescription>
          Session history, newest mined block first.
        </CardDescription>

        {miningHistory.length > 0 ? (
          <div className="mt-4 space-y-2">
            {miningHistory.map((result, i) => (
              <div
                key={`${result.new_block.hash}-${i}`}
                className="flex items-center gap-4 rounded-lg border border-border/50 bg-surface-2/40 px-4 py-3"
              >
                <span className="text-lg font-bold text-primary-strong">
                  #{miningHistory.length - i}
                </span>
                <div className="min-w-0 flex-1">
                  <p
                    className="truncate font-mono text-xs text-muted"
                    title={result.new_block.hash}
                  >
                    {truncateHash(result.new_block.hash)}
                  </p>
                  <div className="mt-1 flex flex-wrap items-center gap-x-2 gap-y-0.5 text-[11px] text-muted-2">
                    <span>
                      {result.new_block.transactions.length} txn
                      {result.new_block.transactions.length !== 1 ? "s" : ""}
                    </span>
                    <span>&middot;</span>
                    <span>Diff {result.new_block.difficulty}</span>
                    <span>&middot;</span>
                    <span>
                      {formatTimestamp(result.new_block.timestamp)}
                    </span>
                  </div>
                </div>
              </div>
            ))}
          </div>
        ) : (
          <EmptyState
            className="mt-4"
            message="No mining history this session."
          />
        )}
      </Card>
    </div>
  );
}
