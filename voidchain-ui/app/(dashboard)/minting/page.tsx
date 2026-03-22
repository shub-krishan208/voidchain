"use client";

import * as React from "react";
import * as Tabs from "@radix-ui/react-tabs";
import { useWallet } from "@/components/providers/wallet-provider";
import { useToast } from "@/components/providers/toast-provider";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { JsonPreview } from "@/components/ui/json-preview";
import { SectionHeader } from "@/components/ui/section-header";
import { Textarea } from "@/components/ui/textarea";
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
  const [lastResult, setLastResult] = React.useState<TransactResponse | null>(null);

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
        <CardTitle>Connected Wallet</CardTitle>
        <CardDescription>
          {wallet ? wallet.address : "No wallet connected. Open Wallet tab first."}
        </CardDescription>
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
                        error instanceof Error ? error.message : "Unknown error",
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
                      description: "Asset transfer transaction sent to mempool.",
                    });
                    setTransferTo("");
                    setTransferItemId("");
                    setTransferMeta("");
                  } catch (error) {
                    pushToast({
                      title: "Transfer failed",
                      description:
                        error instanceof Error ? error.message : "Unknown error",
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
            <CardDescription>Resolve current owner for an itemId.</CardDescription>
            <div className="mt-4 flex flex-col gap-3 sm:flex-row">
              <Input
                placeholder="Item ID"
                value={ownerLookupId}
                onChange={(event) => setOwnerLookupId(event.target.value)}
              />
              <Button
                onClick={async () => {
                  try {
                    const result = await voidchainClient.owner(ownerLookupId.trim());
                    setOwnerInfo(result);
                  } catch (error) {
                    setOwnerInfo(null);
                    pushToast({
                      title: "Owner lookup failed",
                      description:
                        error instanceof Error ? error.message : "Unknown error",
                      variant: "danger",
                    });
                  }
                }}
              >
                Lookup
              </Button>
            </div>
            <JsonPreview className="mt-4" data={ownerInfo || { status: "idle" }} />
          </Card>
        </Tabs.Content>
      </Tabs.Root>

      <Card>
        <CardTitle>Last Asset Transaction Result</CardTitle>
        <CardDescription>Server response for your last mint/transfer submission.</CardDescription>
        <JsonPreview className="mt-4" data={lastResult || { status: "idle" }} />
      </Card>
    </div>
  );
}
