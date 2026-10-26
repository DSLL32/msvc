WLINK 10.0a's initial bare `?` help case is complete: **116 application
functions / 8,962 unique original bytes**. The case uses NUL input and an
explicitly empty target PATH. It exits 0 with exactly 8,475 stdout bytes, empty
stderr and no output artifacts. Reconstruction is paused for review; no second
case has been started.

Build and replay from this directory:

```
make -j4 native cases build/WLINK.EXE
make -j4 fixtures unit audit
```

Fixtures use the shared native i386 DSO runner and cached `capture.bin`; they
never launch Wine. Explicit original builds use the shared launcher/prefix,
Watcom register ABI (`-3r`) and disabled added stack checks (`-s`). The native
GCC ABI is permitted at the shared compatibility boundary. Actual argv, raw
command line and the exact masked self-image/resource overlay are captured
separately; native replay does not invent or requote them.

Only the hash-pinned GS-compatible `toolchains/watcom10.0a/.patched/WLINK.EXE`
is captured; the pristine binary remains unchanged. Its patch manifest records
the four sites and nine changed bytes. This case proves DS-backed 128-byte
reader buffers owned by WriteHelp and PressKey, without granting arbitrary
far-pointer callers. CRT restoration is finite and retains explicit older
library variants. The reached nonlocal jump resumes value 1 in its live Spawn
context; SEH unwinding and expired contexts remain unsupported.

The complete graph audit checks all 116 application functions, 115 returning
ABIs and the separate genuine main-to-exit endpoint. Shared RET bytes count
once: SetBreak is explicitly seeded rather than claimed as an observed new
frontier, and _LnkFree shares ChkLAlloc's final RET. The unexecuted debug cleanup
stub remains masked; its known one-byte RET supports static ABI analysis only.

Final build/regression/audit batch: **1.265 s**. Native fixture/unit invocations:
**14.441 / 19.170 ms**. The final fresh comparison took **1,414.433 ms original /
19.600 ms native**, with native setup/execution/checks of **10.159 / 4.836 /
4.605 ms**. Logs and intermediate diagnostics live under
`~/.ai-tmp/msvc2/watcom-shared-integration/`, outside the source/case tree.
