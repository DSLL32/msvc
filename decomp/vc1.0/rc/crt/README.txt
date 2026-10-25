Historical RC CRT bodies
========================

These files preserve previously accepted source graphs and Win32 ABIs; migration
moves them without reconstructing or accepting more CRT code. Native application
runs use shared lib/recon CRT support, except the two retained locale converters
and historical wcscpy body. None of these records receives App credit.

heap-crt.c is a demonstrated binary-specific variant: RC uses
GetProcessHeap/HeapAlloc/HeapFree, whereas shipped LIBC/LIBCMT use their region
and VirtualAlloc allocator. character-case.c preserves the demonstrated extra
active-codepage test and two-byte return ordering documented in that source.
The startup, arguments, process, lowio/open/stdio, formatting, scanning, flushing,
termination and CRT-state files collectively preserve the already qualified old
CRT call graph, twenty 32-byte FILE objects, locale globals and ordered callback
tables. Replacing individual bodies with a shipped-library routine requires the
existing byte/relocation and graph/ABI proof, not its familiar name alone.

The generic wide-copy, atoi/atol and path-helper bodies are retained as historical
canonical graph implementations. No unique RC-specific variant is asserted for
them. Their independent necessity as source overrides remains a qualification
question; this layout migration does not substitute or reconstruct them. The
two provenance-uncertain direct helpers remain in src/unused-helpers.c under
their existing App classification, without inventing library provenance.
