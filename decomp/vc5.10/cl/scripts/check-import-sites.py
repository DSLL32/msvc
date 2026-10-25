#!/usr/bin/env python3
"""Finite CL assertions over the common auditor's qualified-import report."""
import json
import sys

report = json.load(open(sys.argv[1]))['functions']
gpa = {'dll': 'kernel32.dll', 'name': 'GetProcAddress'}
write = {'dll': 'msvcrt.dll', 'name': '_write'}
for name, original_sites, rebuilt_offsets, identity, opaque in (
        ('_execute_pass', (0x405655, 0x405660), (460, 478), gpa, (0x4056c9, 649)),
        ('_write_link_message', (0x405288, 0x40529c), (63, 91), write, None)):
    record = report[name]['register_imports']
    original, rebuilt = record['original'], record['rebuilt']
    base = rebuilt['roots'][0]  # Common CFG report includes the actual entry root.
    expected_original = {str(site): identity for site in original_sites}
    expected_rebuilt = {str(base + offset): identity for offset in rebuilt_offsets}
    if (original['resolved_register_calls'] != expected_original or
            rebuilt['resolved_register_calls'] or
            {site: value for site, value in rebuilt['direct_import_calls'].items()
             if value == identity} != expected_rebuilt or
            original['unresolved_register_calls'] or rebuilt['unresolved_register_calls'] or
            original['other_unresolved_calls'] != ([opaque[0]] if opaque else []) or
            rebuilt['other_unresolved_calls'] != ([base + opaque[1]] if opaque else []) or
            record['require_all_register_calls_resolved'] != (opaque is None)):
        raise SystemExit('CL qualified import site inventory differs: ' + name)
