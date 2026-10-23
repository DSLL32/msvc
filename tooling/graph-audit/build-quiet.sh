#!/bin/sh
# Build this host tool from any working directory. Keep successful Cargo chatter
# in a log; failures retain Cargo's status and replay both captured streams.
set -eu
if [ "$#" -ne 0 ]; then
    echo 'usage: build-quiet.sh (optional CARGO and GRAPH_AUDIT_BUILD_LOG environment variables)' >&2
    exit 2
fi
graph_audit_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
if [ -n "${GRAPH_AUDIT_BUILD_LOG:-}" ]; then
    case "$GRAPH_AUDIT_BUILD_LOG" in
        /*) graph_audit_log=$GRAPH_AUDIT_BUILD_LOG ;;
        *) graph_audit_log=$(pwd -P)/$GRAPH_AUDIT_BUILD_LOG ;;
    esac
    mkdir -p -- "$(dirname -- "$graph_audit_log")"
else
    mkdir -p -- "$graph_audit_dir/target/build-logs"
    graph_audit_log=$(mktemp "$graph_audit_dir/target/build-logs/cargo.XXXXXX.log")
fi
cd -- "$graph_audit_dir"
if "${CARGO:-cargo}" build --quiet --release --locked \
    --manifest-path "$graph_audit_dir/Cargo.toml" \
    --target-dir "$graph_audit_dir/target" >"$graph_audit_log" 2>&1; then
    exit 0
else
    graph_audit_status=$?
    echo "graph-audit Cargo build failed (status $graph_audit_status; log: $graph_audit_log)" >&2
    cat -- "$graph_audit_log" >&2 || :
    exit "$graph_audit_status"
fi
