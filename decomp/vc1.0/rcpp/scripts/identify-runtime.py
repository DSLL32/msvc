#!/usr/bin/env python3
"""Inventory byte-signature candidates; this does not accept or unmask code."""
import json
from pathlib import Path
from binary import PE, COFF, archive, match_section
from compare import wt

pe = PE(wt.TC/'bin/RCPP.EXE')
text = next(s for s in pe.sections if s['name'] == '.text')
code = pe.read(pe.base+text['rva'], text['size'])
records = []
for library in ('LIBC.LIB', 'LIBCMT.LIB'):
    for member, payload in archive(wt.TC/'lib'/library):
        obj = COFF(payload)
        for section in obj.sections:
            if not section['flags'] & 0x20:
                continue
            for offset in match_section(code, section):
                start = pe.base + text['rva'] + offset
                symbols = [dict(name=s['name'], address=start+s['value'])
                           for s in obj.symbols.values()
                           if s['section'] == section['index'] and s['type'] == 0x20]
                records.append(dict(library=library, member=member, start=start,
                                    bytes=len(section['data']), symbols=symbols,
                                    status='candidate; relocations/semantics require assessment'))
result = dict(warning='Byte match candidates only; no automatic runtime equivalence or acceptance.',
              target_sha256='a66382a8fa84432a1fc380cea85fcdb3305603e5656638f1d959cdb5d25ca6dd',
              candidates=sorted(records, key=lambda r:(r['start'], r['library'], r['member'])))
destination = wt.PREFIX.parent/'runtime-candidates.json'
destination.write_text(json.dumps(result, indent=2)+'\n')
print('%d candidate sections inventoried in %s' % (len(records), destination))
