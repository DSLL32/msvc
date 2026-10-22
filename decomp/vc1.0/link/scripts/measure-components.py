#!/usr/bin/env python3
"""Analysis-only partial body accounting, independent of acceptance/unmasking."""
import argparse
import hashlib
import json
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from indexes import indexes, imports

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
original, _, config, records, _ = indexes()
inventory = Path('analysis/native-body-inventory.tsv')
accepted = set()
for record in records:
    if record['name'] in config['accepted']:
        for start, end in [(record['start'], record['end'])] + record.get('shared', []):
            accepted.update(range(start, end))

# The old Ghidra database has some entry-only seeds. Exclude every one-byte
# entry conservatively, including genuine empty functions, rather than count
# placeholders as recovered bodies. Every retained range must decode in full.
decoder = Cs(CS_ARCH_X86, CS_MODE_32)
functions, excluded = {}, []
current_names = {record['start']: record['name'] for record in config['driver']}
for line in inventory.read_text().splitlines():
    address, name, raw_ranges, calls = line.split('\t')
    entry = int(address, 16)
    ranges = [tuple(int(value, 16) for value in extent.split(':'))
              for extent in raw_ranges.strip(',').split(',') if extent]
    if len(ranges) == 1 and ranges[0][0] == ranges[0][1]:
        excluded.append(hex(entry))
        continue
    ranges = [(start, end + 1) for start, end in ranges]
    body = set()
    for start, end in ranges:
        instructions = list(decoder.disasm(original.read(start, end - start), start))
        assert sum(instruction.size for instruction in instructions) == end - start, (name, hex(start))
        body.update(range(start, end))
    functions[entry] = dict(name=current_names.get(entry, name), ranges=ranges,
                            body=body, calls=[int(value, 16) for value in calls.strip(',').split(',') if value])

text = next(section for section in original.sections if section['name'] == '.text')
all_text = set(range(original.base + text['rva'], original.base + text['rva'] + text['virtual_size']))
crt_start = min(record['start'] for record in json.loads(Path('runtime.json').read_text()))
assert crt_start == 0x509d0
import_records = imports(original)
import_starts = {record['start'] for record in import_records}
import_body = set()
for record in import_records:
    import_body.update(range(record['start'], record['end']))
native_body, crt_body = set(), set()
for entry, function in functions.items():
    if entry not in import_starts:
        (native_body if entry < crt_start else crt_body).update(function['body'])
crt_body -= import_body
native_body -= crt_body | import_body
classified = native_body | crt_body | import_body
components = []
for name, body in [('known target-body inventory', native_body),
                   ('known CRT-body inventory', crt_body),
                   ('IAT import thunk bodies', import_body),
                   ('unclassified: uninventoried bodies, inline data and padding', all_text - classified)]:
    body = body & all_text
    component = dict(name=name, total_bytes=len(body), accepted_bytes=len(body & accepted),
                     remaining_bytes=len(body - accepted))
    components.append(component)
    print('%s: %d accepted / %d known bytes' % (name, component['accepted_bytes'], component['total_bytes']))


def remaining_closure(name, roots, low, high):
    pending, visited, body, entries = list(roots), set(), set(), []
    while pending:
        entry = pending.pop()
        if entry in visited or entry not in functions or not 0x17000 <= entry < crt_start:
            continue
        visited.add(entry)
        function = functions[entry]
        pending.extend(function['calls'])
        if not low <= entry < high:
            continue
        missing = function['body'] - accepted
        if not missing:
            continue
        body.update(missing)
        entries.append(dict(entry=hex(entry), name=function['name'],
                            body_ranges=[[hex(start), hex(end)] for start, end in function['ranges']],
                            uncovered_bytes=len(missing),
                            body_sha256=hashlib.sha256(b''.join(original.read(start, end - start)
                                                               for start, end in function['ranges'])).hexdigest(),
                            callers=[hex(address) for address, other in functions.items() if entry in other['calls']]))
    print('%s: %d uncovered known bytes / %d functions' % (name, len(body), len(entries)))
    return dict(name=name, root_entries=[hex(entry) for entry in roots],
                uncovered_body_bytes=len(body), uncovered_functions=len(entries),
                functions=sorted(entries, key=lambda entry: int(entry['entry'], 16)))


mac_roots = [record['start'] for record in config['driver']
             if ('mac' in record['name'].lower() and 'machine' not in record['name'].lower())
             or 'm68k' in record['name'].lower()]
remaining = [remaining_closure('OMF conversion', [0x20b30], 0x453c0, 0x4ae00),
             remaining_closure('C++ undecoration', [0x3d740], 0x3d740, 0x427f0),
             remaining_closure('i386 dump disassembly', [0x4ae20], 0x4ae20, crt_start),
             remaining_closure('Macintosh/m68k named-root static closure', mac_roots, 0x17000, crt_start)]
report = dict(
    method='Partial Ghidra11.4.2 body inventory, checked with Capstone. Ranges omit inter-block/function gaps. '
           'All one-byte entries are excluded. Native/CRT partition uses the layout boundary at verified '
           'mainCRTStartup 0x509d0, not complete library provenance. Imports are IAT-resolving FF25 thunks. '
           'Unclassified includes accepted bodies absent from this old inventory, inline data and padding; '
           'it is not all padding. This is not branch coverage or a complete body denominator.',
    target_sha256=hashlib.sha256(original.data).hexdigest(),
    inventory_sha256=hashlib.sha256(inventory.read_bytes()).hexdigest(),
    accepted_frontiers=len(config['accepted']), total_text_bytes=len(all_text),
    accepted_extent_bytes=len(accepted & all_text), excluded_one_byte_entries=excluded,
    components=components,
    body_function_counts=dict(target=sum(entry < crt_start and entry not in import_starts for entry in functions),
                              crt_non_import=sum(entry >= crt_start and entry not in import_starts for entry in functions),
                              import_thunks=len(import_records)),
    accepted_extent_regions=[dict(region='before verified CRT 0x509d0', accepted_bytes=len({byte for byte in accepted & all_text if byte < crt_start})),
                             dict(region='CRT region including IAT thunks', accepted_bytes=len({byte for byte in accepted & all_text if byte >= crt_start}))],
    remaining_components=remaining,
    limits='Remaining closures are analysis opportunities, not promised gains. OMF closure includes internal -g '
           'debug conversion, while normal LINK wrapper requests only -o/-co. No additional cases were executed.')
if args.output:
    args.output.write_text(json.dumps(report, indent=2) + '\n')
