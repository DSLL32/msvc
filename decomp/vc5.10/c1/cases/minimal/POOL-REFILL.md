# Pool refill page request

`CompilerPool::Refill` owns47 hot bytesaa9c..aacb and7 cold bytes2c3f7..2c3fe.
It calculates unsigned ceil(minimum_bytes/page_size), raises it to minimum_pages,
calls the real arena allocator with the pool's remaining-count address, stores
the returned cursor and returns it. Both fresh runs stop at that allocator before
any output-count/cursor effect. The cold minimum branch is reconstructed but is
not claimed exercised by this1-byte/1-page case.

The independently recorded actual Refill entry, all formal inventories, raw
ECX/stack arguments, caller arguments, owned arena0 receiver and pool8.remaining
address match. No semantic state changes before the frontier; complete original
regions and known state are checked. Strict graph/ABI/ownership checks pass66
source functions/3390 code bytes plus16 switch-data bytes. Application accounting
is64 functions/2927 code bytes; initialized64 known/6484 UNMODELED is unchanged.

Both complete paired positives and23 fresh affected controls pass; unchanged
prefix controls are explicitly reused. Original/native builds took3.781/1.972s,
audit0.032s, original/native captures3.423/7.551s normal and3.021/8.494s restore,
controls6.285s, native lifecycle fixture4.35ms excluding build. Frozen ordinary
proofs and retained external artifact identities are in `evidence/pool-refill-*`.
The native compiled-content snapshot includes the subsequently reviewed Watcom-only
startup changes; C1 neither enables nor consumes that opt-in behavior.

Minimal compilation continues at AllocatePagesaacb, a genuine RET8 thiscall
member with unsigned page count and unsigned-count output pointer.
