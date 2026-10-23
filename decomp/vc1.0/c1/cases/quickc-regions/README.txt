Standalone C1 QuickC regions, loops and memory initialization
============================================================
The current phase uses genuine parsed -qc/-Ol/-Oe with coherent nested loops,
tracked short parameters, inlining candidates, branches, pointer aliasing,
aggregates, static bytes and floating-point expressions. It is a standalone C1
interface case, with no asserted CL/C2 producer/backend compatibility.

Revision1(2196 bytes) has two exact silent original baselines and four real option
words observed1 at initializer entry/return. Revision2(2530 bytes) adds a used
short/unsigned-short helper because the actual parameter recorder only accepts
plain16-bit scalars in this Win32 type model. Original pointer sizes4/8 cannot
satisfy its historical two-byte pointer eligibility check. Two fresh v2 originals
precede the v2 native oracle. inputs/v1, expected/c1 and evidence/preflight/v1-case
retain the earlier source/oracles/relative manifest without relabeling raw logs.

Current untouched originals exit0 silently. Exact IR sizes are351/3344/369/73/1382
for irdb/irex/irgl/irin/irsy. v2 complete masked-original/native continuation and
fresh first-entry projection cover22 new application functions/4520 bytes.
The normal native fixture checks cached status, complete streams and all artifacts
plus application first-entry order, seeding the preceding accepted prefix.

Failures retained honestly:
- The first native region-exit proof reached a later compact-operand call and
  failed pending-expression-buffer comparison. Compact descriptors had wrongly
  used expression arena2. Actual original6e20c/6e214/6e218 identifies pool3; native
  state showed a recycled zero descriptor. The corrected continuation matches.
- Three ordinary build errors(missing header, declaration scope and token enum spelling)
  were fixed before successful checks. No original oracle was changed.
- One early native V1 debug invocation preceded its original capture completion;
  its workspace failure remains, and the completed original was reused.

The original builtin helper2f8e0 default loads uninitialized local words at2f900
and2f904. Operations172/173 initialize the name/type and are supported. Root
approved preserving the Win32 bug and precisely exiting125 for other native
operations. native-guard.c wraps main externally; the small guard runner also
runs this genuine case as its valid control. Neither adds application graph
vertices or coverage credit.

Raw original/native evidence is stored as ordinary owned files, with SHA256/size
bindings in evidence/files.json. Executables, toolchain copies and auditor/build
bundles are excluded. Referenced scratch executable hashes are historical facts;
regressions do not execute those paths. See validation.json for checks/timings.
