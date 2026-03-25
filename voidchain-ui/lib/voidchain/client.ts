"use client";

import type {
  AssetsResponse,
  BalanceResponse,
  BlocksResponse,
  HeadersResponse,
  HealthResponse,
  MineResponse,
  OwnerResponse,
  PoolResponse,
  ProofResponse,
  TransactResponse,
  TransactionsResponse,
  WalletInfo,
  WalletMaterials,
  WalletRecoverResponse,
} from "@/lib/voidchain/types";

type SignedCurrencyPayload = {
  secretKey: string;
  type: "CURRENCY";
  to: string;
  amount: number;
};

type SignedAssetPayload = {
  secretKey: string;
  type: "ASSET";
  to: string;
  itemId: string;
  meta: string;
};

type SignedTxnPayload = SignedCurrencyPayload | SignedAssetPayload;
type UnsignedSignedTxnPayload =
  | Omit<SignedCurrencyPayload, "secretKey">
  | Omit<SignedAssetPayload, "secretKey">;

async function parseResponse<T>(response: Response): Promise<T> {
  const contentType = response.headers.get("content-type") || "";
  const payloadText = await response.text();

  if (!response.ok) {
    throw new Error(payloadText || response.statusText);
  }

  if (!payloadText) {
    return {} as T;
  }

  if (contentType.includes("application/json")) {
    return JSON.parse(payloadText) as T;
  }

  try {
    return JSON.parse(payloadText) as T;
  } catch {
    return payloadText as T;
  }
}

async function apiRequest<T>(
  path: string,
  init: RequestInit = {},
): Promise<T> {
  const response = await fetch(`/api/voidchain/${path}`, {
    ...init,
    headers: {
      "Content-Type": "application/json",
      ...(init.headers || {}),
    },
  });
  return parseResponse<T>(response);
}

export const voidchainClient = {
  health() {
    return apiRequest<HealthResponse>("health");
  },
  blocks() {
    return apiRequest<BlocksResponse>("blocks");
  },
  headers() {
    return apiRequest<HeadersResponse>("headers");
  },
  proof(txId: string) {
    return apiRequest<ProofResponse>(`proof?txId=${encodeURIComponent(txId)}`);
  },
  pool() {
    return apiRequest<PoolResponse>("pool");
  },
  walletGenerate() {
    return apiRequest<WalletMaterials>("wallet/generate", { method: "POST" });
  },
  walletRecover(secretKey: string) {
    return apiRequest<WalletRecoverResponse>("wallet/recover", {
      method: "POST",
      body: JSON.stringify({ secretKey }),
    });
  },
  walletInfo(address: string) {
    return apiRequest<WalletInfo>(
      `wallet/info?address=${encodeURIComponent(address)}`,
    );
  },
  transactions(address: string) {
    return apiRequest<TransactionsResponse>(
      `transactions?address=${encodeURIComponent(address)}`,
    );
  },
  balance(address: string) {
    return apiRequest<BalanceResponse>(
      `balance?address=${encodeURIComponent(address)}`,
    );
  },
  assets(address: string) {
    return apiRequest<AssetsResponse>(`assets?address=${encodeURIComponent(address)}`);
  },
  owner(itemId: string) {
    return apiRequest<OwnerResponse>(`owner?itemId=${encodeURIComponent(itemId)}`);
  },
  transactSigned(payload: SignedTxnPayload) {
    return apiRequest<TransactResponse>("transact/signed", {
      method: "POST",
      body: JSON.stringify(payload),
    });
  },
  mine(minerAddress?: string) {
    const body = minerAddress ? { minerAddress } : {};
    return apiRequest<MineResponse>("mine", {
      method: "POST",
      body: JSON.stringify(body),
    });
  },
};

export type { SignedTxnPayload, UnsignedSignedTxnPayload };
