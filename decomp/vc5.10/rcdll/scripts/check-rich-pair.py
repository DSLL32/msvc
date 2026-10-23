#!/usr/bin/env python3
"""Validate a rich-case raw checkpoint and reject meaningful altered evidence."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from protocol import HERE,sha
from rich_protocol import release_entry,release_trial,prepare_trial,write_trial
p=argparse.ArgumentParser();p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path);p.add_argument('--output',type=Path);p.add_argument('--stage',choices=['entry','trial','prepare','write'],default='trial')
a=p.parse_args();validate={'entry':release_entry,'trial':release_trial,'prepare':prepare_trial,'write':write_trial}[a.stage];proof=validate(a.original,a.native);controls=[]
with tempfile.TemporaryDirectory(prefix='rich-gate-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')as scratch:
    old=Path(scratch)/'original';new=Path(scratch)/'native'
    shutil.copytree(a.original,old);shutil.copytree(a.native,new)
    validate(old,new) # The unchanged copies MUST pass before rejection controls.
    def refresh(directory,name):
        p=directory/name;r=json.loads((directory/'receipt.json').read_text());r['files'][name]=dict(bytes=p.stat().st_size,sha256=sha(p));(directory/'receipt.json').write_text(json.dumps(r))
    def reject(name,side,filename,change):
        directory=old if side=='original'else new;path=directory/filename
        original=path.read_bytes();receipt=(directory/'receipt.json').read_bytes()
        change(path);refresh(directory,filename)
        try:validate(old,new)
        except (AssertionError,KeyError):controls.append(name)
        else:raise AssertionError('bad evidence passed: '+name)
        path.write_bytes(original);(directory/'receipt.json').write_bytes(receipt)
    def snapshot(change):
        def apply(path):
            value=json.loads(path.read_text());change(value);path.write_text(json.dumps(value))
        return apply
    reject('wrong-callee','native','snapshot.json',snapshot(lambda v:v.update(function='Handler')))
    reject('missing-native-int3','native','snapshot.json',snapshot(lambda v:v.update(executed_byte='90')))
    reject('changed-current-argv','native','snapshot.json',snapshot(lambda v:v['export_vector'][2].update(text='wrong')))
    reject('changed-original-storage','native','snapshot.json',snapshot(lambda v:v['export_vector'][3].update(bytes='00')))
    reject('changed-argv-alias','native','snapshot.json',snapshot(lambda v:v['export_vector'][3].update(pointer=v['export_vector'][3]['storage'])))
    reject('wrong-working-directory','native','snapshot.json',snapshot(lambda v:v['case_inputs'].update(host_cwd='/')))
    reject('wrong-INCLUDE','native','snapshot.json',snapshot(lambda v:v['case_inputs'].update(include='wrong')))
    reject('target-host-environment-leak','native','snapshot.json',snapshot(lambda v:v['case_inputs']['environment'].append('RECON_ACP=1252')))
    if a.stage=='entry':
        reject('wrong-live-buffer','native','snapshot.json',snapshot(lambda v:v['arguments'].update(new_payload='00'*528)))
    elif a.stage=='trial':
        reject('wrong-preprocessor-argv','native','snapshot.json',snapshot(lambda v:v['rich']['preprocessor_argv'].pop()))
        reject('wrong-input-path','native','snapshot.json',snapshot(lambda v:v['arguments'].update(path='wrong.rc')))
        reject('wrong-temporary','native','snapshot.json',snapshot(lambda v:v['rich']['temporaries'].update(rc_input_temporary='wrong')))
        reject('wrong-free-flags','native','snapshot.json',snapshot(lambda v:next(x for x in v['heap']if x['operation']=='free').update(flags=0)))
        reject('free-did-not-unlink','native','snapshot.json',snapshot(lambda v:next(x for x in v['heap']if x['operation']=='free')['live_after'].append(dict(payload=0,bytes=528,flags=9))))
    if a.stage=='prepare':
        reject('wrong-write-size','native','snapshot.json',snapshot(lambda v:v['arguments'].update(bytes=44)))
        reject('wrong-write-bytes','native','snapshot.json',snapshot(lambda v:v['arguments'].update(payload='00'*46)))
        reject('wrong-wide-filename','native','snapshot.json',snapshot(lambda v:v['arguments'].update(filename='00'*514)))
        reject('wrong-file-position','native','snapshot.json',snapshot(lambda v:v['arguments'].update(file_fdinfo='pos: 1\n')))
        reject('wrong-parent-frame','native','snapshot.json',snapshot(lambda v:v['arguments']['parent_frame'].update(caller='main')))
        reject('wrong-array-bounds','native','snapshot.json',snapshot(lambda v:v['arguments']['parent_frame']['objects'][0].update(address=0)))
        reject('wrong-array-layout','native','snapshot.json',snapshot(lambda v:v['arguments']['parent_frame']['objects'][1].update(bytes=1)))
        reject('wrong-array-write-pointer','native','snapshot.json',snapshot(lambda v:v['arguments'].update(data=0)))
        reject('wrong-compiler-input-size','original','events.txt',lambda p:p.write_text(p.read_text().replace('compiler-probe-entry eax=000042c4','compiler-probe-entry eax=000042c0')))
        reject('wrong-compiler-stack-return','original','events.txt',lambda p:p.write_text(p.read_text().replace('compiler-probe-return eax=71c28576','compiler-probe-return eax=00000000')))
    if a.stage=='write':
        reject('wrong-buffered-write','native','snapshot.json',snapshot(lambda v:v['arguments'].update(output_buffer='00'*46)))
        reject('wrong-write-return','native','snapshot.json',snapshot(lambda v:v['arguments'].update(write_returns=[45])))
        reject('wrong-input-file-state','native','snapshot.json',snapshot(lambda v:v['arguments'].update(input_state='00'*32)))
        reject('wrong-output-file-state','native','snapshot.json',snapshot(lambda v:v['arguments'].update(output_state='00'*32)))
        reject('wrong-original-write-result','original','events.txt',lambda p:p.write_text(p.read_text().replace('write-result 46','write-result 45')))
    reject('wrong-allocation-owner','native','snapshot.json',snapshot(lambda v:v['heap_identity']['blocks'][0].update(owner=0)))
    reject('wrong-registered-module','native','snapshot.json',snapshot(lambda v:v['module'].update(sha256='0'*64)))
    reject('wrong-codepage','native','snapshot.json',snapshot(lambda v:v['globals'].update(rc_codepage='00000000')))
    reject('wrong-loaded-target','original','loaded-target.bin',lambda p:p.write_bytes(b'wrong target'))
    reject('wrong-loaded-CRT','original','loaded-crt.bin',lambda p:p.write_bytes(b'wrong CRT'))
    reject('stale-trial','native','functions.json',snapshot(lambda v:v.update(trial_function='Handler')))
    reject('wrong-loaded-driver','original','loaded-driver.bin',lambda p:p.write_bytes(b'wrong driver'))
    reject('extra-original-unmask','original','masked-text.bin',lambda p:p.write_bytes(p.read_bytes()[:0x1000]+b'\x90'+p.read_bytes()[0x1001:]))
    reject('missing-original-frontier','original','events.txt',lambda p:p.write_text('\n'.join(l for l in p.read_text().splitlines()if not l.startswith('frontier '))+'\n'))
    validate(old,new)
proof['negative_controls']=controls
if a.output:a.output.write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps(dict(matched_frontier=proof['matched_frontier'],application_trace=proof['application_trace'],negative_controls=controls),indent=2))
