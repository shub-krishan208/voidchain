import Link from "next/link";

export default function Home() {
  return (
    <main className="flex flex-1 items-center justify-center px-6 py-20">
      <section className="cyber-panel cyber-grid w-full max-w-4xl rounded-2xl p-8 md:p-14">
        <p className="text-sm uppercase tracking-[0.22em] text-muted-2">
          VOIDCHAIN // CONTROL GRID
        </p>
        <h1 className="text-glow mt-5 text-4xl font-semibold tracking-tight md:text-6xl">
          Darknet explorer for blocks, mining, mempool, wallets, and minting.
        </h1>
        <p className="mt-5 max-w-2xl text-base text-muted md:text-lg">
          Run a complete cyberpunk frontend over your VoidChain node with
          passkey-based wallet connect, signed transactions, and live chain
          visibility.
        </p>
        <div className="mt-9 flex flex-wrap gap-3">
          <Link
            href="/blocks"
            className="rounded-md border border-primary bg-primary px-5 py-2.5 text-sm font-semibold text-black transition hover:bg-primary-strong"
          >
            Launch Explorer
          </Link>
          <Link
            href="/wallet"
            className="rounded-md border border-border bg-surface-2 px-5 py-2.5 text-sm font-semibold text-foreground transition hover:border-primary hover:text-primary-strong"
          >
            Connect Wallet
          </Link>
          <Link
            href="/minting"
            className="rounded-md border border-border bg-surface-2 px-5 py-2.5 text-sm font-semibold text-foreground transition hover:border-primary hover:text-primary-strong"
          >
            Mint Assets
          </Link>
        </div>
      </section>
    </main>
  );
}
