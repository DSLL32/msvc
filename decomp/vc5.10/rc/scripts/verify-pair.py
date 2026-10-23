#!/usr/bin/env python3
"""Fail-closed verification of actual raw pair, immutable trial inputs and current artifacts."""
import argparse
import json
from pathlib import Path
from common import HERE,ROOT,read,sha,source_pins,parse_original,parse_native,symbol,CRT_SHA,verify_compiled_sources,original_trial_projection

from reference_data import verify as verify_reference

def validate(directory,candidate):
    directory=Path(directory);config=read(candidate)
    reference_pins=verify_reference(config)
    audit=read(directory/'audit.json')
    assert audit['reference_pins']==reference_pins,'Trial reference inputs changed'
    assert read(directory/'native/receipt.json')['reference_pins']==reference_pins,'Native reference inputs changed'
    paths={'tool':ROOT/'tooling/graph-audit/target/release/graph-audit','config':Path(candidate),'map':HERE/'build/rc.map','metadata':HERE/'graph-metadata.json','original':ROOT/config['target']['path'],'rebuilt':HERE/'build/rc.exe','native':HERE/'build/native/rc','native_build_receipt':directory/'native/compiled-sources.json','original_runtime':HERE/'build/audit-original-runtime.json','rebuilt_runtime':HERE/'build/runtime.json'}
    assert audit['input_bindings']=={k:sha(p) for k,p in paths.items()},'Trial audit inputs changed'
    assert read(paths['original_runtime'])==audit['runtime_projection']['original'] and read(paths['rebuilt_runtime'])==audit['runtime_projection']['rebuilt'],'Runtime projection changed'
    assert audit['config_sha256']==sha(candidate),'Audit did not cover this trial'
    assert audit['graph_metadata_sha256']==sha(HERE/'graph-metadata.json'),'Audit metadata changed'
    assert audit['native_sha256']==sha(HERE/'build/native/rc') and audit['rebuilt_sha256']==sha(HERE/'build/rc.exe'),'Audit binaries changed'
    expected=set(config['accepted']) & {r['name'] for r in config['driver']+config['support']}
    assert audit['checked_functions']==len(expected) and set(audit['functions'])==expected,'Trial source graph not fully audited'
    for side in ['original','native']:
        place=directory/side;receipt=read(place/'receipt.json')
        captured=read(place/'candidate.json')
        if side=='original' and (directory/'reuse-original.json').exists():
            reuse=read(directory/'reuse-original.json')
            assert reuse['receipt_sha256']==sha(place/'receipt.json'),'Reused original receipt changed'
            assert reuse.get('driver_symbol_projection',[])==original_trial_projection(config,captured),'Invalid naming-only original projection'
        else:
            assert (place/'candidate.json').read_bytes()==Path(candidate).read_bytes(),'Trial snapshot differs'
        assert receipt.get('candidate_sha256',receipt.get('config_sha256'))==sha(place/'candidate.json'),'Trial config hash differs'
        if side=='native':
            assert receipt['native_sha256']==sha(HERE/'build/native/rc'),'Native changed since capture'
            assert receipt['rebuilt_sha256']==sha(HERE/'build/rc.exe'),'Rebuilt changed since capture'
            assert receipt['map_sha256']==sha(HERE/'build/rc.map'),'Map changed since capture'
        for path,pin in receipt['files'].items():
            file=place/path;assert file.stat().st_size==pin['bytes'] and sha(file)==pin['sha256'],'Raw capture changed '+side+'/'+path
    original=read(directory/'original/receipt.json')
    assert original['target_sha256']==sha(ROOT/config['target']['path'])==config['target']['sha256'],'Wrong target'
    assert sha(directory/'original/loaded-target.bin')==config['target']['sha256'],'Wrong loaded target'
    assert sha(directory/'original/loaded-crt.bin')==CRT_SHA,'Wrong loaded CRT'
    assert sha(directory/'original/loaded-rcdll.bin')==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL'),'Wrong loaded RCDLL'
    import hashlib
    sources=read(directory/'original/compiled-sources.json')
    assert {name:hashlib.sha256(text.encode()).hexdigest() for name,text in sources.items() if name.startswith('src/')}==original['sources'],'Original immutable source binding differs'
    assert hashlib.sha256(sources['scripts/debug-original.c'].encode()).hexdigest()==original['debugger_source_sha256'],'Original helper snapshot differs'
    compiled=read(directory/'native/compiled-sources.json')
    assert compiled['binary_sha256']==sha(HERE/'build/native/rc'),'Native compiled provenance differs'
    verify_compiled_sources(compiled)
    assert read(directory/'native/receipt.json')['sources']==source_pins(),'Native source binding changed'
    rows=config['driver']+config['support']+config['imports']
    expected_ranges=[(r['start']-0x1000000,r['end']-0x1000000,int(not r['name'].startswith('import:'))) for r in rows if r['name'] in config['accepted']]
    expected_ranges += [(r['start']-0x1000000,r['end']-0x1000000,2) for r in config.get('text_data',[]) if r['owner'] in config['accepted']]
    actual_ranges=[tuple(int(t,16) if i<2 else int(t) for i,t in enumerate(line.split())) for line in (directory/'original/ranges.txt').read_text().splitlines()]
    assert actual_ranges==expected_ranges,'Mask ranges differ from this trial'
    snapshot=(directory/'original/relocated-text.bin').read_bytes()
    actual_mask=(directory/'original/masked-text.bin').read_bytes()
    assert len(snapshot)==0x59e and len(actual_mask)==len(snapshot),'Wrong mask snapshot extent'
    from binary import PE
    target=PE(ROOT/config['target']['path'])
    assert snapshot[0x100:0x3ca]==target.read(0x1001100,0x2ca),'Snapshot target code changed'
    expected_mask=bytearray(b'\xcc'*len(snapshot))
    for begin,end in [(0,0x58),(0x60,0x7c),(0x3ca,len(snapshot))]:expected_mask[begin:end]=snapshot[begin:end]
    for start,end,kind in expected_ranges:expected_mask[start-0x1000:end-0x1000]=snapshot[start-0x1000:end-0x1000]
    assert bytes(expected_mask)==actual_mask,'Actual mask differs from immutable trial'
    o=parse_original(directory/'original',config);n=parse_native(directory/'native')
    n['entries']=[next(r['name'] for r in config['driver'] if symbol(r['name'])==name) for name in n['entries']]
    assert o==n,('Actual raw frontier/state/trace differs',o,n)
    for stream in ['stdout.bin','stderr.bin']:
        assert (directory/'original'/stream).read_bytes()==(directory/'native'/stream).read_bytes(),'Raw stream differs '+stream
    if o['completion']:
        assert (directory/'original/stdout.bin').read_bytes()==(HERE/'cases/help/reference/stdout.bin').read_bytes(),'Completion stdout differs from master'
        assert (directory/'original/stderr.bin').read_bytes()==b'','Completion stderr differs'
    return o

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--candidate',type=Path,default=HERE/'functions.json');p.add_argument('directory',type=Path);a=p.parse_args();print(json.dumps(validate(a.directory,a.candidate),indent=2))
