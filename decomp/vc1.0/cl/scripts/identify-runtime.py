#!/usr/bin/env python3
"""Match CRT code and verify the REL32 call/jump destinations, not just bytes."""
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path
from importlib.machinery import SourceFileLoader
from binary import PE, COFF, archive, match_section
analysis = SourceFileLoader('cl_binary_analysis',str(Path(__file__).resolve().with_name('binary-analysis.py'))).load_module()
imports,publics=analysis.imports,analysis.publics

wt = SourceFileLoader('wine_tool', str(Path(__file__).resolve().with_name('wine-tool.py'))).load_module()
path = sys.argv[1] if len(sys.argv) > 1 else wt.TC / 'bin/CL.EXE'
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
        for off in match_section(code, text):
            start = pe.base + section['rva'] + off
            funcs = sorted((s for s in obj.symbols.values()
                            if s['section'] == text['index'] and
                            (s['type'] == 0x20 or s['storage'] == 2) and
                            not s['name'].startswith('.')), key=lambda s:s['value'])
            records = []
            for i, sym in enumerate(funcs):
                end = funcs[i+1]['value'] if i+1 < len(funcs) else len(text['data'])
                records.append(dict(name=sym['name'], start=start+sym['value'], end=start+end,
                                    object=name, library='SETARGV.OBJ' if name == 'SETARGV.OBJ' else 'LIBC.LIB'))
            candidates.append((text, start, records))

import_targets = {r['name'][7:]: r['start'] for r in imports(pe)}
config = json.loads(Path('functions.json').read_text())
is_original = str(path).endswith('bin/CL.EXE')
source_targets = ({r['name']: r['start'] for r in config['driver']} if is_original else
                  {name: address for name, address in publics().items() if name in {r['name'] for r in config['driver']}})
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

records = [r for text, start, group in candidates for r in group]
if str(path).endswith('bin/CL.EXE'):
    # These leaf bodies are identical, but distinct original graph vertices.
    records = [r for r in records if not
               (r['start'] == 0x1b738 and r['name'] == '_memcpy') and not
               (r['start'] == 0x1c0ec and r['name'] == '_memmove')]
for r in records:
    if r['name'] == '_strcat':
        shared = next(x for x in records if x['name'] == '_strcpy' and x['start'] == r['start']+4)
        r['shared'] = [[r['start']+8, shared['end']]]
records.sort(key=lambda r:r['start'])
Path(sys.argv[2] if len(sys.argv) > 2 else 'runtime.json').write_text(json.dumps(records, indent=2)+'\n')
print('%d runtime functions identified, %d bytes' %
      (len(records), sum(r['end']-r['start'] for r in records)))
