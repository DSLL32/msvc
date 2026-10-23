This immutable case interrupts RCPP.EXE with a real `GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)` while its primary thread is blocked inside a native `ReadFile` on an empty named pipe. It was captured twice against the untouched original before changing its reconstruction. Both originals exited 1 with empty stdout/stderr and identical 46-byte `result.i`; both retained the unchanged 81-byte `interrupt.rc` and created no other files. The output is zero bytes before the event, then contains the flushed prefix after native cleanup:

```
#line 1 "interrupt.rc"

"before interrupt"
```

The exact CRLF output, source LF encoding, raw proof IDs, streams, status, complete before/after file sets and hashes are retained. `manifest.json` pins the target, source, catalog dependency, observation RVAs and captures. No output bytes or fields are normalized. The original registered handler ignores its event argument, calls `_fcloseall`, then exits 1. The capture proves the real callback received argument zero on a distinct OS-created thread; it does not invoke that callback directly.

The analysis-only controller is adapted from the corrected RC interrupt helper at checkpoint `1c09510`, with a different boundary appropriate to RCPP. It allocates a fresh console, clears inherited ignore-C before ordinary child creation, observes successful native handler registration, and verifies the console contains exactly controller and target. Group-zero Ctrl+C therefore broadcasts only within that console. The controller receives exactly one event; the debugger passes the real `DBG_CONTROL_C` as not handled before the native callback entry is observed.

The source emits a prefix and includes `\\.\pipe\vc10-rcpp-interrupt`. The controller creates this pipe, never writes to it, observes the actual native `ReadFile` handle/type/request on the primary thread, and lets the call run. With no return debug event after 200 ms, it suspends the primary, verifies its instruction pointer is in OS code and the original `ReadFile` call frame remains live, checks the client is connected with zero readable bytes, and snapshots the files. This timeout only establishes the blocked read boundary; neither signal delivery nor callback success is inferred from elapsed time.

The `ReadFile` return breakpoint remains installed throughout delivery. After queueing the real signal, the helper resumes the primary so Wine can create its callback thread, then suspends the primary at either OS thread creation or the retained return barrier, whichever arrives first. Already queued barriers do not add another suspension. Thus cancellation of the blocked read cannot let ordinary RCPP code race the callback's stream closure. The native callback disconnects the pipe and flushes/closes output; proof includes the server observing `ERROR_BROKEN_PIPE` and exclusive reopening of `result.i` after exit. The partial output is retained, not deleted.

These raw records describe the historical private-prefix capture. Active RCPP references now live under `rcpp/cases/shared-prefix/console-interrupt`, explicitly recaptured twice with the shared prefix and fixed R:/S: drives. The retained controller uses one serialized shared-prefix original capture; it never clones a prefix, remaps a drive or manages a server. Observation breakpoints are restored when execution resumes; the read-return barrier remains until process exit.

Build and independently repeat both original captures:

```
make -C decomp/vc1.0/rcpp -j2 build/interrupt-run.exe
python3 decomp/vc1.0/fixtures/rcpp-interrupt/capture.py
```

The helper is excluded from reconstructed RCPP's link. The fixture was captured on the Wine version pinned in the manifest. CTRL_BREAK is outside this assigned case; RC's separate fixture documents its Wine 10 limitation.
