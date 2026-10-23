#!/usr/bin/env python3
"""Derive byte mutations from real indexed functions; helper for tests only."""
import importlib.util
import json
from pathlib import Path
import sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
p=Path.cwd()
profile=sys.argv[1]
sys.path.insert(0,str(p/'scripts'))
mods={'cl':'binary-analysis.py','c1':'images.py','c1xx':'win32_indexes.py','c2':'pe_indexes.py','link':'indexes.py','rc':'reference_indexes.py','rcpp':'reference.py'}
if profile=='cvpack':
    import audit
    cfg,original=audit.load()
    from binary import PE
    rebuilt=PE(p/'build/CVPACK.EXE')
    publics=audit.publics()
    addresses=sorted(set(publics.values())|{rebuilt.base+s['rva']+s['virtual_size'] for s in rebuilt.sections if s['flags']&0x20})
    drivers=[dict(name=f['name'],start=f['extent'][0],end=f['extent'][1]) for f in cfg['functions'] if f['status']!='stub']
    ri=[dict(name=f['name'],start=publics[f['symbol']],end=next(a for a in addresses if a>publics[f['symbol']])) for f in cfg['functions']]
else:
    spec=importlib.util.spec_from_file_location('indexes_under_test',p/'scripts'/mods[profile]);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    result=m.indexes()
    if profile=='rcpp':cfg,original,rebuilt,oi,ri=result
    else:original,rebuilt,cfg,oi,ri=result
    drivers=[f for f in cfg['functions' if profile=='rcpp' else 'driver'] if f['name'] in cfg['accepted']]
cs=Cs(CS_ARCH_X86,CS_MODE_32)
call=None
truncated=None
for f in drivers:
    r=next(r for r in ri if r['name']==f['name'])
    instructions=list(cs.disasm(rebuilt.read(r['start'],r['end']-r['start']),r['start']))
    direct=[i for i in instructions if i.mnemonic=='call' and i.bytes[0]==0xe8 and len(i.bytes)==5]
    if call is None and direct:
        i=direct[0]
        call=dict(name=f['name'],address=i.address,offset=rebuilt.offset(i.address),all_calls=[dict(address=i.address,offset=rebuilt.offset(i.address)) for i in direct])
    if truncated is None:
        for i in cs.disasm(original.read(f['start'],f['end']-f['start']),f['start']):
            if i.mnemonic=='ret' and len(i.bytes)==3 and i.address+3==f['end']:
                truncated=dict(name=f['name'],end=i.address+1)
                break
print(json.dumps(dict(call=call,truncated=truncated,first=drivers[0])))
