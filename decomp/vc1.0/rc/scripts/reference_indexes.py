"""Pure canonical PE/map indexes for explicit ABI/graph analysis. No execution."""
import hashlib
import importlib.machinery
import json
from pathlib import Path
import re
from binary import PE, imports
wt=importlib.machinery.SourceFileLoader('rc_reference_paths',str(Path(__file__).with_name('wine-tool.py'))).load_module()

def publics():
    result = {}
    for line in Path('build/rc.map').read_text().splitlines():
        match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if match:
            # Publics precede static symbols in this linker's map. Preserve a
            # real public definition if a library has a same-named private copy.
            result.setdefault(match[1], int(match[2], 16))
    return result


def indexes():
    original = PE(wt.TC / 'bin/RC.EXE')
    reconstruction = PE('build/rc.exe')
    cfg = json.loads(Path('functions.json').read_text())
    assert hashlib.sha256(original.data).hexdigest() == cfg['target']['sha256']
    source_names = {r['name'] for r in cfg['driver']}
    oi = [r for r in json.loads(Path('runtime.json').read_text())
          if r['name'] not in source_names] + imports(original) + cfg['driver']
    names = publics()
    ri = [r for r in json.loads(Path('build/runtime.json').read_text())
          if r['name'] not in source_names] + imports(reconstruction)
    # A byte-identical RET/padding fragment can masquerade as an unrelated
    # CRT symbol. The reconstruction's map must prove both name and address.
    ri = [r for r in ri if r['name'].startswith('import:') or
          (r['name'] in names and names[r['name']] == r['start'])]
    code_end = max(reconstruction.base + s['rva'] + s['virtual_size']
                   for s in reconstruction.sections if s['flags'] & 0x20)
    boundaries = sorted(set(names.values()) | {code_end})
    for record in cfg['driver']:
        if record['name'] in names:
            start = names[record['name']]
            ri.append(dict(name=record['name'], start=start,
                           end=next(a for a in boundaries if a > start)))
    return original, reconstruction, cfg, oi, ri
