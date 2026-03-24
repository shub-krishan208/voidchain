import { Badge } from "@/components/ui/badge";
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
          <span className="font-mono" title={id}>
            TX: {truncateHash(id, 6, 4)}
          </span>
        ) : null}
        {blockHeight !== undefined ? <span>&middot; Block #{blockHeight}</span> : null}
        {timestamp !== undefined ? (
          <span>&middot; {formatTimestamp(timestamp)}</span>
        ) : null}
      </div>
    </div>
  );
}
