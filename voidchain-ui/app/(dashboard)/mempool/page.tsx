"use client";

import * as React from "react";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { JsonPreview } from "@/components/ui/json-preview";
import { SectionHeader } from "@/components/ui/section-header";
import { useToast } from "@/components/providers/toast-provider";
import { voidchainClient } from "@/lib/voidchain/client";
import type { VoidchainTransaction } from "@/lib/voidchain/types";

export default function MempoolPage() {
  const { pushToast } = useToast();
  const [isLoading, setIsLoading] = React.useState(true);
  const [transactions, setTransactions] = React.useState<VoidchainTransaction[]>([]);

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
        <CardTitle>Pending Transactions</CardTitle>
        <CardDescription>
          {isLoading
            ? "Loading mempool..."
            : `Current mempool size: ${transactions.length}`}
        </CardDescription>
        <JsonPreview className="mt-4" data={transactions} />
      </Card>
    </div>
  );
}
