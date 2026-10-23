#!/usr/bin/env python3
"""Focused real-member entry and sole descriptor-store state checks."""
import argparse,copy,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
import arena_state
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def rejected(action,name):
 try:action()
 except(AssertionError,KeyError,ValueError):return
 raise AssertionError('accepted '+name)
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();m.check(a.original,a.native)
 candidate=json.loads((a.original/'candidate.json').read_text());state=json.loads((a.native/'state.json').read_text());events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);primary,pat=arena_state.primary_boundary(a.original,a.native,state,sequence,module);member,at=arena_state.member_boundary(a.original,a.native,state,sequence,module)
 tests=['member-missing','member-invocation','member-crt','member-this','member-descriptor','member-formal','member-caller','helper-formal-name','helper-formal-type','helper-argument','helper-caller-this','helper-caller-descriptor','helper-caller-address','helper-caller-return','descriptor-omitted','descriptor-wrong-owner','second-arena-write','reset-neighbor-write','snapshot-write','initialized-write','original-descriptor-store','original-neighbor','original-snapshot','original-code','original-argument','original-return']
 with tempfile.TemporaryDirectory(prefix='member-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as tmp:
  root=Path(tmp);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n);nativefile=n/'arena-member-entry-observations.json';native_old=nativefile.read_bytes()
  names=['events.txt','relocated-text.bin','compiler-bss-%03d.bin'%sequence,'compiler-snapshot-%03d.bin'%sequence];old={name:(o/name).read_bytes()for name in names}
  for test in tests:
   nativefile.write_bytes(native_old)
   for name,data in old.items():(o/name).write_bytes(data)
   value=copy.deepcopy(state)
   if test.startswith('member-'):
    d=json.loads(native_old);r=d[0]
    if test=='member-missing':d=[]
    elif test=='member-invocation':r['invocation']['args']['mode']^=1
    elif test=='member-crt':r['crt_state']['crt_attached']+=1
    elif test=='member-this':r['args']['this']+=4
    elif test=='member-descriptor':r['args']['descriptor']+=20
    elif test=='member-formal':r['formal_inventory'].pop()
    else:r['caller']['symbol']='c1_main'
    nativefile.write_text(json.dumps(d))
    def check_entry():
     changed,changed_at=arena_state.member_boundary(o,n,value,sequence,module)
     arena_state.validate_primary(o,n,primary,changed,pat,changed_at,module,candidate)
    rejected(check_entry,test);continue
   if test=='helper-formal-name':value['formal_inventory'][0]['name']='invented'
   elif test=='helper-formal-type':value['formal_inventory'][0]['type']='int'
   elif test=='helper-argument':value['args']['descriptor']+=20
   elif test.startswith('helper-caller-'):
    key=test[14:]
    if key=='return':value['caller']['pc']+=1
    else:value['caller']['args'][key]+=1
   elif test=='descriptor-omitted':value['compiler_state']['bss']['arenas'][0]['descriptor']=0
   elif test=='descriptor-wrong-owner':value['compiler_state']['bss']['arenas'][0]['descriptor']+=20
   elif test=='second-arena-write':value['compiler_state']['bss']['arenas'][1]['descriptor']=value['args']['descriptor']
   elif test=='reset-neighbor-write':value['compiler_state']['bss']['arenas'][0]['reset_value']=1
   elif test=='snapshot-write':value['compiler_state']['snapshot']['compiler_flags']^=1
   elif test=='initialized-write':value['compiler_state']['initialized']['compiler_flags']^=1
   elif test in ('original-descriptor-store','original-neighbor','original-snapshot'):
    f=o/('compiler-snapshot-%03d.bin'%sequence if test=='original-snapshot'else 'compiler-bss-%03d.bin'%sequence);d=bytearray(f.read_bytes());d[0 if test=='original-snapshot'else 0x8fc0 if test=='original-descriptor-store'else 0x8fc4]^=1;f.write_bytes(d)
   elif test=='original-code':f=o/'relocated-text.bin';d=bytearray(f.read_bytes());d[0x12a21-0x1000]^=1;f.write_bytes(d)
   else:
    f=o/'events.txt';text=f.read_text();match=re.search(r'^trap seq=%d rva=00012a7a .*\n'%sequence,text,re.M);assert match;row=match[0];stack=re.search(r'stack=([0-9a-f,]+)',row)[1].split(',');idx=0 if test=='original-return'else 1;stack[idx]='%08x'%(int(stack[idx],16)+1);row=re.sub(r'stack=[0-9a-f,]+','stack='+','.join(stack),row);f.write_text(text[:match.start()]+row+text[match.end():])
   rejected(lambda:arena_state.validate_member_writes(o,n,member,value,at,sequence,module,candidate),test)
 print(json.dumps(dict(status='PASS',fresh_target_state_controls=len(tests),rejected=tests,seconds=time.perf_counter()-started,scope='Actual member-entry and descriptor-store validators; independent complete normal/restore raw positives; prior unchanged controls reused'),indent=2))
if __name__=='__main__':main()
