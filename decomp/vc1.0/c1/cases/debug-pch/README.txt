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
