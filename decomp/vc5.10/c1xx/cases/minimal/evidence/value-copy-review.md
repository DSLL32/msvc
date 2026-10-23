# Callback 18 concrete review packet — independently approved

Source: `src/initializers.c`, `src/frontend.h`, `src/frontend.c`.
Storage/writer evidence: `value-copy-plan.md`. Trial records and immutable raw
receipt hashes: `../token-copy-trials.json`. Review inspected the 34-boundary prefix and the sole callback18 candidate. Candidate RVA `3891e..3893b` owns all 29 original bytes.

The callback sets sentinel token kind to 1 and copies the known union prefix.
There are no accepted-code reads selecting or interpreting a union arm: target
code only assigns the union. Integer and opaque object-pointer alternatives are
writer-proven in this executable. The source token discriminator is not consumed
by this callback. Sentinel kind 1 identifies the sentinel; it does not establish
that arbitrary pointer payloads can be compared as integers.

The natural input is the observed zero-initialized prefix. Auxiliary inputs
explicitly select the integer arm: `0x1122334455667788` and
`-0x112233445566778`, with distinct old cache `-7` and old kind 165. Pointer-arm
behavior is NOT tested or claimed. A real pointer-valued input must acquire an
original/native allocation or object identity mapping before validation; high
DWORD defaults are never inferred from low-DWORD pointer stores.

Source `104ba8c8..104ba8f0` and cache payload `104c0b88..104c0bb0` are separate
DLL-lifetime BSS objects; kind is at `104c0b80`. Field-map checks verify exact
known addresses, widths, aliases, and full-payload non-overlap with all other
modeled storage. Original header's seven bytes after kind and both 32-byte
payload tails remain unmodeled. Raw complete original source, payload and header
are retained at defaults, repeat-reset phase, actual callback entry and frontier.
The original's full 40-byte destination equals its ORIGINAL entry source; the
source and untouched header bytes remain unchanged. This is not a native-tail
comparison or a claim of 40-byte native equivalence.

Native `TokenValue` is an 8-byte union; `SentinelToken` is 12 bytes (kind at 0,
value at 4, three bytes of natural padding). The projection covers 17 semantic
bytes, with no placeholder original gap or tail. Native padding is observed and
must remain unchanged across the callback. All adjacent modeled state, source
prefix, prior output objects and nine cleanup registrations remain unchanged.
All new fields participate in repeated BSS clearing; the two numeric auxiliary
pairs also prove nonzero reset inputs with compiler modes 3 and 0. Overall BSS
projection is 561 semantic bytes plus 27 native padding bytes, sizeof 588.

Natural and both auxiliary pairs reach callback 19's genuine named INT3, before
any unknown payload consumer. The integer arm is the only auxiliary semantic
interpretation. Any later wider read, write, escape, serialization, comparison or
unmodeled variant consumer remains a hard reconstruction frontier.

Original VC5 and native builds pass; the Rust whole-function/ABI/import/extent
checks include this exact candidate. Audit time was 0.0282 seconds; individual
build components and original/native capture times are recorded in the packet.
42 fresh controls reject low/high integer corruption, discriminator changes,
wrong input seeds, source/cache aliasing, wrong union/width/layout, placeholder
storage, truncated or changed original tails, original source/header mutations,
native own-padding mutations and false entry/frontier PCs. Their elapsed time
was not measured separately. Prior protocol controls are not claimed fresh.

Read-only Linux validation:

```
python3 scripts/check-projection.py --packet cases/minimal/token-copy-trials.json
python3 scripts/check-token-negative.py
```

The normal pair reuses its unchanged original execution after a native-only GDB
typedef-introspection fix. Both native captures remain immutable; the second
records the real underlying union type. The original target/body/mask, compiled
native bytes and case inputs were unchanged. No original recapture was performed
merely for that debugger metadata correction. The orchestrator independently validated all three pairs and 42 controls,
then released this specific callback for promotion. Its limits remain binding.
