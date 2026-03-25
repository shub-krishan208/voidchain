"use client";

import * as React from "react";
import * as Tabs from "@radix-ui/react-tabs";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { DataField, DataFieldGroup } from "@/components/ui/data-field";
import { EmptyState } from "@/components/ui/empty-state";
import { Input } from "@/components/ui/input";
import { SectionHeader } from "@/components/ui/section-header";
import { TransactionItem } from "@/components/ui/transaction-item";
import { useToast } from "@/components/providers/toast-provider";
import { truncateHash, formatTimestamp } from "@/lib/format";
import { voidchainClient } from "@/lib/voidchain/client";
import type { Block, BlockHeader, ProofResponse } from "@/lib/voidchain/types";

export default function BlocksPage() {
  const { pushToast } = useToast();
  const [isLoading, setIsLoading] = React.useState(true);
  const [blocks, setBlocks] = React.useState<Block[]>([]);
  const [headers, setHeaders] = React.useState<BlockHeader[]>([]);
  const [txId, setTxId] = React.useState("");
  const [proof, setProof] = React.useState<ProofResponse | null>(null);

  const copyValue = React.useCallback(
    async (value: string, label: string) => {
      try {
        await navigator.clipboard.writeText(value);
        pushToast({
          title: `${label} copied`,
          description: `Full ${label.toLowerCase()} copied to clipboard.`,
        });
      } catch {
        pushToast({
          title: "Copy failed",
          description: "Clipboard access was denied.",
          variant: "danger",
        });
      }
    },
    [pushToast],
  );

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

          {/* ── Blocks ── */}
          <Tabs.Content value="blocks">
            <Card>
              <CardTitle>Full Blocks</CardTitle>
              <CardDescription>
                Total blocks:{" "}
                <span className="font-semibold">{blocks.length}</span>
              </CardDescription>

              {blocks.length === 0 ? (
                <EmptyState
                  className="mt-4"
                  message="No blocks on the chain yet."
                />
              ) : (
                <div className="mt-4 space-y-3">
                  {blocks.map((block, idx) => (
                    <div
                      key={block.hash}
                      className="rounded-lg border border-border/50 bg-surface-2/40"
                    >
                      <div className="flex flex-wrap items-center justify-between gap-2 px-4 py-3">
                        <div className="flex items-center gap-3">
                          <span className="text-lg font-bold text-primary-strong">
                            #{idx}
                          </span>
                          <span
                            className="font-mono text-xs text-muted"
                            title={block.hash}
                          >
                            {truncateHash(block.hash)}
                          </span>
                        </div>
                        <div className="flex items-center gap-3 text-xs">
                          <Badge>
                            {block.transactions.length} txn
                            {block.transactions.length !== 1 ? "s" : ""}
                          </Badge>
                          <span className="text-muted-2">
                            {formatTimestamp(block.timestamp)}
                          </span>
                        </div>
                      </div>

                      <details className="group">
                        <summary className="cursor-pointer border-t border-border/30 px-4 py-2 text-xs text-muted-2 hover:text-muted select-none">
                          <span className="group-open:hidden">
                            Show details &amp; transactions
                          </span>
                          <span className="hidden group-open:inline">
                            Hide details
                          </span>
                        </summary>
                        <div className="border-t border-border/30 px-4 pb-4 pt-3 space-y-3">
                          <DataFieldGroup>
                            <DataField
                              label="Hash"
                              value={truncateHash(block.hash, 16, 12)}
                              mono
                            />
                            <DataField
                              label="Previous"
                              value={truncateHash(block.last_hash, 16, 12)}
                              mono
                            />
                            <DataField
                              label="Merkle Root"
                              value={truncateHash(block.merkle_root, 16, 12)}
                              mono
                            />
                            <DataField
                              label="Nonce"
                              value={block.nonce.toLocaleString()}
                            />
                            <DataField
                              label="Difficulty"
                              value={block.difficulty}
                            />
                          </DataFieldGroup>

                          {block.transactions.length > 0 ? (
                            <div>
                              <h4 className="mb-2 text-xs font-medium uppercase tracking-wider text-muted-2">
                                Transactions
                              </h4>
                              <div className="space-y-2">
                                {block.transactions.map((tx) => (
                                  <TransactionItem key={tx.id} tx={tx} />
                                ))}
                              </div>
                            </div>
                          ) : null}
                        </div>
                      </details>
                    </div>
                  ))}
                </div>
              )}
            </Card>
          </Tabs.Content>

          {/* ── Headers ── */}
          <Tabs.Content value="headers">
            <Card>
              <CardTitle>Headers</CardTitle>
              <CardDescription>
                Compact block metadata for quick chain scans.
              </CardDescription>

              {headers.length === 0 ? (
                <EmptyState
                  className="mt-4"
                  message="No block headers available."
                />
              ) : (
                <div className="mt-4 -mx-5 overflow-x-auto">
                  <table className="w-full min-w-[640px] text-sm">
                    <thead>
                      <tr className="border-b border-border text-left text-xs font-medium uppercase tracking-wider text-muted-2">
                        <th className="px-5 py-2">Height</th>
                        <th className="px-3 py-2">Hash</th>
                        <th className="px-3 py-2">Merkle Root</th>
                        <th className="px-3 py-2 text-right">Diff</th>
                        <th className="px-3 py-2 text-right">Nonce</th>
                        <th className="px-5 py-2 text-right">Timestamp</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-border/40">
                      {headers.map((h) => (
                        <tr
                          key={h.hash}
                          className="text-xs hover:bg-surface-2/50 transition-colors"
                        >
                          <td className="px-5 py-2.5 font-semibold text-primary-strong">
                            #{h.height}
                          </td>
                          <td
                            className="px-3 py-2.5 font-mono text-muted"
                            title={h.hash}
                          >
                            {truncateHash(h.hash, 8, 6)}
                          </td>
                          <td
                            className="px-3 py-2.5 font-mono text-muted"
                            title={h.merkle_root}
                          >
                            {truncateHash(h.merkle_root, 8, 6)}
                          </td>
                          <td className="px-3 py-2.5 text-right">
                            {h.difficulty}
                          </td>
                          <td className="px-3 py-2.5 text-right">
                            {h.nonce.toLocaleString()}
                          </td>
                          <td className="px-5 py-2.5 text-right text-muted-2">
                            {formatTimestamp(h.timestamp)}
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}
            </Card>
          </Tabs.Content>

          {/* ── Proof ── */}
          <Tabs.Content value="proof" className="space-y-4">
            <Card>
              <CardTitle>Transaction Proof</CardTitle>
              <CardDescription>
                Find Merkle inclusion proof by transaction id.
              </CardDescription>
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
                          error instanceof Error
                            ? error.message
                            : "Unknown error",
                        variant: "danger",
                      });
                    }
                  }}
                >
                  Fetch proof
                </Button>
              </div>

              {proof ? (
                <div className="mt-4 space-y-4">
                  <DataFieldGroup>
                    <DataField
                      label="Transaction ID"
                      value={
                        <button
                          type="button"
                          className="inline-flex cursor-pointer items-center gap-1 rounded px-1 py-0.5 transition hover:bg-surface-2 hover:text-foreground"
                          title="Copy transaction ID"
                          onClick={() => copyValue(proof.txId, "Transaction ID")}
                        >
                          <span>{truncateHash(proof.txId, 14, 10)}</span>
                          <svg
                            aria-hidden
                            viewBox="0 0 16 16"
                            className="h-3 w-3 fill-current opacity-80"
                          >
                            <path d="M3 1.75A1.75 1.75 0 0 1 4.75 0h6.5A1.75 1.75 0 0 1 13 1.75V3h-1.5V1.75a.25.25 0 0 0-.25-.25h-6.5a.25.25 0 0 0-.25.25v8.5a.25.25 0 0 0 .25.25H6V12H4.75A1.75 1.75 0 0 1 3 10.25v-8.5ZM7 5.75A1.75 1.75 0 0 1 8.75 4h6.5A1.75 1.75 0 0 1 17 5.75v8.5A1.75 1.75 0 0 1 15.25 16h-6.5A1.75 1.75 0 0 1 7 14.25v-8.5Zm1.75-.25a.25.25 0 0 0-.25.25v8.5c0 .138.112.25.25.25h6.5a.25.25 0 0 0 .25-.25v-8.5a.25.25 0 0 0-.25-.25h-6.5Z" />
                          </svg>
                        </button>
                      }
                      mono
                    />
                    <DataField
                      label="Transaction Hash"
                      value={truncateHash(proof.txHash, 14, 10)}
                      mono
                    />
                    <DataField
                      label="Merkle Root"
                      value={truncateHash(proof.root, 14, 10)}
                      mono
                    />
                  </DataFieldGroup>

                  <div>
                    <h4 className="mb-2 text-xs font-medium uppercase tracking-wider text-muted-2">
                      Containing Block
                    </h4>
                    <DataFieldGroup>
                      <DataField
                        label="Height"
                        value={`#${proof.block.height}`}
                      />
                      <DataField
                        label="Hash"
                        value={truncateHash(proof.block.hash, 14, 10)}
                        mono
                      />
                      <DataField
                        label="Difficulty"
                        value={proof.block.difficulty}
                      />
                    </DataFieldGroup>
                  </div>

                  {proof.proof.length > 0 ? (
                    <div>
                      <h4 className="mb-2 text-xs font-medium uppercase tracking-wider text-muted-2">
                        Proof Path
                      </h4>
                      <div className="space-y-1 rounded-lg border border-border/50 bg-surface-1/50 px-4 py-3">
                        {proof.proof.map((step, i) => (
                          <div
                            key={i}
                            className="flex items-center gap-2 font-mono text-xs"
                          >
                            <span className="w-5 text-right text-muted-2">
                              {i + 1}.
                            </span>
                            <Badge
                              variant={step.isLeft ? "default" : "success"}
                              className="w-14 justify-center text-[10px]"
                            >
                              {step.isLeft ? "LEFT" : "RIGHT"}
                            </Badge>
                            <span className="text-muted" title={step.hash}>
                              {truncateHash(step.hash, 12, 8)}
                            </span>
                          </div>
                        ))}
                      </div>
                    </div>
                  ) : null}

                  <div>
                    <h4 className="mb-2 text-xs font-medium uppercase tracking-wider text-muted-2">
                      Transaction Data
                    </h4>
                    <TransactionItem tx={proof.txData} />
                  </div>
                </div>
              ) : (
                <div className="mt-4 rounded-lg border border-dashed border-border/60 py-8 text-center text-sm text-muted-2">
                  Enter a transaction ID above to fetch its Merkle proof.
                </div>
              )}
            </Card>
          </Tabs.Content>
        </Tabs.Root>
      )}
    </div>
  );
}
