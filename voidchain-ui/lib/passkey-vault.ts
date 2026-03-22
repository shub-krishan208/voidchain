"use client";

const VAULT_STORAGE_KEY = "voidchain.passkey.vault.v1";
const DERIVE_NAMESPACE = "voidchain-passkey-vault-v1";

export type WalletVault = {
  address: string;
  credentialId: string;
  encryptedSecretKey: string;
  iv: string;
  label: string;
  createdAt: number;
};

type UnlockedWallet = {
  address: string;
  secretKey: string;
};

function assertWebAuthnSupport() {
  if (!window.PublicKeyCredential || !navigator.credentials) {
    throw new Error("Passkeys are not supported on this device/browser.");
  }
}

function randomBytes(length: number) {
  const out = new Uint8Array(length);
  crypto.getRandomValues(out);
  return out;
}

function bytesToBase64Url(bytes: Uint8Array) {
  const str = btoa(String.fromCharCode(...bytes));
  return str.replaceAll("+", "-").replaceAll("/", "_").replaceAll("=", "");
}

function base64UrlToBytes(value: string) {
  const padded = value.padEnd(value.length + ((4 - (value.length % 4)) % 4), "=");
  const base64 = padded.replaceAll("-", "+").replaceAll("_", "/");
  const binary = atob(base64);
  const out = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i += 1) {
    out[i] = binary.charCodeAt(i);
  }
  return out;
}

async function deriveAesKey(credentialId: string) {
  const raw = new TextEncoder().encode(`${DERIVE_NAMESPACE}:${credentialId}`);
  const digest = await crypto.subtle.digest("SHA-256", raw);
  return crypto.subtle.importKey("raw", digest, "AES-GCM", false, [
    "encrypt",
    "decrypt",
  ]);
}

async function encryptSecret(secretKey: string, credentialId: string) {
  const key = await deriveAesKey(credentialId);
  const iv = randomBytes(12);
  const data = new TextEncoder().encode(secretKey);
  const cipherBuffer = await crypto.subtle.encrypt({ name: "AES-GCM", iv }, key, data);

  return {
    encryptedSecretKey: bytesToBase64Url(new Uint8Array(cipherBuffer)),
    iv: bytesToBase64Url(iv),
  };
}

async function decryptSecret(
  encryptedSecretKey: string,
  iv: string,
  credentialId: string,
) {
  const key = await deriveAesKey(credentialId);
  const cipherBytes = base64UrlToBytes(encryptedSecretKey);
  const ivBytes = base64UrlToBytes(iv);
  const plainBuffer = await crypto.subtle.decrypt(
    { name: "AES-GCM", iv: ivBytes },
    key,
    cipherBytes,
  );
  return new TextDecoder().decode(plainBuffer);
}

async function createPasskeyCredential(label: string) {
  assertWebAuthnSupport();

  const credential = (await navigator.credentials.create({
    publicKey: {
      challenge: randomBytes(32),
      rp: { name: "VoidChain UI" },
      user: {
        id: randomBytes(32),
        name: `voidchain-${Date.now()}`,
        displayName: label,
      },
      pubKeyCredParams: [
        { type: "public-key", alg: -7 },
        { type: "public-key", alg: -257 },
      ],
      timeout: 60000,
      attestation: "none",
      authenticatorSelection: {
        userVerification: "preferred",
        residentKey: "preferred",
      },
    },
  })) as PublicKeyCredential | null;

  if (!credential) {
    throw new Error("Passkey setup failed.");
  }

  return bytesToBase64Url(new Uint8Array(credential.rawId));
}

async function assertWithPasskey(credentialId: string) {
  assertWebAuthnSupport();
  const assertion = (await navigator.credentials.get({
    publicKey: {
      challenge: randomBytes(32),
      allowCredentials: [
        { id: base64UrlToBytes(credentialId), type: "public-key" },
      ],
      timeout: 60000,
      userVerification: "preferred",
    },
  })) as PublicKeyCredential | null;

  if (!assertion) {
    throw new Error("Passkey verification failed.");
  }
}

function saveVault(vault: WalletVault) {
  localStorage.setItem(VAULT_STORAGE_KEY, JSON.stringify(vault));
}

export function readVault(): WalletVault | null {
  if (typeof window === "undefined") {
    return null;
  }
  const raw = localStorage.getItem(VAULT_STORAGE_KEY);
  if (!raw) {
    return null;
  }
  try {
    return JSON.parse(raw) as WalletVault;
  } catch {
    return null;
  }
}

export function clearVault() {
  if (typeof window !== "undefined") {
    localStorage.removeItem(VAULT_STORAGE_KEY);
  }
}

export async function provisionWalletVault(
  address: string,
  secretKey: string,
  label = "VoidChain Wallet",
) {
  const credentialId = await createPasskeyCredential(label);
  await assertWithPasskey(credentialId);
  const encrypted = await encryptSecret(secretKey, credentialId);

  const vault: WalletVault = {
    address,
    credentialId,
    encryptedSecretKey: encrypted.encryptedSecretKey,
    iv: encrypted.iv,
    label,
    createdAt: Date.now(),
  };
  saveVault(vault);
  return vault;
}

export async function unlockWalletVault(): Promise<UnlockedWallet> {
  const vault = readVault();
  if (!vault) {
    throw new Error("No local passkey wallet found.");
  }

  await assertWithPasskey(vault.credentialId);
  const secretKey = await decryptSecret(
    vault.encryptedSecretKey,
    vault.iv,
    vault.credentialId,
  );
  return { address: vault.address, secretKey };
}
