"use client";

import * as React from "react";
import * as Tabs from "@radix-ui/react-tabs";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { JsonPreview } from "@/components/ui/json-preview";
import { SectionHeader } from "@/components/ui/section-header";
import { useToast } from "@/components/providers/toast-provider";
import { voidchainClient } from "@/lib/voidchain/client";
import type { Block, BlockHeader, ProofResponse } from "@/lib/voidchain/types";

export default function BlocksPage() {
  const { pushToast } = useToast();
  const [isLoading, setIsLoading] = React.useState(true);
  const [blocks, setBlocks] = React.useState<Block[]>([]);
  const [headers, setHeaders] = React.useState<BlockHeader[]>([]);
  const [txId, setTxId] = React.useState("");
  const [proof, setProof] = React.useState<ProofResponse | null>(null);

  const refresh = React.useCallback(async () => {
    setIsLoading(true);
    try {
      const [blocksRes, headersRes] = await Promise.all([
        voidchainClient.blocks(),
        voidchainClient.headers(),
      ]);
      setBlocks(blocksRes.blocks || []);
      setHeaders(headersRes.headers || []);
    } catch (error) {
      pushToast({
        title: "Failed loading chain data",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "danger",
      });
    } finally {
      setIsLoading(false);
    }
  }, [pushToast]);

  React.useEffect(() => {
    refresh();
  }, [refresh]);

  return (
    <div className="space-y-6">
      <SectionHeader
        title="Blocks Explorer"
        subtitle="Inspect full blocks, compact headers, and Merkle proofs."
      />

      <div className="flex items-center gap-2">
        <Button variant="secondary" onClick={refresh}>
          Refresh chain
        </Button>
      </div>

      {isLoading ? (
        <Card>
          <CardTitle>Loading...</CardTitle>
          <CardDescription>Fetching chain snapshots.</CardDescription>
        </Card>
      ) : (
        <Tabs.Root defaultValue="blocks" className="space-y-4">
          <Tabs.List className="inline-flex rounded-md border border-border bg-surface-1 p-1">
            <Tabs.Trigger
              value="blocks"
              className="rounded px-3 py-1.5 text-sm data-[state=active]:bg-primary-soft data-[state=active]:text-primary-strong"
            >
              Blocks
            </Tabs.Trigger>
            <Tabs.Trigger
              value="headers"
              className="rounded px-3 py-1.5 text-sm data-[state=active]:bg-primary-soft data-[state=active]:text-primary-strong"
            >
              Headers
            </Tabs.Trigger>
            <Tabs.Trigger
              value="proof"
              className="rounded px-3 py-1.5 text-sm data-[state=active]:bg-primary-soft data-[state=active]:text-primary-strong"
            >
              Proof
            </Tabs.Trigger>
          </Tabs.List>

          <Tabs.Content value="blocks">
            <Card>
              <CardTitle>Full Blocks</CardTitle>
              <CardDescription>
                Total blocks: <span className="font-semibold">{blocks.length}</span>
              </CardDescription>
              <JsonPreview className="mt-4" data={blocks} />
            </Card>
          </Tabs.Content>

          <Tabs.Content value="headers">
            <Card>
              <CardTitle>Headers</CardTitle>
              <CardDescription>
                Compact block metadata for quick chain scans.
              </CardDescription>
              <JsonPreview className="mt-4" data={headers} />
            </Card>
          </Tabs.Content>

          <Tabs.Content value="proof" className="space-y-4">
            <Card>
              <CardTitle>Transaction Proof</CardTitle>
              <CardDescription>Find Merkle inclusion proof by transaction id.</CardDescription>
              <div className="mt-4 flex flex-col gap-2 sm:flex-row">
                <Input
                  placeholder="Transaction ID"
                  value={txId}
                  onChange={(event) => setTxId(event.target.value)}
                />
                <Button
                  onClick={async () => {
                    try {
                      const value = txId.trim();
                      if (!value) {
                        throw new Error("Provide a transaction id first.");
                      }
                      const proofData = await voidchainClient.proof(value);
                      setProof(proofData);
                    } catch (error) {
                      setProof(null);
                      pushToast({
                        title: "Proof lookup failed",
                        description:
                          error instanceof Error ? error.message : "Unknown error",
                        variant: "danger",
                      });
                    }
                  }}
                >
                  Fetch proof
                </Button>
              </div>
              <JsonPreview className="mt-4" data={proof || { status: "idle" }} />
            </Card>
          </Tabs.Content>
        </Tabs.Root>
      )}
    </div>
  );
}
