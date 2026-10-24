Direct diagnostic-log lifecycle (complete)

This external test owns its strings and calls the real debug initializer,
debug-log API, flag setter and debug_printf. It tests a missing parent directory,
lazy db.log creation, replacement with second.log, log-only routing, and a final
close followed by stdout-only routing. These are direct support observations;
the inner debug_printf -> open_debug_log call is a real target edge. No public
LINK command-line reachability is claimed for the external setter calls.

The one untouched-original baseline is frozen in reference/. Its normal CRT
continuation exits zero. Exact stdout/stderr are 14/44 bytes, db.log contains
"first\r\n", and second.log contains "second\r\n". The separately owned 1384-byte
test report records defined API results, flag transitions, logical stream
identity and errors. No timestamp/path/pointer/output normalization applies.

The actual debug_flags array occupies 100 bytes at VA 0x5c008. Only bytes 1 and 2
change. analysis/proof.json binds the canonical setter/getter and closed-log
def-use proof. The final close leaves the original debug_log pointer intact;
the last debug_printf bypasses both pointer loads because flag 1 is clear.
There is no subsequent open or reuse. Normal CRT cleanup checks the closed
slot's flags. Observer samples of the static FILE representation are not target
dereferences through the retained closed global.

analysis/historical-flags-observer-limitation.json records the older COMDAT
observer's adjacent-address limitation. Its original 0x5c06c sample is not this
array or proof of its mode identity. Existing raw evidence remains unchanged.
The current case reads all 100 bytes from 0x5c008 and requires explicit address
and unchanged-other-byte controls. analysis/observer-epochs.json distinguishes
the immutable baseline observer from the later actual-return-PC logging.

analysis/shipped-support.json verifies complete SETBUF.OBJ and PERROR.OBJ bodies
and all relocation targets. They are shipped CRT support, not newly handwritten
CRT functions or application credit. Native correspondence is at their actual
caller-qualified public boundaries, not their internal helper ABIs.

Earlier holds: acceptance/missing-parent-hold preserves original fopen errors
2/3/3 versus native 2/2/2. acceptance/missing-parent-candidate preserves the
separate closed FILE slot descriptor mismatch (-1 versus 0). Both raw packets
remain; reviewed shared fixes close the complete six-observation packet in
acceptance/missing-parent-fixed without another original run.

Earlier hold: acceptance/perror-hold records the actual public perror mismatch.
The original preserves errors 2/3/3 and writes 44 bytes including CRLF; the host
libc version leaves errno 22 and emits 43 bytes with LF. Both reach the next
setbuf frontier. The reviewed shared correction closes that exact packet in
acceptance/perror-fixed; it uses the genuine descriptor text-write path. No
target CRT implementation, output normalization or original repeat was needed.

acceptance/setter-frontier matches all 25 observations before the new setter.
acceptance/complete matches all 36 observations and complete artifacts against
both baseline and fresh original. All typed inputs, return ESP/nonvolatiles,
100 flag bytes, FILE owners/pending data, errors and full live FP state are
strict. The setter's incidental EAX value is not a void return contract. There
is no ambient-error or FP qualification. Eighteen offline denials include wrong
array address, other flag-byte mutation, missing calls, callers, arguments,
guards, FILE state, stack/nonvolatiles, error transitions and FP precision/C0.

The native cached runner now uses regular redirected stdout/stderr, matching
the captured original. Its initial PIPE-based attempt failed at the deliberately
unsupported perror provider contract. The initial aggregate retains its failure
and the 89 preceding passes. An explicit native PIPE diagnostic subsequently
records status 125; it is an unsupported native route, not original behavior.
The corrected 90th case passes traced and output-only checks against the same
immutable ELF and C/H hashes. The other 89 were not repeated. Exact trace:
open_debug_log, set_debug_flag; CRT/shim entries remain excluded.

Original-compiler build and 839 CFG graphs, 819 callback/source references,
514 contextual data definitions and 41 ECX ABI checks pass. Common graph/extent
denials and native cache negatives pass. Two application bodies add 125 extent
bytes: 119 instruction bytes plus 6 explicit internal alignment bytes. The two
shipped CRT support bodies add 208 extent bytes and zero application credit;
the existing 16 historical handwritten CRT overrides are unchanged.

Current accounting: 1001 accepted frontiers, 802 application functions (759
public, 43 direct), application extent 229367/281634 = 81.4415%; support-inclusive
extent 260306/281634 = 92.4271%. Partial known target-body inventory is
194239/194916. The remaining 11883 unclassified bytes are not called CRT; a full
non-CRT denominator remains unavailable.

Build took 14.724 s and cache projection 1.186 s. The initial 89-pass plus failed
90th aggregate took 10.496 s; corrected new-case trace/output processes took
0.314/0.090 s externally. Internal new-case totals are 0.230/0.0139 s, including
8.69/4.22 ms actual ELF execution. Setup, trace projection and checks are split
in timings.json. Capture, GDB, provider review and build are not fixture runtime.

Normal regression: make -j4 check. Explicit original compilation/audits, outside
fixtures: make -j4 all audit. scripts/capture-debug-log.py and the GDB observer
are explicit analysis tools only; cached fixtures never run Wine or originals.
next-proposal.json is read-only and requires root review before another case.
