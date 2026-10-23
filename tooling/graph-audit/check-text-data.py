#!/usr/bin/env python3
"""Mutate a pre-existing scratch C2 switch-data audit snapshot; never worker files.

The snapshot's config/metadata/runtime/PE/MAP and command must already be frozen
under ~/.ai-tmp (snapshot.json). This tests graph/extent ownership, not data-byte
functional equivalence, which remains the worker's separate policy.
"""
import argparse
from contextlib import contextmanager
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
a=p.parse_args();root=a.snapshot.resolve();assert root.is_relative_to(Path.home()/'.ai-tmp')
snapshot=json.loads((root/'snapshot.json').read_text());command=snapshot['command']
paths={key:Path(command[command.index('--'+key)+1])for key in ['config','metadata','rebuilt']}
assert all(path.is_relative_to(root)for path in paths.values())
config=json.loads(paths['config'].read_text());metadata=json.loads(paths['metadata'].read_text());name='@c2_match_option@16'
row=next(r for r in config['driver']if r['name']==name);data=metadata['functions'][name]['rebuilt']['text_data'];table,classmap=data
pe=paths['rebuilt'].read_bytes();u16=lambda at:struct.unpack_from('<H',pe,at)[0];u32=lambda at:struct.unpack_from('<I',pe,at)[0]
header=u32(60);optional=header+24;base=u32(optional+28);first=optional+u16(header+20)
def offset(address):
    for i in range(u16(header+6)):
        s=first+40*i;start=base+u32(s+12)
        if start<=address<start+u32(s+16):return u32(s+20)+address-start
    raise AssertionError('non-file-backed mutation address')
assert pe[offset(table[0]-3):offset(table[0])]==bytes.fromhex('c20800')
@contextmanager
def modified():
    saved={p:p.read_bytes()for p in paths.values()}
    try:yield
    finally:
        for p,b in saved.items():p.write_bytes(b)
results=[]
def run(label,expect):
    begin=time.perf_counter();p=subprocess.run(command,capture_output=True,text=True);elapsed=time.perf_counter()-begin
    assert(p.returncode==0)==expect,(label,p.stderr)
    results.append(dict(case=label,accepted=p.returncode==0,seconds=elapsed,error=p.stderr.strip()))
    return json.loads(p.stdout)if p.returncode==0 else None
positive=run('actual_C2_switch_and_classmap_strict',True)
for label in ['missing_classmap','classmap_prefix_gap','classmap_clipped_suffix','declared_live_RET','data_crosses_chunk','overlapping_declarations','external_owner_removed','non_file_backed_external','malformed_interval']:
    with modified():
        cfg=copy.deepcopy(config);meta=copy.deepcopy(metadata);function=meta['functions'][name];data=function['rebuilt']['text_data'];target=next(r for r in cfg['driver']if r['name']==name)
        if label=='missing_classmap':data.pop()
        elif label=='classmap_prefix_gap':data[1][0]+=1
        elif label=='classmap_clipped_suffix':data[1][1]-=1
        elif label=='declared_live_RET':data[0][0]-=3
        elif label=='data_crosses_chunk':data[1][1]+=1
        elif label=='overlapping_declarations':data.append(data[0].copy())
        elif label=='external_owner_removed':target['shared']=[]
        elif label=='non_file_backed_external':function['original']['text_data'][0]=[0xdead0000,0xdead0010];target['shared'][0]=[0xdead0000,0xdead0010]
        else:data[1].append(0)
        paths['config'].write_text(json.dumps(cfg));paths['metadata'].write_text(json.dumps(meta));run(label,False)
with modified():
    b=bytearray(pe);b[offset(table[0]-3):offset(table[0])]=b'\x90'*3;paths['rebuilt'].write_bytes(b);run('missing_terminal_RET_cannot_fall_into_data',False)
with modified():
    b=bytearray(pe);b[offset(classmap[1]-1)]^=1;paths['rebuilt'].write_bytes(b);run('data_content_needs_separate_byte_proof_scope_control',True)
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=hashlib.sha256(Path(command[0]).read_bytes()).hexdigest(),snapshot_source_sha256=snapshot['source_hashes'],projected_shared=snapshot['projected_shared'],positive_report=positive,results=results),indent=2)+'\n')
print('C2 explicit text-data controls: positive, 10 rejections, 1 separate-byte-proof scope control passed.')
