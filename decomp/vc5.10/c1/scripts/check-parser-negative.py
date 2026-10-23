#!/usr/bin/env python3
"""Focused new-field/default/restore, repeated parser ABI, switchdata and getenv guards."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import main_fields
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
KEYS=['internal_bm_flags','internal_zb_value','memory_percent']
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]=dict(sha256=m.pair.sha(f),size=f.stat().st_size);p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();m.check(a.original,a.native);aux=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 tests=['metadata-default-'+k for k in KEYS]+['metadata-alias','metadata-snapshot','metadata-extent','entry-count','entry-order','entry-formal','entry-argument','entry-mode','entry-bytes','return-count','return-order','return-stack','return-value','return-pc','getenv-count','getenv-name','getenv-formal','getenv-result','getenv-stack','getenv-error','getenv-fpu','original-entry-count','original-return-value','original-return-stack','original-return-order','original-getenv-result','original-getenv-provider','original-getenv-name','original-table-target','original-parser-prefix']
 tests+=['original-default-'+k for k in KEYS]+['native-live-'+k for k in KEYS]+['native-snapshot-'+k for k in KEYS]
 if aux:tests+=['aux-before-'+k for k in KEYS]+['aux-saved-'+k for k in KEYS]+['aux-restored-'+k for k in KEYS]+['aux-original-saved-'+k for k in KEYS]
 for test in tests:
  if test.startswith('metadata-'):
   fields=json.loads((m.PROJECT/'cases/minimal/field-map.json').read_text());row=next(r for r in fields['modeled_initialized']if r['name']==('memory_percent'if not test.startswith('metadata-default-')else test[17:]))
   if test.startswith('metadata-default-'):row['default']^=1
   elif test=='metadata-alias':row['native_offset']=52
   elif test=='metadata-snapshot':row['snapshot_original_offset']+=4
   else:fields['main_fields']['initialized_unknown_bytes']=6496
   try:main_fields.expected_fields(fields)
   except(AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
  with tempfile.TemporaryDirectory(prefix='parser-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
   root=Path(temporary);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(m.PROJECT/'build'/name,b/name)
   if test.startswith('entry-')or test.startswith('return-'):
    returned=test.startswith('return-');f=n/('internal-parser-returns.json'if returned else 'internal-parser-entries.json');v=json.loads(f.read_text());r=v[0]
    if test.endswith('count'):v.pop()
    elif test.endswith('order'):v[0],v[1]=v[1],v[0]
    elif test=='entry-formal':r['formal_inventory'].pop()
    elif test=='entry-argument':r['argument']+=1
    elif test=='entry-mode':r['mode']=1
    elif test=='entry-bytes':r['bytes'][0]^=1
    else:r[{'return-stack':'esp','return-value':'eax','return-pc':'pc'}[test]]^=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('getenv-'):
    returned=test not in ('getenv-count','getenv-name','getenv-formal');f=n/('internal-getenv-returns.json'if returned else 'internal-getenv-entries.json');v=json.loads(f.read_text());r=v[0]
    if test=='getenv-count':v=[]
    elif test=='getenv-name':r['name']='CL'
    elif test=='getenv-formal':r['formal_inventory']=[]
    else:r[{'getenv-result':'eax','getenv-stack':'esp','getenv-error':'last_error','getenv-fpu':'fpu'}[test]]^=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('native-'):
    f=n/'state.json';v=json.loads(f.read_text());prefix='native-live-'if test.startswith('native-live-')else 'native-snapshot-';v['compiler_state']['initialized'if prefix=='native-live-'else 'snapshot'][test[len(prefix):]]^=1;f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith(('aux-before-','aux-saved-','aux-restored-')):
    f=n/'restore-observations.json';v=json.loads(f.read_text());prefix=next(p for p in ['aux-before-','aux-saved-','aux-restored-']if test.startswith(p));v['post_restore'if prefix=='aux-restored-'else 'before']['snapshot'if prefix=='aux-saved-'else 'initialized'][test[len(prefix):]]^=1;f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('original-default-')or test.startswith('aux-original-saved-'):
    prefix='original-default-'if test.startswith('original-default-')else 'aux-original-saved-';key=test[len(prefix):];rva,size,offset=main_fields.INITIALIZED[key]
    seq=0 if prefix=='original-default-'else int(re.findall(r'^trap seq=(\d+)',(o/'events.txt').read_text(),re.M)[-1]);region='compiler-data'if seq==0 else 'compiler-snapshot';f=o/('%s-%03d.bin'%(region,seq));v=bytearray(f.read_bytes());v[rva-0x5e000]^=1;f.write_bytes(v);rehash(o,f.name)
   elif test in ('original-table-target','original-parser-prefix'):
    f=o/'relocated-text.bin';v=bytearray(f.read_bytes());v[(0x12988 if test=='original-table-target'else 0x12900)-0x1000]^=1;f.write_bytes(v);rehash(o,f.name)
   else:
    f=o/'events.txt';v=f.read_text()
    if test=='original-entry-count':v=re.sub(r'^internal-parser-entry index=9 .*\n','',v,flags=re.M)
    else:
     pattern,old,new={
      'original-return-value':(r'^internal-parser-return index=0 .*$', 'eax=00000000','eax=00000001'),
      'original-return-stack':(r'^internal-parser-return index=0 .*$', 'esp=','esp=1'),
      'original-return-order':(r'^internal-parser-return index=0 .*$', 'index=0','index=1'),
      'original-getenv-result':(r'^internal-getenv .*$', 'result=00000000','result=00000001'),
      'original-getenv-provider':(r'^internal-getenv .*$', 'provider=','provider=1'),
      'original-getenv-name':(r'^internal-getenv .*$', 'name=MSC_CMD_FLAGS','name=CL')}[test]
     match=re.search(pattern,v,re.M);assert match;v=v[:match.start()]+match[0].replace(old,new,1)+v[match.end():]
    f.write_text(v);rehash(o,f.name)
   try:m.check(o,n,b)
   except(AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-started,rejected=tests)))
if __name__=='__main__':main()
