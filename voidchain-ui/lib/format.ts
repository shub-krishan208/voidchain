export function truncateHash(hash: string, start = 10, end = 8): string {
  if (hash.length <= start + end + 3) return hash;
  return `${hash.slice(0, start)}\u2026${hash.slice(-end)}`;
}

export function truncateAddress(addr: string, start = 12, end = 8): string {
  const cleaned = addr
    .replace(/-----[A-Z\s]+-----/g, "")
    .replace(/\s+/g, "")
    .trim();
  if (cleaned.length <= start + end + 3) return cleaned;
  return `${cleaned.slice(0, start)}\u2026${cleaned.slice(-end)}`;
}

export function formatTimestamp(ts: number): string {
  return new Date(ts).toLocaleString(undefined, {
    month: "short",
    day: "numeric",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
}

export function formatAmount(amount: number): string {
  return new Intl.NumberFormat(undefined, {
    minimumFractionDigits: 0,
    maximumFractionDigits: 4,
  }).format(amount);
}
