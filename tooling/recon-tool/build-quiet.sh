#!/bin/sh
# Cargo diagnostics are retained in scratch, printed only on failure.
set -eu
recon_tool_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
recon_tool_logs=${TMPDIR:-"$HOME/.ai-tmp"}/recon-tool-build
mkdir -p "$recon_tool_logs"
recon_tool_log=$(mktemp "$recon_tool_logs/cargo.XXXXXX.log")
if (cd "$recon_tool_root" && cargo build --release --target i686-unknown-linux-gnu) >"$recon_tool_log" 2>&1; then
    :
else
    recon_tool_status=$?
    cat "$recon_tool_log" >&2
    exit "$recon_tool_status"
fi
