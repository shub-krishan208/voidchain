import { cn } from "@/lib/cn";

export function DataFieldGroup({
  className,
  children,
}: {
  className?: string;
  children: React.ReactNode;
}) {
  return (
    <dl
      className={cn(
        "divide-y divide-border/50 rounded-lg border border-border/50 bg-surface-1/50 overflow-hidden",
        className,
      )}
    >
      {children}
    </dl>
  );
}

export function DataField({
  label,
  value,
  mono,
}: {
  label: string;
  value: React.ReactNode;
  mono?: boolean;
}) {
  return (
    <div className="flex items-baseline justify-between gap-4 px-4 py-2.5">
      <dt className="shrink-0 text-xs font-medium uppercase tracking-wider text-muted-2">
        {label}
      </dt>
      <dd
        className={cn(
          "text-right text-sm text-foreground break-all",
          mono && "font-mono text-xs",
        )}
      >
        {value}
      </dd>
    </div>
  );
}
