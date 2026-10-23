#!/usr/bin/env python3
"""Focused actual API, response-reference, profile inventory and output guards."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]=dict(sha256=m.pair.sha(f),size=f.stat().st_size);p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();m.check(a.original,a.native)
 tests=['empty-missing','empty-rva','empty-pc','empty-return','empty-formal','empty-after-API-snapshot','API-missing','API-pointer','API-formal','API-stack','API-output','API-before-bytes','API-error','API-fpu','profile-before-count','profile-after-count','profile-memory-count','profile-tag','profile-reference','file-tag','file-reference','file-payload','binding-target','binding-environment','binding-provider','binding-raw-output','binding-memory-status','binding-encoder','binding-factory','final-snapshot','final-neighbor','original-argument','original-provider','original-stack','original-error','original-output','original-output-neighbor','original-empty-code']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='system-info-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
   root=Path(temporary);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(m.PROJECT/'build'/name,b/name)
   if test.startswith('empty-'):
    f=n/'empty-entry-observations.json';v=json.loads(f.read_text());r=v[0]
    if test=='empty-missing':v=[]
    elif test=='empty-rva':r['boundary_rva']=0x12999
    elif test=='empty-pc':r['pc']+=1
    elif test=='empty-return':r['return']+=1
    elif test=='empty-formal':r['formal_inventory']=[dict(name='invented',type='int')]
    else:r['compiler_state']=json.loads((n/'state.json').read_text())['compiler_state']
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('API-')or test.startswith('profile-'):
    before=test in ('API-missing','API-pointer','API-formal','API-before-bytes','profile-before-count');f=n/('system-info-entries.json'if before else 'system-info-returns.json');v=json.loads(f.read_text());r=v[0]
    if test=='API-missing':v=[]
    elif test=='API-pointer':r['argument']+=4
    elif test=='API-formal':r['formal_inventory']=[]
    elif test=='API-stack':r['esp']+=4
    elif test in ('API-output','API-before-bytes'):r['bytes'][0]^=1
    elif test=='API-error':r['last_error']+=1
    elif test=='API-fpu':r['fpu']^=1
    elif test in ('profile-before-count','profile-after-count'):r['profile']['system_calls']+=1
    elif test=='profile-memory-count':r['profile']['memory_calls']=1
    elif test=='profile-tag':r['profile']['bound']=1
    else:r['profile']['reference']='0'*64
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('file-'):
    f=n/'system-info-profile.bin';v=bytearray(f.read_bytes());v[{'file-tag':0,'file-reference':4,'file-payload':68}[test]]^=1;f.write_bytes(v);rehash(n,f.name)
   elif test.startswith('binding-'):
    f=n/'system-info-binding.json';v=json.loads(f.read_text());key={'binding-target':'canonical_target_sha256','binding-environment':'environment_sha256','binding-provider':'provider_export_rva','binding-raw-output':'system_info_sha256','binding-memory-status':'memory_status_supplied','binding-encoder':'encoder_sha256','binding-factory':'factory_sha256'}[test]
    v[key]=True if key=='memory_status_supplied'else v[key]+1 if isinstance(v[key],int)else '0'*64;f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('final-'):
    f=n/'state.json';v=json.loads(f.read_text());v['compiler_state']['snapshot'if test=='final-snapshot'else 'initialized']['internal_bm_flags']^=1;f.write_text(json.dumps(v));rehash(n,f.name)
   elif test in ('original-argument','original-provider','original-stack','original-error'):
    f=o/'events.txt';v=f.read_text();pattern=r'^systeminfo-before .*\n'if test in ('original-argument','original-provider')else r'^systeminfo-after .*\n';match=re.search(pattern,v,re.M);assert match
    key={'original-argument':'argument=','original-provider':'provider=','original-stack':'esp=','original-error':'error=0'}[test];replacement='error=1'if test=='original-error'else key+'1';v=v[:match.start()]+match[0].replace(key,replacement,1)+v[match.end():];f.write_text(v);rehash(o,f.name)
   elif test=='original-empty-code':f=o/'relocated-text.bin';v=bytearray(f.read_bytes());v[0x12998-0x1000]^=1;f.write_bytes(v);rehash(o,f.name)
   elif test=='original-output':f=o/'system-info-after-000.bin';v=bytearray(f.read_bytes());v[4]^=1;f.write_bytes(v);rehash(o,f.name)
   else:
    events=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);f=o/('compiler-data-%03d.bin'%seq);v=bytearray(f.read_bytes());v[0x152c]^=1;f.write_bytes(v);rehash(o,f.name)
   try:m.check(o,n,b)
   except(AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-started,rejected=tests)))
if __name__=='__main__':main()
