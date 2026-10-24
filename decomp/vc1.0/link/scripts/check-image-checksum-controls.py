#!/usr/bin/env python3
"""Focused offline negative checks for the frozen actual image-helper packet."""
import copy,json,time
from pathlib import Path
from image_checksum_abi import compare
c=Path('cases/direct-image-checksum/acceptance/closure');left=[json.loads(l)for l in(c/'original.jsonl').read_text().splitlines()];right=json.loads(Path('cases/direct-image-checksum/acceptance/final-native/native.json').read_text());start=time.perf_counter();compare(left,right,complete=True);checks=[]
def deny(name,edit):
    a=copy.deepcopy(left);b=copy.deepcopy(right);edit(a,b)
    try:compare(a,b,complete=True)
    except (AssertionError,KeyError,IndexError):checks.append(dict(control=name,status='rejected'));return
    raise AssertionError(('negative accepted',name))
for n,edit in [
 ('empty-observations',lambda a,b:b.update(events=[])),
 ('wrong-filename-owner',lambda a,b:b['events'][0]['formal_words'].__setitem__(0,0)),
 ('wrong-previous-owner',lambda a,b:b['events'][0]['formal_words'].__setitem__(1,b['events'][0]['output_address'])),
 ('missing-formal',lambda a,b:b['events'][0]['formal_words'].pop()),
 ('wrong-output-value',lambda a,b:b['events'][0].update(outputs='00'*16)),
 ('wrong-CW',lambda a,b:b['events'][0].update(fp_control=0x37f)),
 ('wrong-status',lambda a,b:b['events'][0].update(fp_status=0x100)),
 ('wrong-tags',lambda a,b:b['events'][0].update(fp_tag=0xfffe)),
 ('wrong-error',lambda a,b:b['events'][0]['errors'].__setitem__(0,2)),
 ('wrong-DOS',lambda a,b:b['events'][0]['errors'].__setitem__(1,2)),
 ('wrong-LastError',lambda a,b:b['events'][0]['errors'].__setitem__(2,2)),
 ('guard-write',lambda a,b:b['events'][0].update(workspace_guards='00'+b['events'][0]['workspace_guards'][2:])),
 ('wrong-stream-descriptor',lambda a,b:next(e for e in b['events']if e['name']=='recon_fileno').update(stream_descriptor=4)),
 ('wrong-public-caller',lambda a,b:next(e for e in b['events']if e['name']=='recon_filelength' and e['event']=='return').update(actual_caller='other')),
 ('wrong-original-caller',lambda a,b:next(e for e in a if e.get('target')==0x516b0 and e['event']=='return').update(eip=0)),
 ('wrong-return',lambda a,b:next(e for e in b['events']if e['name']=='checksum_image_file' and e['event']=='return').update(eax=2)),
 ('wrong-nonvolatile',lambda a,b:next(e for e in b['events']if e['event']=='return').update(ebx=0)),
 ('wrong-stack-cleanup',lambda a,b:next(e for e in b['events']if e['event']=='return').update(esp=0)),
 ('wrong-mapped-owner',lambda a,b:next(e for e in b['events']if e['mapping_live'])['workspace'].__setitem__(5,0)),
 ('wrong-section-bytes',lambda a,b:next(e for e in b['events']if e['mapping_live']).update(sections='00'*160)),
 ('missing-file-wrong-error',lambda a,b:b['events'][-1]['errors'].__setitem__(2,0)),
 ]:deny(n,edit)
print(json.dumps(dict(status='pass',checks=checks,seconds=time.perf_counter()-start,scope='Frozen packets only; no target/debugger/original/Wine. No state exclusions.'),indent=2))
