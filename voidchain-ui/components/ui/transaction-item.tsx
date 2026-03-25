"use client";

import { Badge } from "@/components/ui/badge";
import { useToast } from "@/components/providers/toast-provider";
import {
  truncateHash,
  truncateAddress,
  formatAmount,
  formatTimestamp,
} from "@/lib/format";
import type {
  VoidchainTransaction,
  TransactionSummary,
} from "@/lib/voidchain/types";

type AnyTx = VoidchainTransaction | TransactionSummary;

function txId(tx: AnyTx): string {
  return "txId" in tx ? tx.txId : tx.id;
}

export function TransactionItem({ tx }: { tx: AnyTx }) {
  const { pushToast } = useToast();
  const isCurrency = tx.type === "CURRENCY";
  const id = txId(tx);
  const from = tx.from;
  const to = tx.to ?? "";
  const amount =
    "amount" in tx && tx.amount !== undefined ? tx.amount : undefined;
  const itemId =
    "itemId" in tx && tx.itemId !== undefined ? tx.itemId : undefined;
  const meta = "meta" in tx && tx.meta ? tx.meta : undefined;
  const blockHeight =
    "blockHeight" in tx ? (tx as TransactionSummary).blockHeight : undefined;
  const timestamp =
    "timestamp" in tx ? (tx as TransactionSummary).timestamp : undefined;

  return (
    <div className="rounded-lg border border-border/50 bg-surface-1/60 px-4 py-3">
      <div className="flex items-center justify-between gap-3">
        <Badge variant={isCurrency ? "success" : "default"}>{tx.type}</Badge>
        <div className="text-right">
          {amount !== undefined ? (
            <span className="text-sm font-semibold text-foreground">
              {formatAmount(amount)}{" "}
              <span className="text-xs text-muted">VDC</span>
            </span>
          ) : null}
          {itemId ? (
            <span className="text-sm font-medium text-primary-strong">
              {itemId}
            </span>
          ) : null}
        </div>
      </div>

      <div className="mt-1.5 flex items-center gap-1.5 font-mono text-xs text-muted">
        <span title={from}>{truncateAddress(from, 10, 6)}</span>
        <span className="text-primary-strong">&rarr;</span>
        <span title={to}>{to ? truncateAddress(to, 10, 6) : "\u2014"}</span>
      </div>

      {meta ? (
        <p className="mt-1 truncate text-xs text-muted-2">{meta}</p>
      ) : null}

      <div className="mt-1.5 flex flex-wrap items-center gap-x-2 gap-y-0.5 text-[10px] text-muted-2">
        {id ? (
          <button
            type="button"
            className="inline-flex cursor-pointer items-center gap-1 rounded px-1 py-0.5 font-mono transition hover:bg-surface-2 hover:text-foreground"
            title="Copy transaction ID"
            onClick={async () => {
              try {
                await navigator.clipboard.writeText(id);
                pushToast({
                  title: "Transaction ID copied",
                  description: "Full transaction ID copied to clipboard.",
                });
              } catch {
                pushToast({
                  title: "Copy failed",
                  description: "Clipboard access was denied.",
                  variant: "danger",
                });
              }
            }}
          >
            <span title={id}>TX: {truncateHash(id, 6, 4)}</span>
            <svg
              aria-hidden
              viewBox="0 0 16 16"
              className="h-3 w-3 fill-current opacity-80"
            >
              <path d="M3 1.75A1.75 1.75 0 0 1 4.75 0h6.5A1.75 1.75 0 0 1 13 1.75V3h-1.5V1.75a.25.25 0 0 0-.25-.25h-6.5a.25.25 0 0 0-.25.25v8.5a.25.25 0 0 0 .25.25H6V12H4.75A1.75 1.75 0 0 1 3 10.25v-8.5ZM7 5.75A1.75 1.75 0 0 1 8.75 4h6.5A1.75 1.75 0 0 1 17 5.75v8.5A1.75 1.75 0 0 1 15.25 16h-6.5A1.75 1.75 0 0 1 7 14.25v-8.5Zm1.75-.25a.25.25 0 0 0-.25.25v8.5c0 .138.112.25.25.25h6.5a.25.25 0 0 0 .25-.25v-8.5a.25.25 0 0 0-.25-.25h-6.5Z" />
            </svg>
          </button>
        ) : null}
        {blockHeight !== undefined ? <span>&middot; Block #{blockHeight}</span> : null}
        {timestamp !== undefined ? (
          <span>&middot; {formatTimestamp(timestamp)}</span>
        ) : null}
      </div>
    </div>
  );
}
