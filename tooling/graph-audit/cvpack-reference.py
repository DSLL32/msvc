#!/usr/bin/env python3
"""Extracted CVPACK graph/ownership/RET reference, NOT its full audit.

--prepare explicitly runs the existing strict archive identity verifier once.
The ordinary benchmark path uses that frozen, hash-bound index on both sides.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--project',required=True,type=Path)
parser.add_argument('--cache',required=True,type=Path)
parser.add_argument('--prepare',action='store_true')
args=parser.parse_args()
project=args.project.resolve()
sys.path.insert(0,str(project/'scripts'))
from binary import PE
import audit
from graph import names
from runtime import identify
sha=lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
config,original=audit.load()
root=project.parents[2]
rebuilt_path=project/'build/CVPACK.EXE'
linked=audit.publics()
provenance=dict(original_sha256=sha(root/config['target']['path']),rebuilt_sha256=sha(rebuilt_path),
                archive_sha256=sha(root/'toolchains/1.0-nt/lib/LIBC.LIB'),functions_sha256=sha(project/'functions.json'),
                map_sha256=sha(project/'build/cvpack.map'),runtime_anchors_sha256=sha(project/'runtime-anchors.json'))
if args.prepare:
    original_runtime=identify(root/config['target']['path'],{'_main':config['target']['application_main']})
    assert original_runtime==json.loads((project/'runtime.json').read_text())
    rebuilt_runtime=identify(rebuilt_path,{'_main':linked['_main']})
    args.cache.write_text(json.dumps(dict(schema=1,provenance=provenance,original_runtime=original_runtime,rebuilt_runtime=rebuilt_runtime),indent=2)+'\n')
    print('Explicit frozen strict runtime identity cache captured')
    raise SystemExit(0)
cache=json.loads(args.cache.read_text())
assert cache['schema']==1 and cache['provenance']==provenance
runtime=cache['original_runtime']
assert runtime==json.loads((project/'runtime.json').read_text())
rebuilt=PE(rebuilt_path)
assert original.base+original.entry==config['target']['entry']
functions=config['functions']
assert len({r['name'] for r in functions})==len(functions)
assert len({r['original'] for r in functions})==len(functions)
audit.validate_ownership(config,original,runtime,json.loads((project/'data.json').read_text()))
oi=audit.identity_table(runtime,functions)
ri=audit.identity_table(cache['rebuilt_runtime'],functions,linked)
text_end=max(rebuilt.base+s['rva']+s['virtual_size'] for s in rebuilt.sections if s['flags']&0x20)
addresses=sorted(set(linked.values())|{text_end})
report={}
for record in functions:
    assert record['status'] in ('stub','candidate','accepted')
    assert record['kind']=='application'
    if record['status']=='stub':continue
    start,end=record['extent']
    assert start==record['original']
    switches={int(a):targets for a,targets in record.get('switches',{}).items()}
    for table in record.get('switch_tables',[]):
        actual=[audit.u32(original.read(table['start']+i*4,4),0) for i in range(len(table['targets']))]
        assert actual==table['targets'],'switch targets differ'
    a=audit.audited_graph(original,start,end,record.get('roots',()),record.get('text_data',()),switches)
    rs=linked[record['symbol']]
    re=next(address for address in addresses if address>rs)
    roots=audit.seh_roots(rebuilt,rs,re,record.get('compiler_seh_scaffolding',False))
    roots += [rs+offset for offset in record.get('rebuilt_extra_roots',[])]
    switches,data=audit.compiler_switches(rebuilt,rs,re,record.get('compiler_switch_counts',[]))
    b=audit.audited_graph(rebuilt,rs,re,roots,data,switches)
    left,right=names(a['edges'],oi),names(b['edges'],ri)
    assert left==right,(record['name'],left,right)
    assert a['returns']==b['returns']=={record['stack_cleanup']}
    assert record['abi']
    report[record['name']]=dict(edges=sorted(left),returns=sorted(a['returns']))
print(json.dumps(dict(profile='cvpack',checked_functions=len(report),functions=report),sort_keys=True))
