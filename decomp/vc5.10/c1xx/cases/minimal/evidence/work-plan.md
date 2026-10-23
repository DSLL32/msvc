# Byte work-buffer initializer

The source owns67 bytes38c86..38cc9. It calls the separate cdecl reserve helper
38cc9 twice with reserve0x500000/commit0x4000 and computes each limit asbase+3ffb.
Four original BSS pointersba850/b9020/ba854/b9028 become two contextual
CompilerWorkBuffer{BYTE *data,*limit} objects. Original gaps stay unmodeled.
Own consumer3b4b6 copies both bases into cursors;3b4e7 onward writes characters
through the second base. No stronger lexer domain or memory layout is inferred.

The first reserve helper remains a genuine INT3 stub, with real unsigned formals
reserve_bytes andcommit_bytes. All three fresh pairs stop there. Independent
preprologue work-entry observations anchor main's earlier phase proof; actual
work-entry to reserve then proves no data/BSS store occurred before allocation.
Both real caller returns and original/native reserve arguments are checked.
All full original initialized and BSS bytes, native modeled fields, LastError,
x87 state and Invoke mode remain unchanged across this call boundary.

All16 new pointer bytes have loaded-zero evidence and join nonzero BSS-reset
proofs. Explicit auxiliary identities are distinct interiors100/200/300/400 of
the owned snapshot allocation; they are cleared before use and never interpreted.
Native BSS is1116 bytes:1085 semantic and31 natural padding. Initialized137
semantic bytes and8383 unknown original bytes are unchanged. No API/allocation
response is mocked; VirtualAlloc has not yet been executed under reconstruction.

`work-trials.json` records three fresh pairs,21 targeted controls and provenance
of reused main controls. Native GDB captures remain3.64–3.92seconds; these are
observation costs, separate from the completed lifecycle fixture's2ms execution.
