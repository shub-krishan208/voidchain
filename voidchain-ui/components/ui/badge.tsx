import * as React from "react";
import { cn } from "@/lib/cn";

type BadgeVariant = "default" | "success" | "danger";

const badgeVariant: Record<BadgeVariant, string> = {
  default: "border-primary/50 bg-primary-soft text-primary-strong",
  success: "border-success/50 bg-success/15 text-success",
  danger: "border-danger/50 bg-danger/15 text-danger",
};

export function Badge({
  variant = "default",
  className,
  ...props
}: React.HTMLAttributes<HTMLSpanElement> & { variant?: BadgeVariant }) {
  return (
    <span
      className={cn(
        "inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-semibold uppercase tracking-wide",
        badgeVariant[variant],
        className,
      )}
      {...props}
    />
  );
}
