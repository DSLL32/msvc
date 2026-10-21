#!/usr/bin/env python3
"""Match CRT code and verify the REL32 call/jump destinations, not just bytes."""
import json
import struct
import sys
import re
from collections import defaultdict
from pathlib import Path
from importlib.machinery import SourceFileLoader
from binary import PE, COFF, archive, match_section
from binary import imports

wt = SourceFileLoader('wine_tool', 'wine-tool.py').load_module()
path = sys.argv[1] if len(sys.argv) > 1 else wt.TC / 'bin/RC.EXE'
pe = PE(path)
section = next(s for s in pe.sections if s['name'] == '.text')
code = pe.data[section['off']:section['off']+section['virtual_size']]
candidates = []
objects = [(library, name, data) for library in ('LIBC.LIB', 'LIBCMT.LIB')
           for name, data in archive(wt.TC / 'lib' / library)]
objects += [('SETARGV.OBJ', 'SETARGV.OBJ', (wt.TC / 'lib/SETARGV.OBJ').read_bytes())]
for library, name, data in objects:
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
                                        object=name, library=library))
                candidates.append((fragment, start, records))

import_targets = {r['name'][7:]: r['start'] for r in imports(pe)}
configuration = json.loads(Path('functions.json').read_text())
source_names = {r['name'] for r in configuration['driver']}
if Path(path).resolve() == (wt.TC / 'bin/RC.EXE').resolve():
    source_targets = {r['name']: r['start'] for r in configuration['driver']}
else:
    source_targets = {}
    for line in Path('build/rc.map').read_text().splitlines():
        match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if match and match[1] in source_names:
            source_targets[match[1]] = int(match[2], 16)
# Identical masked code can call unrelated functions: remove/rmdir, new/delete,
# setargv/getpid and spawnv/spawnvp are examples. Resolve those relocations.
while True:
    known = defaultdict(set)
    for text, start, records in candidates:
        for r in records:
            known[r['name']].add(r['start'])
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
            if symbol['section'] == text['index']:
                expected = {start + symbol['value']}
            elif symbol['section'] == 0:
                if name.startswith('_') and '@' in name:
                    target = import_targets.get(name[1:].rsplit('@', 1)[0])
                    expected = set() if target is None else {target}
                elif name in source_targets:
                    expected = {source_targets[name]}
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

records = []
seen = set()
for text, start, group in candidates:
    for record in group:
        key = (record['name'], record['start'], record['end'])
        if key not in seen:
            records.append(record)
            seen.add(key)
for r in records:
    if r['name'] == '_strcat':
        shared = next(x for x in records if x['name'] == '_strcpy' and x['start'] == r['start']+4)
        r['shared'] = [[r['start']+8, shared['end']]]
records.sort(key=lambda r:r['start'])
Path(sys.argv[2] if len(sys.argv) > 2 else 'runtime.json').write_text(json.dumps(records, indent=2)+'\n')
print('%d runtime functions identified, %d bytes' %
      (len(records), sum(r['end']-r['start'] for r in records)))
