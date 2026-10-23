#!/usr/bin/env python3
"""Fresh primary-arena entry and genuine thiscall boundary rejection tests."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import arena_state
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def reject(action,name):
 try:action()
 except(AssertionError,KeyError,ValueError):return
 raise AssertionError('accepted '+name)
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();m.check(a.original,a.native)
 state=json.loads((a.native/'state.json').read_text());candidate=json.loads((a.original/'candidate.json').read_text());events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);entry,at=arena_state.primary_boundary(a.original,a.native,state,sequence,module)
 tests=['primary-missing','primary-pc','primary-rva','primary-formals','primary-return','primary-caller','primary-invocation','member-count','member-pc','member-ecx','member-esp','member-return','member-descriptor','member-address','member-formal-name','member-formal-type','member-state','final-this','final-descriptor','final-address','final-formal-omission','final-formal-type','final-caller','final-state','original-ecx','original-descriptor','original-address','original-return','original-call','original-ret8']
 with tempfile.TemporaryDirectory(prefix='primary-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as tmp:
  root=Path(tmp);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n)
  files=['primary-entry-observations.json','arena-member-entry-observations.json'];native_old={name:(n/name).read_bytes()for name in files};original_old={name:(o/name).read_bytes()for name in ['events.txt','relocated-text.bin']}
  for test in tests:
   for name,data in native_old.items():(n/name).write_bytes(data)
   for name,data in original_old.items():(o/name).write_bytes(data)
   value=copy.deepcopy(state)
   if test.startswith('primary-'):
    f=n/'primary-entry-observations.json';d=json.loads(f.read_text());r=d[0]
    if test=='primary-missing':d=[]
    elif test=='primary-pc':r['pc']+=1
    elif test=='primary-rva':r['boundary_rva']+=1
    elif test=='primary-formals':r['formal_inventory']=[dict(name='invented',type='int')]
    elif test=='primary-return':r['return']+=1
    elif test=='primary-caller':r['caller']['symbol']='c1_main'
    else:r['invocation']['args']['mode']^=1
    f.write_text(json.dumps(d));reject(lambda:arena_state.primary_boundary(o,n,value,sequence,module),test);continue
   if test.startswith('member-'):
    f=n/'arena-member-entry-observations.json';d=json.loads(f.read_text());r=d[0]
    if test=='member-count':d=[]
    elif test=='member-formal-name':r['formal_inventory'][1]['name']='invented'
    elif test=='member-formal-type':r['formal_inventory'][1]['type']='int'
    elif test=='member-state':r['compiler_state']['initialized']['compiler_flags']^=1
    elif test=='member-esp':r['esp']=0
    else:r[test[7:]]+=1
    f.write_text(json.dumps(d))
   elif test.startswith('final-'):
    if test in ('final-this','final-descriptor','final-address'):value['args'][test[6:]]+=1
    elif test=='final-formal-omission':value['formal_inventory'].pop()
    elif test=='final-formal-type':value['formal_inventory'][1]['type']='int'
    elif test=='final-caller':value['caller']['pc']+=1
    else:value['compiler_state']['initialized']['compiler_flags']^=1
   elif test in ('original-call','original-ret8'):
    f=o/'relocated-text.bin';d=bytearray(f.read_bytes());d[(0x12a05 if test=='original-call'else 0x12a78)-0x1000]^=1;f.write_bytes(d)
   else:
    f=o/'events.txt';text=f.read_text();match=re.search(r'^trap seq=%d rva=00012a18 .*\n'%sequence,text,re.M);assert match;row=match[0]
    if test=='original-ecx':row=re.sub(r'ecx=([0-9a-f]+)',lambda m:'ecx=%08x'%(int(m[1],16)+4),row)
    else:
     word={'original-return':0,'original-descriptor':1,'original-address':2}[test];stack=re.search(r'stack=([0-9a-f,]+)',row)[1].split(',');stack[word]='%08x'%(int(stack[word],16)+1);row=re.sub(r'stack=[0-9a-f,]+','stack='+','.join(stack),row)
    f.write_text(text[:match.start()]+row+text[match.end():])
   reject(lambda:arena_state.validate_primary(o,n,entry,value,at,sequence,module,candidate),test)
 print(json.dumps(dict(status='PASS',fresh_target_boundary_controls=len(tests),rejected=tests,seconds=time.perf_counter()-started,scope='Actual primary-entry/thiscall validator; two full paired positives separately pass; unchanged full-pipeline protocol/state guards reused'),indent=2))
if __name__=='__main__':main()
