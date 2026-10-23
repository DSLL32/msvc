# Accepted parser data classification correction

Historical parser and subsequent empty-helper executions restored the correct
132+84 code bytes and16 table bytes, with padding excluded. Their graph configs
incorrectly placed the table among code_chunks. Those original receipts, configs,
audit reports and raw files remain unchanged historical evidence.

The current config removes exactly12988..12998 from parser code_chunks and adds
that same range as shared. Existing metadata already declares these16 bytes as
text_data and supplies all four exact switch targets. The unchanged candidate's
chunks/ranges describe restored-byte scope, including this non-entry data range;
audit_roles.expected_code now projects the separate actual code intervals.

The strict normalization accepts only this one accepted-parser role correction.
It rejects changes to hot/cold extents, table range, entries, membership, role,
helper identity or any other config field. The full restored-byte union and entry
plan are checked explicitly. Candidate, source, helper, rebuilt DLL/MAP, ELF,
inputs, target and recorded states are unchanged. No Wine or native execution
was added solely for this correction.

Both existing normal and restore raw pairs validate against the new strict audit:
58 source functions,2862 code bytes plus16 owned text-data bytes =2878 physical
bytes. Separate13 focused corruption controls pass; their exact list and times
are in focused-controls.json. Prior76 SYSTEM_INFO controls remain historical,
not rerun. The new checker/contract content and both old receipt identities are
bound by the controller snapshot and validation records.

From the C1 directory, reproduce the positive and focused controls with:

    python3 scripts/check-data-classification.py /root/.ai-tmp/msvc2/vc5.10/c1/minimal-system-info-001 /root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-system-info-001

The separate restore positive command is:

    python3 scripts/check-trial.py /root/.ai-tmp/msvc2/vc5.10/c1/minimal-system-info-clear-aux-001 /root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-system-info-clear-aux-001
