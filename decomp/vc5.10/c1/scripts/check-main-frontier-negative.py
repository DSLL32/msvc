#!/usr/bin/env python3
"""Check genuine main argc/argv/callsite and the already reconstructed BOOL store."""
import argparse,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]=dict(sha256=trial.pair.sha(f),size=f.stat().st_size);p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();start=time.perf_counter();trial.check(a.original,a.native)
 tests=['native-argc','native-argv-pointer','native-argv-bytes','native-pass-mode','native-missing-formal','native-extra-formal','native-wrong-caller','original-argc','original-argv-pointer','original-return','original-pass-mode','original-call-opcode','original-mode-store','compiled-ELF','compiled-stability','compiled-source-manifest']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='main-frontier-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temp:
   r=Path(temp);o=r/'original';n=r/'native';b=r/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ['trial-audit.json','trial-audit-binding.json']:shutil.copy2(trial.PROJECT/'build'/name,b/name)
   if test.startswith('native-'):
    f=n/'state.json';v=json.loads(f.read_text())
    if test=='native-argc':v['args']['argc']+=1
    elif test=='native-argv-pointer':v['args']['argv']+=4
    elif test=='native-argv-bytes':v['argv'][0]='wrong.dll'
    elif test=='native-pass-mode':v['pass_mode']=0
    elif test=='native-missing-formal':v['formal_inventory'].pop()
    elif test=='native-extra-formal':v['formal_inventory'].append(dict(name='mode',type='int'))
    else:v['caller']['symbol']='wrong_caller'
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test in ('original-argc','original-argv-pointer','original-return'):
    f=o/'events.txt';text=f.read_text()
    def mutate(m):
     words=m[2].split(',');words[1 if test=='original-argc' else 2 if test=='original-argv-pointer' else 0]='00000001';return m[1]+','.join(words)
    text=re.sub(r'(^trap seq=\d+ rva=000127a7 .*stack=)([0-9a-f,]+)',mutate,text,flags=re.M);f.write_text(text);rehash(o,f.name)
   elif test=='original-pass-mode':
    seq=int(re.findall(r'^trap seq=(\d+)',(o/'events.txt').read_text(),re.M)[-1]);f=o/('state-%03d.bin'%seq);v=bytearray(f.read_bytes());struct.pack_into('<I',v,0x8c,0);f.write_bytes(v);rehash(o,f.name)
   elif test.startswith('original-'):
    f=o/'relocated-text.bin';v=bytearray(f.read_bytes());v[(0x1226f if test=='original-call-opcode' else 0x1225b)-0x1000]^=1;f.write_bytes(v);rehash(o,f.name)
   else:
    f=n/'compiled-build.json';v=json.loads(f.read_text())
    if test=='compiled-ELF':v['native_sha256']='0'*64
    elif test=='compiled-stability':v['stable_before_after']=False
    else:v['sources'].pop(next(iter(v['sources'])))
    f.write_text(json.dumps(v));rehash(n,f.name)
    q=n/'receipt.json';v=json.loads(q.read_text());v['compiled_build_sha256']=trial.pair.sha(f);q.write_text(json.dumps(v))
   try:trial.check(o,n,b)
   except (AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-start,rejected=tests)))
if __name__=='__main__':main()
