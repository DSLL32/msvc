#!/usr/bin/env python3
"""Affected append/storage/whole-loop controls; Linux/offline only."""
import argparse,copy,hashlib,importlib.util,json,struct,tempfile,time
from pathlib import Path
from append_state import check


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();original,native=a.original.resolve(),a.native.resolve()
 spec=importlib.util.spec_from_file_location('append_control_decoder',Path(__file__).with_name('check-callback.py'));d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
 trial=json.loads((native/'trial.json').read_text());state=json.loads((native/'state.json').read_text());text=(original/'events.txt').read_text();started=time.perf_counter();positive=check(d.s,d.d,original,native,trial,text,state,{})
 cases=['append-root','append-value','append-formal','append-caller','append-return-stack','append-return-register','append-header-count','append-header-capacity','append-items','append-native-neighbor','append-original-neighbor','append-original-slot','append-error','append-FP','append-owner','append-private-vector','sixth-include-value','fifth-defines-count','missing-last-append','last-iterator-NULL','last-iterator-cursor','last-matcher-result','last-matcher-callback','later-action-rule','later-action-store','later-numeric-result','later-numeric-formal','later-predicate','later-atol','later-suffix-action','later-CRC-seed','later-CRC-slot','resolver-output','resolver-input','resolver-capacity','resolver-formal','resolver-owner','resolver-original-slot','resolver-unknown','final-neighbor','final-INT3','whole-extent']
 results=[]
 with tempfile.TemporaryDirectory(prefix='append-controls-',dir=original.parent)as temporary:
  old,new=Path(temporary)/'original',Path(temporary)/'native';old.mkdir();new.mkdir()
  for dest,source in ((old,original),(new,native)):
   for item in source.iterdir():(dest/item.name).symlink_to(item,target_is_directory=item.is_dir())
  for case in cases:
   changed=[];config,final,events=copy.deepcopy(trial),copy.deepcopy(state),text
   def write(directory,name,data):
    q=directory/name;q.unlink();q.write_bytes(data);changed.append((q,(original if directory==old else native)/name))
   def row(name,fn,index=0):
    value=json.loads((native/(name+'.json')).read_text());fn(value[index]);write(new,name+'.json',json.dumps(value).encode())
   def flip(directory,name,at=0):
    value=bytearray(((original if directory==old else native)/name).read_bytes());value[at]^=1;write(directory,name,value)
   def arg(name,index,slot):row(name,lambda r:r['args'].__setitem__(slot,r['args'][slot]+1),index)
   if case=='append-root':arg('append-entries',2,0)
   elif case=='append-value':arg('append-entries',2,1)
   elif case=='append-formal':row('append-entries',lambda r:r['formal_inventory'][0].update(type='void *'))
   elif case=='append-caller':row('append-entries',lambda r:r.update(caller='match_option'))
   elif case=='append-return-stack':row('append-returns',lambda r:r.update(esp=r['esp']+4),3)
   elif case=='append-return-register':row('append-returns',lambda r:r.update(edi=r['edi']^1),4)
   elif case=='append-header-count':row('append-returns',lambda r:r.update(vector_header='07'+r['vector_header'][2:]),2)
   elif case=='append-header-capacity':row('append-entries',lambda r:r.update(vector_header=r['vector_header'][:4]+'29'+r['vector_header'][6:]),3)
   elif case=='append-items':row('append-returns',lambda r:r.update(items='01'+r['items'][2:]),4)
   elif case=='append-native-neighbor':row('append-returns',lambda r:r.update(committed=r['committed'][:-2]+'01'),5)
   elif case=='append-original-neighbor':flip(old,'append-5-return-committed.bin',-1)
   elif case=='append-original-slot':flip(old,'append-3-entry-registers.bin',64)
   elif case=='append-error':row('append-returns',lambda r:r.update(last_error=0),5)
   elif case=='append-FP':row('append-returns',lambda r:r['fp'].update(sw=r['fp']['sw']^1),4)
   elif case=='append-owner':row('append-entries',lambda r:r['locale'].update(pctype=r['locale']['pctype']+2),3)
   elif case=='append-private-vector':flip(new,'locale-native-append-3-entry-CRT-argv.bin',-1)
   elif case=='sixth-include-value':arg('append-entries',5,1)
   elif case=='fifth-defines-count':row('append-returns',lambda r:r.update(vector_header='04'+r['vector_header'][2:]),4)
   elif case=='missing-last-append':
    value=json.loads((native/'append-returns.json').read_text());value.pop();write(new,'append-returns.json',json.dumps(value).encode())
   elif case=='last-iterator-NULL':row('actions-outer-returns',lambda r:r.update(eax=1),20)
   elif case=='last-iterator-cursor':row('actions-outer-returns',lambda r:r['guard'].update(bss='01'+r['guard']['bss'][2:]),20)
   elif case=='last-matcher-result':row('actions-match-returns',lambda r:r.update(eax=1),20)
   elif case=='last-matcher-callback':arg('actions-matches',20,2)
   elif case=='later-action-rule':arg('actions-entries',15,0)
   elif case=='later-action-store':row('actions-returns',lambda r:r['guard'].update(data='01'+r['guard']['data'][2:]),15)
   elif case=='later-numeric-result':row('integer-returns',lambda r:r.update(eax=9),4)
   elif case=='later-numeric-formal':row('integer-entries',lambda r:r['formal_inventory'][0].update(type='int'),4)
   elif case=='later-predicate':row('integer-api-returns',lambda r:r.update(eax=0),13)
   elif case=='later-atol':row('integer-api-returns',lambda r:r.update(eax=9),14)
   elif case=='later-suffix-action':arg('suffix-entries',2,2)
   elif case=='later-CRC-seed':arg('actions-signatures',len(json.loads((native/'actions-signatures.json').read_text()))-1,2)
   elif case=='later-CRC-slot':row('actions-signature-returns',lambda r:r['argument_slots_after'].__setitem__(2,r['eax']^1),len(json.loads((native/'actions-signatures.json').read_text()))-1)
   elif case=='resolver-output':arg('append-resolvers',0,0)
   elif case=='resolver-input':arg('append-resolvers',0,1)
   elif case=='resolver-capacity':arg('append-resolvers',0,2)
   elif case=='resolver-formal':row('append-resolvers',lambda r:r['formal_inventory'][1].update(type='void *'))
   elif case=='resolver-owner':row('append-resolvers',lambda r:r['output_owner'].update(bytes=519))
   elif case=='resolver-original-slot':flip(old,'actions-resolver-entry-0-registers.bin',52)
   elif case=='resolver-unknown':flip(old,'frontend-data-locale-actions-resolver-entry-0.bin',8000)
   elif case=='final-neighbor':final['frontend']['pending_group_head']^=1
   elif case=='final-INT3':final['pc']+=128
   elif case=='whole-extent':config['ranges'][-1]['end_rva']+=1
   else:raise AssertionError(case)
   try:check(d.s,d.d,old,new,config,events,final,{})
   except(ValueError,AssertionError)as error:results.append({'case':case,'rejection':str(error)})
   else:raise ValueError('append mutation accepted: '+case)
   for q,target in changed:q.unlink();q.symlink_to(target,target_is_directory=target.is_dir())
 result=dict(kind='affected append/option-continuation controls; no execution',original=str(original),native=str(native),original_receipt_sha256=sha(original/'receipt.json'),native_receipt_sha256=sha(native/'receipt.json'),checker_sha256=sha(Path(__file__).with_name('append_state.py')),control_sha256=sha(Path(__file__)),positive=positive,count=len(results),cases=results,seconds=time.perf_counter()-started)
 if a.output.exists():raise ValueError('refuse receipt overwrite')
 a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'count':len(results),'seconds':result['seconds']}))


if __name__=='__main__':main()
