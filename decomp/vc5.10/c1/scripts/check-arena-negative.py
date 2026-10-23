#!/usr/bin/env python3
"""Fresh arena boundary controls and exhaustive new-field projection guards.

The field guards call the actual target-state validator against the immutable
paired data. They do not rerun unchanged whole-pipeline protocol campaigns.
"""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import arena_state
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def rejects(action,name):
 try:action()
 except(AssertionError,KeyError,ValueError):return
 raise AssertionError('accepted '+name)
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();m.check(a.original,a.native)
 candidate=json.loads((a.original/'candidate.json').read_text());fields=json.loads((m.PROJECT/candidate['field_map']).read_text());state=json.loads((a.native/'state.json').read_text());events=(a.original/'events.txt').read_text()
 module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);entry,entry_sequence=arena_state.boundary(a.original,a.native,state,sequence,module);probe=json.loads((a.original/'receipt.json').read_text())['probe']
 full=['entry-missing','entry-rva','entry-pc','entry-formal','entry-return','entry-caller','entry-invocation','entry-after-frontier','frontier-formal','frontier-caller','original-call','original-return']
 for test in full:
  with tempfile.TemporaryDirectory(prefix='arena-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as tmp:
   root=Path(tmp);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n)
   if test.startswith('entry-'):
    f=n/'arena-entry-observations.json';d=json.loads(f.read_text());r=d[0]
    if test=='entry-missing':d=[]
    elif test=='entry-rva':r['boundary_rva']+=1
    elif test=='entry-pc':r['pc']+=1
    elif test=='entry-formal':r['formal_inventory']=[dict(name='invented',type='int')]
    elif test=='entry-return':r['return']+=1
    elif test=='entry-caller':r['caller']['symbol']='initialize_compiler_pass'
    elif test=='entry-invocation':r['invocation']['args']['argc']+=1
    else:r['compiler_state']['initialized']['compiler_phase']+=1
   elif test.startswith('frontier-'):
    f=n/'state.json';d=json.loads(f.read_text())
    if test=='frontier-formal':d['formal_inventory']=[dict(name='invented',type='int')]
    else:d['caller']['pc']+=1
   elif test=='original-call':f=o/'relocated-text.bin';d=bytearray(f.read_bytes());d[0x1299c-0x1000]^=1
   else:f=o/'events.txt';d=f.read_text();row=re.findall(r'^trap seq=%d .*stack=([^,]+)'%sequence,d,re.M)[0];d=d.replace('stack='+row,'stack=%08x'%(int(row,16)+1))
   f.write_bytes(d)if isinstance(d,bytearray)else f.write_text(d if isinstance(d,str)else json.dumps(d))
   receipt=f.parent/'receipt.json';v=json.loads(receipt.read_text());v['artifacts'][f.name]=dict(sha256=m.pair.sha(f),size=f.stat().st_size);receipt.write_text(json.dumps(v))
   rejects(lambda:m.check(o,n),test)
 focused=[]
 def validate(entry_override=entry,state_override=state,native=a.native):
  arena_state.validate(a.original,native,fields,entry_override,state_override,entry_sequence,sequence,module,candidate,probe)
 for i in range(6):
  for key in arena_state.MEMBERS:
   name='field-map[%d].%s'%(i,key);changed=copy.deepcopy(fields);next(r for r in changed['modeled_bss']if r['name']=='arenas[%d].%s'%(i,key))['native_offset']+=4
   rejects(lambda:arena_state.expected_fields(changed),name);focused.append(name)
   name='native-default[%d].%s'%(i,key);e=copy.deepcopy(entry);v=copy.deepcopy(state);e['compiler_state']['bss']['arenas'][i][key]=v['compiler_state']['bss']['arenas'][i][key]=1
   rejects(lambda:validate(e,v),name);focused.append(name)
 for i in range(6):
  for key in arena_state.DESCRIPTOR_MEMBERS:
   name='descriptor[%d].%s'%(i,key);e=copy.deepcopy(entry);v=copy.deepcopy(state);e['compiler_state']['arena_descriptors'][i][key]^=1;v['compiler_state']['arena_descriptors'][i][key]^=1
   rejects(lambda:validate(e,v),name);focused.append(name)
 if probe=='restore':
  with tempfile.TemporaryDirectory(prefix='arena-restore-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as tmp:
   native=Path(tmp)/'native';shutil.copytree(a.native,native);f=native/'restore-observations.json';old=f.read_bytes()
   for i in range(6):
    for key in arena_state.MEMBERS:
     name='nonzero-reset[%d].%s'%(i,key);d=json.loads(old);d['before']['bss']['arenas'][i][key]^=1;f.write_text(json.dumps(d));rejects(lambda:validate(native=native),name);focused.append(name)
 print(json.dumps(dict(status='PASS',fresh_full_controls=len(full),fresh_target_state_controls=len(focused),full_rejected=full,target_state_rejected=focused,seconds=time.perf_counter()-started,reused='Unchanged prior core/parser/SYSTEM_INFO/allocation controls are retained with frozen evidence, not rerun'),indent=2))
if __name__=='__main__':main()
