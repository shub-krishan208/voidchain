import { cn } from "@/lib/cn";

export function EmptyState({
  message,
  className,
}: {
  message: string;
  className?: string;
}) {
  return (
    <div
      className={cn(
        "flex items-center justify-center rounded-lg border border-dashed border-border/60 py-10 text-sm text-muted-2",
        className,
      )}
    >
      {message}
    </div>
  );
}
