"use client";

import * as React from "react";
import { cn } from "@/lib/cn";

type ButtonVariant = "primary" | "secondary" | "ghost" | "danger";

type ButtonProps = React.ButtonHTMLAttributes<HTMLButtonElement> & {
  variant?: ButtonVariant;
};

const variantClasses: Record<ButtonVariant, string> = {
  primary:
    "border-primary bg-primary text-black hover:bg-primary-strong focus-visible:ring-primary/60",
  secondary:
    "border-border bg-surface-2 text-foreground hover:border-primary hover:text-primary-strong focus-visible:ring-primary/40",
  ghost:
    "border-transparent bg-transparent text-muted hover:bg-surface-2 hover:text-foreground focus-visible:ring-primary/30",
  danger:
    "border-danger bg-danger/90 text-black hover:bg-danger focus-visible:ring-danger/40",
};

export function Button({
  className,
  variant = "primary",
  type = "button",
  ...props
}: ButtonProps) {
  return (
    <button
      type={type}
      className={cn(
        "inline-flex h-10 items-center justify-center rounded-md border px-4 text-sm font-medium transition focus-visible:outline-none focus-visible:ring-2 disabled:cursor-not-allowed disabled:opacity-60",
        variantClasses[variant],
        className,
      )}
      {...props}
    />
  );
}
