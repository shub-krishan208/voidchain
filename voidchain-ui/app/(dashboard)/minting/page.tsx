"use client";

import * as React from "react";
import * as Tabs from "@radix-ui/react-tabs";
import { useWallet } from "@/components/providers/wallet-provider";
import { useToast } from "@/components/providers/toast-provider";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { DataField, DataFieldGroup } from "@/components/ui/data-field";
import { EmptyState } from "@/components/ui/empty-state";
import { Input } from "@/components/ui/input";
import { SectionHeader } from "@/components/ui/section-header";
import { Textarea } from "@/components/ui/textarea";
import { TransactionItem } from "@/components/ui/transaction-item";
import { truncateAddress } from "@/lib/format";
import { voidchainClient } from "@/lib/voidchain/client";
import type { OwnerResponse, TransactResponse } from "@/lib/voidchain/types";

export default function MintingPage() {
  const { wallet, transactSigned } = useWallet();
  const { pushToast } = useToast();

  const [mintItemId, setMintItemId] = React.useState("");
  const [mintMeta, setMintMeta] = React.useState("");
  const [transferItemId, setTransferItemId] = React.useState("");
  const [transferMeta, setTransferMeta] = React.useState("");
  const [transferTo, setTransferTo] = React.useState("");
  const [ownerLookupId, setOwnerLookupId] = React.useState("");
  const [ownerInfo, setOwnerInfo] = React.useState<OwnerResponse | null>(null);
  const [lastResult, setLastResult] = React.useState<TransactResponse | null>(
    null,
  );

  const ensureWallet = React.useCallback(() => {
    if (!wallet) {
      throw new Error("Connect wallet first.");
    }
    return wallet;
  }, [wallet]);

  return (
    <div className="space-y-6">
      <SectionHeader
        title="Minting & Asset Transfer"
        subtitle="Mint follows chain rule: first claim requires from == to."
      />

      <Card>
        <div className="flex items-center justify-between gap-3">
          <div>
            <CardTitle>Connected Wallet</CardTitle>
            <CardDescription>
              {wallet
                ? truncateAddress(wallet.address, 16, 10)
                : "No wallet connected. Open Wallet tab first."}
            </CardDescription>
          </div>
          {wallet ? <Badge variant="success">Connected</Badge> : null}
        </div>
      </Card>

      <Tabs.Root defaultValue="mint" className="space-y-4">
        <Tabs.List className="inline-flex rounded-md border border-border bg-surface-1 p-1">
          <Tabs.Trigger
            value="mint"
            className="rounded px-3 py-1.5 text-sm data-[state=active]:bg-primary-soft data-[state=active]:text-primary-strong"
          >
            Mint asset
          </Tabs.Trigger>
          <Tabs.Trigger
            value="transfer"
            className="rounded px-3 py-1.5 text-sm data-[state=active]:bg-primary-soft data-[state=active]:text-primary-strong"
          >
            Transfer asset
          </Tabs.Trigger>
          <Tabs.Trigger
            value="owner"
            className="rounded px-3 py-1.5 text-sm data-[state=active]:bg-primary-soft data-[state=active]:text-primary-strong"
          >
            Owner lookup
          </Tabs.Trigger>
        </Tabs.List>

        <Tabs.Content value="mint">
          <Card>
            <CardTitle>Mint</CardTitle>
            <CardDescription>
              Minting submits ASSET with `to` set to your own address.
            </CardDescription>
            <div className="mt-4 space-y-3">
              <Input
                placeholder="Item ID"
                value={mintItemId}
                onChange={(event) => setMintItemId(event.target.value)}
              />
              <Textarea
                placeholder="Metadata"
                value={mintMeta}
                onChange={(event) => setMintMeta(event.target.value)}
              />
              <Button
                disabled={!wallet}
                onClick={async () => {
                  try {
                    const active = ensureWallet();
                    const result = await transactSigned({
                      type: "ASSET",
                      to: active.address,
                      itemId: mintItemId.trim(),
                      meta: mintMeta.trim(),
                    });
                    setLastResult(result as TransactResponse);
                    pushToast({
                      title: "Mint submitted",
                      description: "Asset claim has been sent to mempool.",
                    });
                    setMintItemId("");
                    setMintMeta("");
                  } catch (error) {
                    pushToast({
                      title: "Mint failed",
                      description:
                        error instanceof Error
                          ? error.message
                          : "Unknown error",
                      variant: "danger",
                    });
                  }
                }}
              >
                Mint asset
              </Button>
            </div>
          </Card>
        </Tabs.Content>

        <Tabs.Content value="transfer">
          <Card>
            <CardTitle>Transfer</CardTitle>
            <CardDescription>
              Transfer asset ownership from connected wallet to another address.
            </CardDescription>
            <div className="mt-4 space-y-3">
              <Input
                placeholder="Recipient address (PEM)"
                value={transferTo}
                onChange={(event) => setTransferTo(event.target.value)}
              />
              <Input
                placeholder="Item ID"
                value={transferItemId}
                onChange={(event) => setTransferItemId(event.target.value)}
              />
              <Textarea
                placeholder="Metadata"
                value={transferMeta}
                onChange={(event) => setTransferMeta(event.target.value)}
              />
              <Button
                disabled={!wallet}
                onClick={async () => {
                  try {
                    ensureWallet();
                    const result = await transactSigned({
                      type: "ASSET",
                      to: transferTo.trim(),
                      itemId: transferItemId.trim(),
                      meta: transferMeta.trim(),
                    });
                    setLastResult(result as TransactResponse);
                    pushToast({
                      title: "Transfer submitted",
                      description:
                        "Asset transfer transaction sent to mempool.",
                    });
                    setTransferTo("");
                    setTransferItemId("");
                    setTransferMeta("");
                  } catch (error) {
                    pushToast({
                      title: "Transfer failed",
                      description:
                        error instanceof Error
                          ? error.message
                          : "Unknown error",
                      variant: "danger",
                    });
                  }
                }}
              >
                Transfer asset
              </Button>
            </div>
          </Card>
        </Tabs.Content>

        <Tabs.Content value="owner">
          <Card>
            <CardTitle>Owner Lookup</CardTitle>
            <CardDescription>
              Resolve current owner for an itemId.
            </CardDescription>
            <div className="mt-4 flex flex-col gap-3 sm:flex-row">
              <Input
                placeholder="Item ID"
                value={ownerLookupId}
                onChange={(event) => setOwnerLookupId(event.target.value)}
              />
              <Button
                onClick={async () => {
                  try {
                    const result = await voidchainClient.owner(
                      ownerLookupId.trim(),
                    );
                    setOwnerInfo(result);
                  } catch (error) {
                    setOwnerInfo(null);
                    pushToast({
                      title: "Owner lookup failed",
                      description:
                        error instanceof Error
                          ? error.message
                          : "Unknown error",
                      variant: "danger",
                    });
                  }
                }}
              >
                Lookup
              </Button>
            </div>

            {ownerInfo ? (
              <DataFieldGroup className="mt-4">
                <DataField label="Item ID" value={ownerInfo.itemId} mono />
                <DataField
                  label="Owner"
                  value={truncateAddress(ownerInfo.owner, 16, 10)}
                  mono
                />
              </DataFieldGroup>
            ) : (
              <div className="mt-4 rounded-lg border border-dashed border-border/60 py-6 text-center text-sm text-muted-2">
                Enter an item ID to look up its owner.
              </div>
            )}
          </Card>
        </Tabs.Content>
      </Tabs.Root>

      <Card>
        <CardTitle>Last Asset Transaction Result</CardTitle>
        <CardDescription>
          Server response for your last mint/transfer submission.
        </CardDescription>

        {lastResult ? (
          <div className="mt-4 space-y-2">
            <p className="text-sm font-medium text-success">
              {lastResult.message}
            </p>
            <TransactionItem tx={lastResult.transaction} />
          </div>
        ) : (
          <EmptyState
            className="mt-4"
            message="No mint or transfer submitted yet."
          />
        )}
      </Card>
    </div>
  );
}
