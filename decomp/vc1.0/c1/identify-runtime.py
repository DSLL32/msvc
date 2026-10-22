#!/usr/bin/env python3
"""Match CRT code and verify the REL32 call/jump destinations, not just bytes."""
import json
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path
from importlib.machinery import SourceFileLoader
from binary import PE, COFF, archive, match_section
from compare import imports

wt = SourceFileLoader('wine_tool', 'wine-tool.py').load_module()
path = sys.argv[1] if len(sys.argv) > 1 else wt.TC / 'bin/C13232.EXE'
pe = PE(path)
section = next(s for s in pe.sections if s['name'] == '.text')
code = pe.data[section['off']:section['off']+section['virtual_size']]
candidates = []
objects = list(archive(wt.TC / 'lib/LIBC.LIB'))
objects += [('SETARGV.OBJ', (wt.TC / 'lib/SETARGV.OBJ').read_bytes())]
for name, data in objects:
    obj = COFF(data)
    for text in obj.sections:
        if not text['flags'] & 0x20:
            continue
        funcs = sorted((s for s in obj.symbols.values()
                        if s['section'] == text['index'] and
                        (s['type'] == 0x20 or s['storage'] == 2) and
                        not s['name'].startswith('.')), key=lambda s:s['value'])
        matches = match_section(code, text)
        pieces = [(text, funcs, matches)] if matches else []
        if not matches:
            # A CRT object can contain a revised neighbor while this individual
            # function remains byte-identical. Match at public function borders.
            for i, sym in enumerate(funcs):
                begin = sym['value']
                end = funcs[i+1]['value'] if i+1 < len(funcs) else len(text['data'])
                if begin == end:
                    continue
                fragment = dict(text, data=text['data'][begin:end], relocs=[])
                for offset, target, kind in text['relocs']:
                    if begin <= offset < end:
                        adjusted = dict(target)
                        if adjusted['section'] == text['index']:
                            adjusted['value'] -= begin
                        fragment['relocs'].append((offset-begin, adjusted, kind))
                piece_symbol = dict(sym, value=0)
                pieces.append((fragment, [piece_symbol], match_section(code, fragment)))
        for fragment, symbols, matches in pieces:
            for off in matches:
                start = pe.base + section['rva'] + off
                records = []
                for i, sym in enumerate(symbols):
                    end = symbols[i+1]['value'] if i+1 < len(symbols) else len(fragment['data'])
                    records.append(dict(name=sym['name'], start=start+sym['value'], end=start+end,
                                        object=name, library='SETARGV.OBJ' if name == 'SETARGV.OBJ' else 'LIBC.LIB'))
                candidates.append((fragment, start, records))

# A library wrapper may call a source-reconstructed CRT entry in the same
# archive object. Resolve that relocation against the linked public, rather
# than requiring the obsolete library neighbor to survive as an exact match.
source_targets = {}
if len(sys.argv) > 1:
    driver_names = {r['name'] for r in json.loads(Path('functions.json').read_text())['driver']}
    for line in Path('build/c1.map').read_text().splitlines():
        match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if match and match[1] in driver_names:
            source_targets[match[1]] = int(match[2], 16)

import_targets = {r['name'][7:]: r['start'] for r in imports(pe)}
# Identical masked code can call unrelated functions: remove/rmdir, new/delete,
# setargv/getpid and spawnv/spawnvp are examples. Resolve those relocations.
while True:
    known = defaultdict(set)
    for text, start, records in candidates:
        for r in records:
            known[r['name']].add(r['start'])
    for name, address in source_targets.items():
        known[name].add(address)
    remaining = []
    for text, start, records in candidates:
        valid = True
        for offset, symbol, kind in text['relocs']:
            if kind != 20:
                continue
            actual = start + offset + 4 + struct.unpack('<i', pe.read(start+offset, 4))[0]
            addend = struct.unpack_from('<i', text['data'], offset)[0]
            name = symbol['name']
            expected = None
            if name in source_targets:
                expected = {source_targets[name]}
            elif symbol['section'] == text['index']:
                expected = {start + symbol['value']}
            elif symbol['section'] == 0:
                if name.startswith('_') and '@' in name:
                    target = import_targets.get(name[1:].rsplit('@', 1)[0])
                    expected = set() if target is None else {target}
                elif name in known:
                    expected = known[name]
            if expected is not None and actual - addend not in expected:
                valid = False
                break
        if valid:
            remaining.append((text, start, records))
    if len(remaining) == len(candidates):
        break
    candidates = remaining

records = [r for text, start, group in candidates for r in group]
for r in records:
    if r['name'] == '_strcat':
        shared = next(x for x in records if x['name'] == '_strcpy' and x['start'] == r['start']+4)
        r['shared'] = [[r['start']+8, shared['end']]]
records.sort(key=lambda r:r['start'])
Path(sys.argv[2] if len(sys.argv) > 2 else 'runtime.json').write_text(json.dumps(records, indent=2)+'\n')
print('%d runtime functions identified, %d bytes' %
      (len(records), sum(r['end']-r['start'] for r in records)))
