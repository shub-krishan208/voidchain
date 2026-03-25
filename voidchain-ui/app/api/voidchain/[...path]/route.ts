import type { NextRequest } from "next/server";
import { VoidchainApiError, voidchainServerRequest } from "@/lib/voidchain/server";

export const runtime = "nodejs";

const ALLOWED_GET = new Set([
  "health",
  "blocks",
  "headers",
  "proof",
  "state",
  "balance",
  "owner",
  "assets",
  "transactions",
  "wallet/info",
  "pool",
]);

const ALLOWED_POST = new Set([
  "wallet/generate",
  "wallet/recover",
  "transact",
  "transact/signed",
  "mine",
]);

function allow(method: string, path: string) {
  if (method === "GET") {
    return ALLOWED_GET.has(path);
  }
  if (method === "POST") {
    return ALLOWED_POST.has(path);
  }
  return false;
}

async function proxyRequest(
  request: NextRequest,
  context: { params: Promise<{ path: string[] }> },
) {
  const params = await context.params;
  const path = params.path.join("/");
  if (!allow(request.method, path)) {
    return Response.json(
      { error: "Unsupported API route or method." },
      { status: 404 },
    );
  }

  const query = request.nextUrl.search;
  const upstreamPath = `${path}${query}`;

  try {
    const isBodyMethod = request.method === "POST" || request.method === "PUT";
    const bodyText = isBodyMethod ? await request.text() : undefined;
    const payload = await voidchainServerRequest<unknown>(upstreamPath, {
      method: request.method,
      body: bodyText && bodyText.length > 0 ? bodyText : undefined,
      headers: {
        "Content-Type":
          request.headers.get("content-type") || "application/json",
      },
    });
    return Response.json(payload, { status: 200 });
  } catch (error) {
    if (error instanceof VoidchainApiError) {
      return new Response(error.message, { status: error.status });
    }
    return new Response("VoidChain upstream request failed", { status: 500 });
  }
}

export async function GET(
  request: NextRequest,
  context: { params: Promise<{ path: string[] }> },
) {
  return proxyRequest(request, context);
}

export async function POST(
  request: NextRequest,
  context: { params: Promise<{ path: string[] }> },
) {
  return proxyRequest(request, context);
}

export async function OPTIONS() {
  return new Response(null, {
    status: 204,
    headers: { Allow: "GET, POST, OPTIONS" },
  });
}
