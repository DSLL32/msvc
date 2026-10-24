#!/usr/bin/env python3
"""Focused suffix/continuation/pending-input controls without execution."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import time

from suffix_state import check


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    spec=importlib.util.spec_from_file_location('suffix_control_decoder',Path(__file__).with_name('check-callback.py'));d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
    original,native=a.original.resolve(),a.native.resolve();trial=json.loads((native/'trial.json').read_text());state=json.loads((native/'state.json').read_text());text=(original/'events.txt').read_text();started=time.perf_counter()
    positive=check(d.s,d.d,original,native,trial,text,state,{})
    cases=['entry-argument','entry-action','entry-formal','entry-caller','return-value','return-stack','return-register','return-third-slot','original-pattern','original-argument','entry-owner','return-FP','return-guard','Z-action-rule','Z-action-value','Z-action-store','Z-original-unknown','CRC-seed','CRC-result','CRC-third-slot','iterator-owner','iterator-stack','matcher-callback','append-root','append-value','append-formal','append-caller','append-header','append-items','append-native-commit','append-original-commit','append-original-ABI','append-private-vector','append-error','final-neighbor','final-Z-store','final-INT3','original-trap-stack','cold-extent']
    results=[]
    with tempfile.TemporaryDirectory(prefix='suffix-controls-',dir=original.parent) as temporary:
        old,new=Path(temporary)/'original',Path(temporary)/'native';old.mkdir();new.mkdir()
        for dest,source in ((old,original),(new,native)):
            for item in source.iterdir():(dest/item.name).symlink_to(item,target_is_directory=item.is_dir())
        for case in cases:
            changed=[];config,final,events=copy.deepcopy(trial),copy.deepcopy(state),text
            def write(directory,name,data):
                q=directory/name;q.unlink();q.write_bytes(data);changed.append((q,(original if directory==old else native)/name))
            def row(name,fn,index=0):
                v=json.loads((native/(name+'.json')).read_text());fn(v[index]);write(new,name+'.json',json.dumps(v).encode())
            def flip(directory,name,offset=0):
                data=bytearray(((original if directory==old else native)/name).read_bytes());data[offset]^=1;write(directory,name,data)
            def arg(name,index,slot):row(name,lambda r:r['args'].__setitem__(slot,r['args'][slot]+1),index)
            if case=='entry-argument':arg('suffix-entries',0,1)
            elif case=='entry-action':arg('suffix-entries',0,2)
            elif case=='entry-formal':row('suffix-entries',lambda r:r['formal_inventory'][2].update(type='void *'))
            elif case=='entry-caller':row('suffix-entries',lambda r:r.update(caller='apply_option_action'))
            elif case=='return-value':row('suffix-returns',lambda r:r.update(eax=0))
            elif case=='return-stack':row('suffix-returns',lambda r:r.update(esp=r['esp']+4))
            elif case=='return-register':row('suffix-returns',lambda r:r.update(edi=r['edi']^1))
            elif case=='return-third-slot':row('suffix-returns',lambda r:r['argument_slots_after'].__setitem__(2,36))
            elif case=='original-pattern':flip(old,'suffix-0-pattern.bin')
            elif case=='original-argument':flip(old,'suffix-0-argument.bin')
            elif case=='entry-owner':row('suffix-entries',lambda r:r['locale'].update(pctype=r['locale']['pctype']+2))
            elif case=='return-FP':row('suffix-returns',lambda r:r['fp'].update(sw=r['fp']['sw']^1))
            elif case=='return-guard':row('suffix-returns',lambda r:r['guard'].update(bss='01'+r['guard']['bss'][2:]))
            elif case=='Z-action-rule':arg('actions-entries',3,0)
            elif case=='Z-action-value':arg('actions-entries',3,1)
            elif case=='Z-action-store':row('actions-returns',lambda r:r['guard'].update(data='01'+r['guard']['data'][2:]),3)
            elif case=='Z-original-unknown':flip(old,'frontend-data-locale-actions-action-return-3.bin',8000)
            elif case.startswith('CRC-'):
                index=4 if trial.get('test_seed') else 5
                if case=='CRC-seed':arg('actions-signatures',index,2)
                elif case=='CRC-result':row('actions-signature-returns',lambda r:r.update(eax=r['eax']^1),index)
                else:row('actions-signature-returns',lambda r:r['argument_slots_after'].__setitem__(2,r['eax']^1),index)
            elif case=='iterator-owner':row('actions-outer-returns',lambda r:r.update(eax=r['eax']+1),4)
            elif case=='iterator-stack':row('actions-outer-returns',lambda r:r.update(esp=r['esp']+4),4)
            elif case=='matcher-callback':arg('actions-matches',4,2)
            elif case=='append-root':arg('suffix-append-entries',0,0)
            elif case=='append-value':arg('suffix-append-entries',0,1)
            elif case=='append-formal':row('suffix-append-entries',lambda r:r['formal_inventory'][0].update(type='void *'))
            elif case=='append-caller':row('suffix-append-entries',lambda r:r.update(caller='match_option'))
            elif case=='append-header':row('suffix-append-entries',lambda r:r.update(vector_header='01'+r['vector_header'][2:]))
            elif case=='append-items':row('suffix-append-entries',lambda r:r.update(items='01'+r['items'][2:]))
            elif case=='append-native-commit':row('suffix-append-entries',lambda r:r.update(committed=r['committed'][:-2]+'01'))
            elif case=='append-original-commit':flip(old,'suffix-append-committed.bin',-1)
            elif case=='append-original-ABI':flip(old,'actions-append-entry-0-registers.bin',44)
            elif case=='append-private-vector':flip(new,'locale-native-suffix-next-CRT-argv.bin',-1)
            elif case=='append-error':row('suffix-append-entries',lambda r:r.update(last_error=0))
            elif case=='final-neighbor':final['frontend']['pending_group_head']^=1
            elif case=='final-Z-store':final['frontend']['options_data']['language_extensions']=7
            elif case=='final-INT3':final['pc']+=128
            elif case=='original-trap-stack':events=events.replace('stack=104396bc,','stack=104396bd,')
            elif case=='cold-extent':config['ranges'][-1]['end_rva']+=1
            else:raise AssertionError(case)
            try:check(d.s,d.d,old,new,config,events,final,{})
            except(ValueError,AssertionError)as error:results.append(dict(case=case,rejection=str(error)))
            else:raise ValueError('suffix mutation accepted: '+case)
            for q,target in changed:q.unlink();q.symlink_to(target,target_is_directory=target.is_dir())
    result=dict(kind='new affected suffix/continuation semantic controls; no execution',original=str(original),native=str(native),original_receipt_sha256=sha(original/'receipt.json'),native_receipt_sha256=sha(native/'receipt.json'),checker_sha256=sha(Path(__file__).with_name('suffix_state.py')),control_sha256=sha(Path(__file__)),positive=positive,count=len(results),cases=results,seconds=time.perf_counter()-started)
    if a.output.exists():raise ValueError('refuse overwrite')
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'])))


if __name__=='__main__':main()
