"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/cn";
import { useToast } from "@/components/providers/toast-provider";
import { useWallet } from "@/components/providers/wallet-provider";

const navItems = [
  { href: "/blocks", label: "Blocks" },
  { href: "/mempool", label: "Mempool" },
  { href: "/mining", label: "Mining" },
  { href: "/wallet", label: "Wallet" },
  { href: "/minting", label: "Minting" },
];

function shortAddress(value: string) {
  if (value.length < 24) {
    return value;
  }
  return `${value.slice(0, 16)}...${value.slice(-12)}`;
}

export function DashboardShell({ children }: { children: React.ReactNode }) {
  const pathname = usePathname();
  const { wallet, hasVault, storedAddress, connectWithPasskey, disconnect } =
    useWallet();
  const { pushToast } = useToast();

  return (
    <div className="flex min-h-full flex-1">
      <aside className="hidden w-64 flex-col border-r border-border bg-surface-1/80 p-5 backdrop-blur md:flex">
        <Link href="/" className="text-sm font-bold uppercase tracking-[0.22em] text-primary">
          VoidChain
        </Link>
        <p className="mt-1 text-xs text-muted-2">Dark control grid</p>

        <nav className="mt-8 flex flex-col gap-2">
          {navItems.map((item) => {
            const active = pathname.startsWith(item.href);
            return (
              <Link
                key={item.href}
                href={item.href}
                className={cn(
                  "rounded-md border px-3 py-2 text-sm transition",
                  active
                    ? "border-primary bg-primary-soft text-primary-strong"
                    : "border-transparent text-muted hover:border-border hover:bg-surface-2 hover:text-foreground",
                )}
              >
                {item.label}
              </Link>
            );
          })}
        </nav>
      </aside>

      <div className="flex min-h-full flex-1 flex-col">
        <header className="sticky top-0 z-30 border-b border-border bg-background/90 px-4 py-3 backdrop-blur md:px-8">
          <div className="mx-auto flex w-full max-w-7xl items-center justify-between">
            <div className="flex items-center gap-2">
              <Badge variant={wallet ? "success" : "default"}>
                {wallet ? "wallet connected" : "wallet idle"}
              </Badge>
              <p className="hidden text-xs text-muted md:block">
                {wallet
                  ? shortAddress(wallet.address)
                  : storedAddress
                    ? `vault: ${shortAddress(storedAddress)}`
                    : "No passkey wallet saved"}
              </p>
            </div>

            <div className="flex items-center gap-2">
              {wallet ? (
                <Button variant="secondary" onClick={disconnect}>
                  Disconnect
                </Button>
              ) : hasVault ? (
                <Button
                  onClick={async () => {
                    try {
                      await connectWithPasskey();
                      pushToast({ title: "Wallet unlocked", description: "Passkey accepted." });
                    } catch (error) {
                      pushToast({
                        title: "Passkey unlock failed",
                        description:
                          error instanceof Error ? error.message : "Unknown error",
                        variant: "danger",
                      });
                    }
                  }}
                >
                  Connect Passkey
                </Button>
              ) : (
                <Link href="/wallet">
                  <Button>Setup Wallet</Button>
                </Link>
              )}
            </div>
          </div>
        </header>

        <main className="mx-auto flex w-full max-w-7xl flex-1 flex-col px-4 py-6 md:px-8">
          {children}
        </main>
      </div>
    </div>
  );
}
