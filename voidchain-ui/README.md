# VoidChain UI

Dark-only cyberpunk web UI for VoidChain with:

- Blocks explorer (`/blocks`)
- Mempool monitor (`/mempool`)
- Mining console (`/mining`)
- Passkey wallet connect + wallet dashboard (`/wallet`)
- Asset minting/transfer flow (`/minting`)

## Runtime Architecture

- Frontend: Next.js App Router
- BFF layer: `app/api/voidchain/[...path]/route.ts`
- Upstream: VoidChain node HTTP API (see `mds/website_backend_integration_helper.md`)
- Auth model: non-custodial passkey unlock for locally encrypted wallet secret key

## Environment

Copy `.env.example` to `.env.local`:

```bash
cp .env.example .env.local
```

Important variables:

- `VOIDCHAIN_API_URL`: upstream VoidChain node URL for server-side proxying.
- fallback if omitted: `http://localhost:4040`.

## Local Development

From `voidchain-ui`:

```bash
bun install
bun run dev
```

Visit [http://localhost:3000](http://localhost:3000).

## Local Node Setup Reminder

VoidChain backend in this repository currently defaults to port `4040` via root `/.env` (`PORT=4040`), even though source fallback in `src/main.cpp` is `18169` if `PORT` is missing.

To keep frontend + backend aligned during local runs, use:

- `VOIDCHAIN_API_URL=http://localhost:4040`

## Security Notes

- This project never persists raw wallet secret keys on the Next.js server.
- `secretKey` is stored client-side only, encrypted in local storage and unlocked by passkey verification.
- Production deployment should enforce HTTPS and avoid logging request bodies.
