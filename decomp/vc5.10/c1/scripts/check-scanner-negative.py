#!/usr/bin/env python3
"""Fresh focused scanner-entry and first actual argument-parser ABI controls."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]=dict(sha256=m.pair.sha(f),size=f.stat().st_size);p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();m.check(a.original,a.native)
 tests=['entry-missing','entry-duplicate','entry-rva','entry-symbol','entry-pc','entry-return','entry-formal','entry-argument','entry-caller','entry-snapshot','entry-crt','entry-invocation','native-argument','native-mode','native-formal','native-bytes','native-state','native-caller','original-entry-missing','original-entry-sequence','original-entry-return','original-parser-argument','original-parser-mode','original-parser-return','original-call','original-prefix','original-unknown-neighbor']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='scanner-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
   root=Path(temporary);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(m.PROJECT/'build'/name,b/name)
   if test.startswith('entry-'):
    f=n/'scanner-entry-observations.json';v=json.loads(f.read_text());r=v[0]
    if test=='entry-missing':v=[]
    elif test=='entry-duplicate':v.append(copy.deepcopy(r))
    elif test=='entry-rva':r['boundary_rva']=0x12900
    elif test=='entry-symbol':r['symbol']='parse_internal_switch'
    elif test=='entry-pc':r['pc']+=1
    elif test=='entry-return':r['return']+=1
    elif test=='entry-formal':r['formal_inventory']=[]
    elif test=='entry-argument':r['args']['arguments']+=4
    elif test=='entry-caller':r['caller']['symbol']='wrong'
    elif test=='entry-snapshot':r['compiler_state']=json.loads((n/'main-entry-observations.json').read_text())[0]['compiler_state']
    elif test=='entry-crt':r['crt_state']['crt_attached']^=1
    else:r['invocation']['argv_pointers'][1]+=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('native-'):
    f=n/'state.json';v=json.loads(f.read_text())
    if test=='native-argument':v['args']['argument']+=1
    elif test=='native-mode':v['args']['mode']=1
    elif test=='native-formal':v['formal_inventory'].pop()
    elif test=='native-bytes':v['argument_bytes'][0]^=1
    elif test=='native-state':v['compiler_state']['bss']['work_buffers'][0]['data']+=1
    else:v['caller']['pc']+=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   else:
    events=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    if test in ('original-call','original-prefix'):
     f=o/'relocated-text.bin';v=bytearray(f.read_bytes());v[(0x128cd if test=='original-call'else 0x128c1)-0x1000]^=1;f.write_bytes(v)
    elif test=='original-unknown-neighbor':f=o/('compiler-bss-%03d.bin'%seq);v=bytearray(f.read_bytes());v[0x100]^=1;f.write_bytes(v)
    else:
     f=o/'events.txt'
     if test=='original-entry-missing':events=re.sub(r'^trap seq=\d+ rva=000128b1 .*\n','',events,flags=re.M)
     elif test=='original-entry-sequence':events=re.sub(r'(^trap seq=)\d+( rva=000128b1 )',r'\g<1>'+str(seq)+r'\g<2>',events,flags=re.M)
     else:
      rva=0x128b1 if test=='original-entry-return'else 0x12900;match=re.search(r'(^trap seq=\d+ rva=%08x .*stack=)([0-9a-f,]+)'%rva,events,re.M);words=match[2].split(',');index=1 if test=='original-parser-argument'else 2 if test=='original-parser-mode'else 0;words[index]='00000001';events=events[:match.start(2)]+','.join(words)+events[match.end(2):]
     f.write_text(events)
    rehash(o,f.name)
   try:m.check(o,n,b)
   except(AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-started,rejected=tests)))
if __name__=='__main__':main()
