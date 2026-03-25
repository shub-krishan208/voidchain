"use client";

import * as React from "react";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { EmptyState } from "@/components/ui/empty-state";
import { SectionHeader } from "@/components/ui/section-header";
import { TransactionItem } from "@/components/ui/transaction-item";
import { useToast } from "@/components/providers/toast-provider";
import { voidchainClient } from "@/lib/voidchain/client";
import type { VoidchainTransaction } from "@/lib/voidchain/types";

export default function MempoolPage() {
  const { pushToast } = useToast();
  const [isLoading, setIsLoading] = React.useState(true);
  const [transactions, setTransactions] = React.useState<
    VoidchainTransaction[]
  >([]);

  const refresh = React.useCallback(async () => {
    setIsLoading(true);
    try {
      const pool = await voidchainClient.pool();
      setTransactions(pool.transactions || []);
    } catch (error) {
      pushToast({
        title: "Failed loading mempool",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "danger",
      });
    } finally {
      setIsLoading(false);
    }
  }, [pushToast]);

  React.useEffect(() => {
    refresh();
    const handle = setInterval(refresh, 10000);
    return () => clearInterval(handle);
  }, [refresh]);

  const currencyCount = transactions.filter(
    (t) => t.type === "CURRENCY",
  ).length;
  const assetCount = transactions.filter((t) => t.type === "ASSET").length;

  return (
    <div className="space-y-6">
      <SectionHeader
        title="Mempool"
        subtitle="Live pending transactions queued for mining."
      />
      <div className="flex items-center gap-2">
        <Button variant="secondary" onClick={refresh}>
          Refresh pool
        </Button>
      </div>

      <Card>
        <div className="flex flex-wrap items-center justify-between gap-3">
          <div>
            <CardTitle>Pending Transactions</CardTitle>
            <CardDescription>
              {isLoading
                ? "Loading mempool..."
                : `${transactions.length} transaction${transactions.length !== 1 ? "s" : ""} in pool`}
            </CardDescription>
          </div>
          {!isLoading && transactions.length > 0 ? (
            <div className="flex items-center gap-2">
              {currencyCount > 0 ? (
                <Badge variant="success">{currencyCount} currency</Badge>
              ) : null}
              {assetCount > 0 ? (
                <Badge variant="default">{assetCount} asset</Badge>
              ) : null}
            </div>
          ) : null}
        </div>

        {isLoading ? null : transactions.length === 0 ? (
          <EmptyState
            className="mt-4"
            message="Mempool is empty — no pending transactions."
          />
        ) : (
          <div className="mt-4 space-y-2">
            {transactions.map((tx) => (
              <TransactionItem key={tx.id} tx={tx} />
            ))}
          </div>
        )}
      </Card>
    </div>
  );
}
