Bounded C function inlining
============================

User-selected queue: /O2 /Ob1 /G5 /W3, then /O2 /Ob2 /G5 /W3, both with
/nologo /c inline.c. The same source has two __inline functions, an ordinary
small helper, parameters and early returns. Actual original CL child requests
and complete boundary inventories are frozen in evidence. Independent original
C1 runs reproduce each case twice with raw exact outputs and no normalization.
Both cases are complete after the embedded CodeView case. Existing frontend
inline-candidate handling produces each exact raw output, with no new function.
The remaining inline-query stubs require the separate Quick Compiler path;
ordinary /Ob1 and /Ob2 do not reach them. validation.json preserves the fresh
masked-original/native matches and empty new-entry histories at852.
Native fixture execution never invokes Wine or recaptures an oracle.
