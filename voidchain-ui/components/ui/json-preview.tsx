import { cn } from "@/lib/cn";

export function JsonPreview({
  data,
  className,
}: {
  data: unknown;
  className?: string;
}) {
  return (
    <pre
      className={cn(
        "max-h-96 overflow-auto rounded-md border border-border bg-surface-1 p-3 font-mono text-xs text-muted",
        className,
      )}
    >
      {JSON.stringify(data, null, 2)}
    </pre>
  );
}
