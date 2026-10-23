Debug-bearing PCH workflow
==========================

One user-selected create/reuse workflow. Original CL /Z7 /Od /G5 /W3 uses
/Ycdebug.h then /Yudebug.h with /Fpdebug.pch. Header contains real enum,
recursive/nested records, union, packed layout, function pointer and inline
function. Source/header mtimes are pinned input data; PCH/object mtimes for
consumer are captured exactly. Canonical S:\vc1.0\c1\debug-pch paths are
preserved without normalization. Two complete original CL runs produce
identical40167-byte PCH and exact raw frontend intermediates.

Normal native fixtures never invoke Wine. Consumer must use the real native
producer PCH after exact comparison with the original bytes. Consumer also
checks the PCH remains unchanged. Original backend object inputs are raw
captured dependencies only; no backend reconstruction is assigned. /Yd is
omitted because it selects a different debug-PCH mode.

Both phases complete at875 accepted entries,748 source functions. The producer
required eight new source functions plus three identified CRT support entries
and SetEndOfFile; the consumer added restore_pch_debug_state. Native PCH bytes
match the original exactly and the actual producer output is passed onward.
Every enum comparator operand pair (six calls) was observed and matched on both
sides. LF_PRECOMP restoration and all24 native phases pass. Negative checks
reject a changed producer PCH before launching the consumer and reject any
changed PCH bytes left by the consumer. See validation.json and evidence/.
