export type CurrencyTxn = {
  type: "CURRENCY";
  id: string;
  from: string;
  to: string;
  amount: number;
  signature: string;
};

export type AssetTxn = {
  type: "ASSET";
  id: string;
  from: string;
  to: string;
  itemId: string;
  meta: string;
  signature: string;
};

export type VoidchainTransaction = CurrencyTxn | AssetTxn;

export type BlockHeader = {
  timestamp: number;
  last_hash: string;
  hash: string;
  merkle_root: string;
  nonce: number;
  difficulty: number;
  height: number;
};

export type Block = {
  timestamp: number;
  last_hash: string;
  hash: string;
  merkle_root: string;
  nonce: number;
  difficulty: number;
  transactions: VoidchainTransaction[];
};

export type TransactionSummary = {
  txId: string;
  type: "CURRENCY" | "ASSET";
  from: string;
  to?: string;
  amount?: number;
  itemId?: string;
  meta?: string;
  blockHeight: number;
  timestamp: number;
};

export type WalletInfo = {
  address: string;
  balance: number;
  assets: string[];
  recentTransactions: TransactionSummary[];
};

export type WalletMaterials = {
  address: string;
  secretKey: string;
};

export type WalletRecoverResponse = {
  address: string;
};

export type MineResponse = {
  message: string;
  new_block: Block;
};

export type TransactResponse = {
  message: string;
  transaction: VoidchainTransaction;
};

export type HealthResponse = {
  status: string;
};

export type ProofResponse = {
  txId: string;
  txHash: string;
  txData: VoidchainTransaction;
  root: string;
  block: BlockHeader;
  proof: Array<{ hash: string; isLeft: boolean }>;
};

export type BlocksResponse = {
  blocks: Block[];
};

export type HeadersResponse = {
  headers: BlockHeader[];
};

export type PoolResponse = {
  transactions: VoidchainTransaction[];
};

export type BalanceResponse = {
  address: string;
  balance: number;
};

export type OwnerResponse = {
  itemId: string;
  owner: string;
};

export type AssetsResponse = {
  address: string;
  assets: string[];
};

export type TransactionsResponse = {
  address: string;
  transactions: TransactionSummary[];
};
