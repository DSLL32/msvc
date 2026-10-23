# Pool arena selection

The25-byte `CompilerPool::SetArena` member12b30..12b49 selects the indexed
arena and calls the genuine `Refill(1)` member. The source retains ECX/stack
thiscall and RET4. All known callers ignore its result; no meaningful EAX result
is claimed. The pending refill body remains a named INT3 stub.

Fresh normal and restore pairs independently observe actual SetArena entry and
the next actual Refill trap. Pool8 receives index0; only its arena pointer changes
from NULL to the owned arena0 object. Complete original BSS before/after, all
modeled state, initialized/saved state, neighboring pools, real formal inventories,
caller arguments, receiver and stack bytes match. The full six-arena prefix and
separate actual host CRT/Invoke namespaces remain checked.

Strict graph/ABI/extent checks pass65 functions:3336 code bytes and16 switch-data
bytes. This includes63 application functions/2873 code bytes. Both paired
positives and22 fresh targeted mutation controls pass; unchanged prefix campaigns
are explicitly reused. Native recorded build took1.900s; original-toolchain build
passed but was not separately timed. Audit took0.033s, original/native captures
3.323/6.730s normal and2.570/7.900s restore, targeted controls5.279s. Native
lifecycle fixture passes in3.85ms excluding build. Ordinary proof files and
external retained-artifact identities are in `evidence/pool-binding-*`.

Minimal compilation continues at Refillaa9c. Initialized state remains64 known
and6484 UNMODELED bytes; no reservation contents or unmodeled state are consumed.
