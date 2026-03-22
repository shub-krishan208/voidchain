import { VOIDCHAIN_DEFAULT_URL } from "@/lib/voidchain/constants";

type RequestInitWithBody = RequestInit & { body?: BodyInit | null };

export class VoidchainApiError extends Error {
  status: number;

  constructor(message: string, status: number) {
    super(message);
    this.status = status;
  }
}

export function getVoidchainBaseUrl() {
  return (
    process.env.VOIDCHAIN_API_URL ||
    process.env.NEXT_PUBLIC_VOIDCHAIN_API_URL ||
    VOIDCHAIN_DEFAULT_URL
  );
}

function normalizePath(path: string) {
  if (path.startsWith("/")) {
    return path;
  }
  return `/${path}`;
}

export async function voidchainServerRequest<T>(
  path: string,
  init: RequestInitWithBody = {},
): Promise<T> {
  const target = `${getVoidchainBaseUrl()}${normalizePath(path)}`;
  const response = await fetch(target, {
    ...init,
    headers: {
      "Content-Type": "application/json",
      ...(init.headers || {}),
    },
    cache: "no-store",
  });

  const contentType = response.headers.get("content-type") || "";
  const payloadText = await response.text();

  if (!response.ok) {
    throw new VoidchainApiError(payloadText || response.statusText, response.status);
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
