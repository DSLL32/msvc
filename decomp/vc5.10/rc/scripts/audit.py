#!/usr/bin/env python3
"""Rust graph audit plus RC-specific metadata, source and mask ownership checks."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import subprocess
from common import HERE,ROOT,read,sha,names
from binary import PE
from reference_data import verify as verify_reference

def audit(config,report=None):
    settings=read(config)
    reference_pins=verify_reference(settings)
    assert sha(ROOT/settings['target']['path'])==settings['target']['sha256'],'Target changed'
    rows=sorted(names(settings),key=lambda r:r['start'])
    assert len({r['name'] for r in rows})==len(rows),'Duplicate logical name'
    for row in rows:
        assert 0x1001100<=row['start']<row['end']<=0x10013ca,'Owner outside target code'
    for a,b in zip(rows,rows[1:]):assert a['end']<=b['start'],'Overlapping function ownership'
    known={r['name'] for r in rows}
    data=sorted(settings.get('text_data',[]),key=lambda r:r['start'])
    for r in data:
        assert r['owner'] in known and 0x1001000<=r['start']<r['end']<=0x1001100,'Invalid contextual data owner/range'
    for a,b in zip(data,data[1:]):assert a['end']<=b['start'],'Overlapping contextual data'
    assert set(settings['accepted'])<=known,'Unknown accepted function'
    source='\n'.join(p.read_text() for p in (HERE/'src').glob('*.c'))
    for row in settings['driver']+settings['support']:
        plain=row.get('symbol',row['name']).lstrip('_').split('@')[0]
        assert re.search(r'\b'+re.escape(plain)+r'\s*\(',source),'Missing named body/stub '+plain
        if row['name'] in settings['accepted']:
            body=re.search(r'\b'+re.escape(plain)+r'\s*\([^)]*\)\s*\{(.*?)\n\}',source,re.S)
            assert body and 'int 3' not in body[1],'Accepted body still traps'
    for p in list((HERE/'src').glob('*.c'))+list((HERE/'src').glob('*.h')):
        inc=re.findall(r'^#include\s+(.+)$',p.read_text(),re.M)
        assert inc and inc[-1]=='<recon.h>','recon.h must be last include: '+str(p)
    publics={m[1]:int(m[2],16) for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', (HERE/'build/rc.map').read_text(),re.M)}
    pe=PE(HERE/'build/rc.exe');section=next(s for s in pe.sections if s['flags']&0x20)
    ends=sorted(set(publics.values())|{pe.base+section['rva']+section['virtual_size']})
    runtime=[]
    for row in settings['support']:
        assert row['name'] in publics,'Missing support symbol'
        start=publics[row['name']]
        runtime.append({'name':row['name'],'start':start,'end':next(a for a in ends if a>start),'evidence':{'kind':'manually reconstructed support'}})
    original_runtime=[dict(r,evidence={'kind':'manually reconstructed support'}) for r in settings['support']]
    original_path=HERE/'build/audit-original-runtime.json';rebuilt_path=HERE/'build/runtime.json'
    original_path.write_text(json.dumps(original_runtime,indent=2)+'\n');rebuilt_path.write_text(json.dumps(runtime,indent=2)+'\n')
    cmd=[str(ROOT/'tooling/graph-audit/target/release/graph-audit'),'--profile','cl','--repo',str(ROOT),'--project',str(HERE),'--config',str(Path(config).resolve()),'--runtime',str(original_path),'--rebuilt-runtime',str(rebuilt_path),'--rebuilt',str(HERE/'build/rc.exe'),'--map',str(HERE/'build/rc.map'),'--metadata',str(HERE/'graph-metadata.json'),'--discovery','cfg','--check-ret','--check-imports','--check-references','--check-ownership','--check-extents','--include-evidenced-runtime']
    inputs={'tool':Path(cmd[0]),'config':Path(config),'map':HERE/'build/rc.map','metadata':HERE/'graph-metadata.json','original':ROOT/settings['target']['path'],'rebuilt':HERE/'build/rc.exe','native':HERE/'build/native/rc','native_build_receipt':HERE/'build/native/rc.inputs.json','original_runtime':original_path,'rebuilt_runtime':rebuilt_path}
    before={k:sha(p) for k,p in inputs.items()}
    result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert result.returncode==0,result.stderr.decode()
    assert before=={k:sha(p) for k,p in inputs.items()},'Audit inputs changed during tool execution'
    output=json.loads(result.stdout)
    spec=importlib.util.spec_from_file_location('rc_iat',HERE/'scripts/iat-references.py');iat=importlib.util.module_from_spec(spec);spec.loader.exec_module(iat)
    original_pe=PE(ROOT/settings['target']['path']);rebuilt_records={r['name']:r for r in runtime}
    for row in settings['driver']:
        start=publics[row.get('symbol',row['name'])];rebuilt_records[row['name']]={'start':start,'end':next(a for a in ends if a>start)}
    iat_report={}
    cached_report={}
    for row in settings['driver']+settings['support']:
        if row['name'] not in settings['accepted']:continue
        other=rebuilt_records[row['name']]
        a=iat.identities(original_pe,row['start'],row['end']);b=iat.identities(pe,other['start'],other['end'])
        assert a==b,('IAT identity references differ',row['name'],a,b)
        iat_report[row['name']]=a
        if row['name'] in ['_rc_main','_rc_diagnostic@12']:
            cached_report[row['name']]={'original':iat.cached_import(original_pe,row['start'],row['end'],row['name']),'rebuilt':iat.cached_import(pe,other['start'],other['end'],row['name'])}
    output['temporary_iat_reference_residual']=iat_report
    output['cached_import_residual']=cached_report
    output['input_bindings']=before
    output['reference_pins']=reference_pins
    output['runtime_projection']={'original':original_runtime,'rebuilt':runtime}
    output['graph_metadata_sha256']=sha(HERE/'graph-metadata.json');output['config_sha256']=sha(config);output['original_sha256']=sha(ROOT/settings['target']['path']);output['rebuilt_sha256']=sha(HERE/'build/rc.exe');output['native_sha256']=sha(HERE/'build/native/rc')
    if report:Path(report).write_text(json.dumps(output,indent=2)+'\n')
    return output

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--config',type=Path,default=HERE/'functions.json');p.add_argument('--report',type=Path);a=p.parse_args();print(json.dumps(audit(a.config,a.report),indent=2))
