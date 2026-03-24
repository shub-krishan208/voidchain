"use client";

import * as React from "react";
import * as Dialog from "@radix-ui/react-dialog";
import { useToast } from "@/components/providers/toast-provider";
import { useWallet } from "@/components/providers/wallet-provider";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardDescription, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { JsonPreview } from "@/components/ui/json-preview";
import { SectionHeader } from "@/components/ui/section-header";
import { Textarea } from "@/components/ui/textarea";
import { voidchainClient } from "@/lib/voidchain/client";
import type { TransactionsResponse, WalletInfo } from "@/lib/voidchain/types";

function shortAddress(value: string) {
  if (value.length < 24) {
    return value;
  }
  return `${value.slice(0, 14)}...${value.slice(-10)}`;
}

function normalizeAddressInput(value: string) {
  return value
    .replaceAll("\\r\\n", "\n")
    .replaceAll("\\n", "\n")
    .replaceAll("\r\n", "\n")
    .replaceAll("\r", "\n")
    .trim();
}

export default function WalletPage() {
  const { pushToast } = useToast();
  const {
    wallet,
    hasVault,
    storedAddress,
    createWalletWithPasskey,
    recoverWalletWithPasskey,
    connectWithPasskey,
    disconnect,
    forgetWallet,
    transactSigned,
  } = useWallet();

  const [walletLabel, setWalletLabel] = React.useState("VoidChain Wallet");
  const [recoverSecret, setRecoverSecret] = React.useState("");
  const [isBusy, setIsBusy] = React.useState(false);
  const [showSecret, setShowSecret] = React.useState<string | null>(null);
  const [walletInfo, setWalletInfo] = React.useState<WalletInfo | null>(null);
  const [txHistory, setTxHistory] = React.useState<TransactionsResponse | null>(null);

  const [sendTo, setSendTo] = React.useState("");
  const [sendAmount, setSendAmount] = React.useState("1");

  const refreshWalletData = React.useCallback(async () => {
    if (!wallet?.address) {
      setWalletInfo(null);
      setTxHistory(null);
      return;
    }
    try {
      const [info, history] = await Promise.all([
        voidchainClient.walletInfo(wallet.address),
        voidchainClient.transactions(wallet.address),
      ]);
      setWalletInfo(info);
      setTxHistory(history);
    } catch (error) {
      pushToast({
        title: "Failed loading wallet data",
        description: error instanceof Error ? error.message : "Unknown error",
        variant: "danger",
      });
    }
  }, [pushToast, wallet?.address]);

  React.useEffect(() => {
    refreshWalletData();
  }, [refreshWalletData]);

  return (
    <div className="space-y-6">
      <SectionHeader
        title="Wallet Operations"
        subtitle="Passkey-powered connect, recover, and signed value transfers."
      />

      <Card>
        <CardTitle>Wallet Status</CardTitle>
        <CardDescription>
          {wallet
            ? `Connected: ${shortAddress(wallet.address)}`
            : hasVault
              ? `Vault ready: ${shortAddress(storedAddress || "")}`
              : "No passkey wallet configured yet."}
        </CardDescription>
        <div className="mt-4 flex flex-wrap items-center gap-2">
          {wallet ? (
            <>
              <Badge variant="success">Connected</Badge>
              <Button variant="secondary" onClick={disconnect}>
                Disconnect
              </Button>
              <Button
                variant="danger"
                onClick={() => {
                  forgetWallet();
                  pushToast({
                    title: "Wallet vault removed",
                    description: "Stored passkey wallet metadata has been deleted.",
                  });
                }}
              >
                Forget vault
              </Button>
            </>
          ) : (
            <>
              {hasVault ? (
                <Button
                  onClick={async () => {
                    setIsBusy(true);
                    try {
                      await connectWithPasskey();
                      pushToast({
                        title: "Wallet unlocked",
                        description: "Passkey verification succeeded.",
                      });
                    } catch (error) {
                      pushToast({
                        title: "Unlock failed",
                        description:
                          error instanceof Error ? error.message : "Unknown error",
                        variant: "danger",
                      });
                    } finally {
                      setIsBusy(false);
                    }
                  }}
                  disabled={isBusy}
                >
                  Connect with passkey
                </Button>
              ) : null}
            </>
          )}
        </div>
      </Card>

      <div className="grid gap-4 lg:grid-cols-2">
        <Card>
          <CardTitle>Create New Wallet</CardTitle>
          <CardDescription>
            Generates a new wallet and binds it to a new passkey.
          </CardDescription>
          <div className="mt-4 space-y-3">
            <Input
              value={walletLabel}
              onChange={(event) => setWalletLabel(event.target.value)}
              placeholder="Passkey display label"
            />
            <Button
              disabled={isBusy}
              onClick={async () => {
                setIsBusy(true);
                try {
                  const next = await createWalletWithPasskey(walletLabel.trim());
                  setShowSecret(next.secretKey);
                  pushToast({
                    title: "Wallet created",
                    description: "Passkey and encrypted vault are now configured.",
                  });
                  refreshWalletData();
                } catch (error) {
                  pushToast({
                    title: "Create wallet failed",
                    description:
                      error instanceof Error ? error.message : "Unknown error",
                    variant: "danger",
                  });
                } finally {
                  setIsBusy(false);
                }
              }}
            >
              Create + secure wallet
            </Button>
          </div>
        </Card>

        <Card>
          <CardTitle>Recover Existing Wallet</CardTitle>
          <CardDescription>
            Import by `secretKey`, then secure with a passkey vault.
          </CardDescription>
          <div className="mt-4 space-y-3">
            <Textarea
              value={recoverSecret}
              onChange={(event) => setRecoverSecret(event.target.value)}
              placeholder="64-char private key hex"
            />
            <Button
              disabled={isBusy}
              onClick={async () => {
                setIsBusy(true);
                try {
                  await recoverWalletWithPasskey(recoverSecret.trim(), walletLabel.trim());
                  setRecoverSecret("");
                  pushToast({
                    title: "Wallet recovered",
                    description: "Passkey vault now protects your recovered wallet.",
                  });
                  refreshWalletData();
                } catch (error) {
                  pushToast({
                    title: "Recover failed",
                    description:
                      error instanceof Error ? error.message : "Unknown error",
                    variant: "danger",
                  });
                } finally {
                  setIsBusy(false);
                }
              }}
            >
              Recover + secure wallet
            </Button>
          </div>
        </Card>
      </div>

      <Card>
        <CardTitle>Send Currency</CardTitle>
        <CardDescription>
          Signed via `/transact/signed` using your unlocked wallet key.
        </CardDescription>
        <div className="mt-4 grid gap-3 md:grid-cols-[1fr_180px] md:grid-rows-[auto_auto]">
          <Textarea
            placeholder="Recipient address (PEM)"
            className="md:row-span-2"
            value={sendTo}
            onChange={(event) => setSendTo(event.target.value)}
          />
          <Input
            placeholder="Amount"
            type="number"
            min="0"
            step="0.0001"
            value={sendAmount}
            onChange={(event) => setSendAmount(event.target.value)}
          />
          <Button
            disabled={!wallet}
            onClick={async () => {
              try {
                if (!wallet) {
                  throw new Error("Connect your wallet first.");
                }
                const recipientAddress = normalizeAddressInput(sendTo);
                if (!recipientAddress) {
                  throw new Error("Recipient address is required.");
                }
                const amount = Number(sendAmount);
                if (!Number.isFinite(amount) || amount <= 0) {
                  throw new Error("Amount must be greater than zero.");
                }
                await transactSigned({
                  type: "CURRENCY",
                  to: recipientAddress,
                  amount,
                });
                pushToast({
                  title: "Transaction submitted",
                  description: "Currency transfer added to mempool.",
                });
                setSendTo("");
                setSendAmount("1");
                refreshWalletData();
              } catch (error) {
                pushToast({
                  title: "Send failed",
                  description:
                    error instanceof Error ? error.message : "Unknown error",
                  variant: "danger",
                });
              }
            }}
          >
            Send
          </Button>
        </div>
      </Card>

      <div className="grid gap-4 lg:grid-cols-2">
        <Card>
          <CardTitle>Wallet Snapshot</CardTitle>
          <CardDescription>Balance, assets, and recent chain activity.</CardDescription>
          <JsonPreview className="mt-4" data={walletInfo || { status: "idle" }} />
        </Card>
        <Card>
          <CardTitle>Transaction History</CardTitle>
          <CardDescription>Full transaction list for connected address.</CardDescription>
          <JsonPreview className="mt-4" data={txHistory || { status: "idle" }} />
        </Card>
      </div>

      <Dialog.Root open={Boolean(showSecret)} onOpenChange={() => setShowSecret(null)}>
        <Dialog.Portal>
          <Dialog.Overlay className="fixed inset-0 bg-black/70" />
          <Dialog.Content className="fixed left-1/2 top-1/2 w-[92vw] max-w-2xl -translate-x-1/2 -translate-y-1/2 rounded-xl border border-border bg-surface-1 p-6 shadow-xl">
            <Dialog.Title className="text-lg font-semibold text-primary-strong">
              Save your wallet secret key now
            </Dialog.Title>
            <Dialog.Description className="mt-1 text-sm text-muted">
              This is shown once for backup. You still unlock daily usage with your
              passkey.
            </Dialog.Description>
            <pre className="mt-4 overflow-auto rounded-md border border-border bg-surface-2 p-3 font-mono text-xs text-foreground">
              {showSecret}
            </pre>
            <div className="mt-4 flex justify-end">
              <Dialog.Close asChild>
                <Button variant="secondary">Close</Button>
              </Dialog.Close>
            </div>
          </Dialog.Content>
        </Dialog.Portal>
      </Dialog.Root>
    </div>
  );
}
