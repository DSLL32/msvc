#!/usr/bin/env python3
"""Locate exact original-toolchain CRT sections, ignoring COFF fixups only."""
import json
from pathlib import Path
from binary import PE, COFF, archive, match_section
from importlib.machinery import SourceFileLoader

wt = SourceFileLoader('wine_tool', 'wine-tool.py').load_module()
import sys
pe = PE(sys.argv[1] if len(sys.argv) > 1 else wt.TC / 'bin/CL.EXE')
text = next(s for s in pe.sections if s['name'] == '.text')
code = pe.data[text['off']:text['off']+text['virtual_size']]
records = []
objects = list(archive(wt.TC / 'lib/LIBC.LIB'))
objects += [('SETARGV.OBJ', (wt.TC / 'lib/SETARGV.OBJ').read_bytes())]
for name, data in objects:
    obj = COFF(data)
    for section in obj.sections:
        if not section['flags'] & 0x20: continue
        matches = match_section(code, section)
        for off in matches:
            start = pe.base + text['rva'] + off
            funcs = sorted((s for s in obj.symbols.values()
                            if s['section'] == section['index'] and
                            (s['type'] == 0x20 or s['storage'] == 2)),
                           key=lambda s:s['value'])
            funcs = [s for s in funcs if not s['name'].startswith('.')]
            for i, sym in enumerate(funcs):
                end = funcs[i+1]['value'] if i+1 < len(funcs) else len(section['data'])
                records.append(dict(name=sym['name'], start=start+sym['value'],
                                    end=start+end, object=name,
                                    library='SETARGV.OBJ' if name == 'SETARGV.OBJ' else 'LIBC.LIB'))
from compare import imports
import struct
known = {r['name']: r['start'] for r in records if r['name'] not in ('__setargv', '__getpid', '__getdrives')}
known.update({r['name']:r['start'] for r in imports(pe)})
wrappers = {'__setargv':'___setargv', '__getpid':'import:GetCurrentProcessId', '__getdrives':'import:GetLogicalDrives'}
records = [r for r in records if r['name'] not in wrappers or
           r['start'] + 5 + struct.unpack('<i', pe.read(r['start']+1,4))[0] == known.get(wrappers[r['name']])]
for r in records:
    if r['name'] == '_strcat':
        shared = next(x for x in records if x['name'] == '_strcpy' and x['start'] == r['start']+4)
        r['shared'] = [[r['start']+8, shared['end']]]
records.sort(key=lambda r:r['start'])
Path(sys.argv[2] if len(sys.argv) > 2 else 'runtime.json').write_text(json.dumps(records, indent=2)+'\n')
print('%d runtime functions identified, %d bytes' %
      (len(records), sum(r['end']-r['start'] for r in records)))
