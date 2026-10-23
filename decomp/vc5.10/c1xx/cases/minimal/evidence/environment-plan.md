# Compiler arena environment wrapper

The candidate owns38df1..38e33 and cold72150..7215f:81 code bytes. Its
first actual operation is CALL38e33 (return38df9), before any arena/descriptor
read. The first natural frontier is a genuine no-argument named INT3 stub.
The complete wrapper loop, failure-report call and later helper are preserved
in Win32 C++ solely because the genuine member calls require thiscall.
Nine20-byte descriptors atd5af8..d5bac are contextual records encountered here,
not a data image. Their exact defaults are checked against actual loaded bytes.
The loop tests flags+0c, walks descriptors20/arenas32, calls Reserve(desc,NULL),
reports60 on failure, then calls initialize_pool_arenas. Those later paths have
static graph/ABI proof, not dynamic execution coverage in this trial.

Own member38e70 stores descriptor+10, reserve_bytes+14, VirtualAlloc resultbase+8,
and zero mapped_view+1c. Reset38f0a proves cursor/committed_limit=base and
committed_bytes/reset_value=0. Further semantics of reset_value remain unknown
and future consumers stay pending. Serializer4f8ec proves descriptor access;
4f94b/4f955 copies descriptor+10 into the serialization object's alignment+24.
Opaque allocation/pointer domains are not consumed at the current frontier.

The nine exact32-byte arena records join FrontendBss, its normal BSS clear, field
map, layouts and both nonzero restore modes. Each owned pointer is compared as
snapshot/interior or the actual matching descriptor object identity; scalar
17/23/37/29 remain scalars. The external descriptor table is preserved through
reset. Batched GDB reads observe the whole72 typed arena words and45 descriptor
words at every existing checkpoint; no observation is cached or skipped.
There is no native placeholder gap or opaque bulk BSS backing array.

Schema31 has1373 semantic BSS bytes,31 native C padding bytes, sizeof1404.
The initialized projection stays149 semantic+51 padding,8371 original unknown
bytes. Full original BSS at wrapper/helper frontiers and all initialized/saved/
support state must be unchanged. Auxiliary reset inputs are explicitly labeled
and separate from the natural promotion pair. The actual helper's source formals
are void, its original return38df9 and native saved caller/directCALL are checked.
