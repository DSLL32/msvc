# Pool member ABI and encountered fields

Callback24 `389c8..389d5` pushes1, sets ECX to `104c0ac8`, calls
`389d5`, then returns. The member `389d5..389eb` returns this in EAX,
receives one unsigned stack argument, and returns with RET4. The genuine
caller and member use a minimal C++ TU and shared RECON_THISCALL; no extra
ABI wrapper or generated constructor is introduced. Both compiler targets
build without RTTI, C++ exceptions or libstdc++.

The four-field POD has cursor(+0), remaining(+4), minimum_pages(+8) and
opaque arena pointer(+12). `10401035..1040105b` allocates20 bytes by advancing
the cursor and subtracting remaining bytes. `1ee29` rounds requested bytes
to pages, takes at least this+8, passes &remaining to arena member1ee58,
and stores its returned cursor. `38f94` selects arena `c6500+index*32`,
stores it in +12, then calls1ee29. The arena pointee remains unmodeled.

Three fresh wrapper trials establish exact ECX/stack and native this/argument
identity, field defaults, nonzero expanded BSS clearing in both reset modes,
and the genuine unreconstructed member INT3 frontier. Seventeen controls
reject ABI, object/storage, mask and acceptance-credit changes. The original
member remains fully masked and receives no credit in this wrapper acceptance.

The frozen wrapper state-model evidence string omitted a separator in the
10401035 address; numeric field addresses and state checks were correct.
The current explanatory string is corrected without relabeling raw evidence.

The real 22-byte member is accepted separately after fresh original/native
entry and caller-return observations: actual ECX/stack argument, EAX=this,
and ESP_after=ESP_entry+8 (return address plus RET4) match. Natural and both
nonzero constructor-input/reset modes produce cursor=NULL, remaining=0,
minimum_pages=1, arena=NULL without changing modeled neighbors. Eighteen
return/assignment controls pass; seventeen wrapper controls were refreshed.
A published shared CRT _access fix changed only the native build, so all
three native captures were refreshed against the immutable original captures,
with exact original-side execution/config/mask/helper equality checked.
The Linux-only lifecycle regression remains 12ms warm, with no Wine calls.

Callback25 `389eb..389f8` initializes a second genuine pool at `bf5b0`.
Its four fields join typed BSS clearing and separate original/native storage
maps. Both actual member entries now have per-instance native raw ABI
observations and original ECX/stack observations; the first completed object
stays unchanged while the second is initialized. Sixteen new instance checks
and eighteen refreshed member-return controls pass. No new member credit or
pointee modeling is claimed. The next callback26 has no incoming ABI inputs:
it pushes a constant and loads its own receiver before the shared member call.
