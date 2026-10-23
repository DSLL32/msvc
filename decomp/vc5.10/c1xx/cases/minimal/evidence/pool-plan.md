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
