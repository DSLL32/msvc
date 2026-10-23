#!/usr/bin/env python3
"""Ensure completion acceptance rejects stale bindings and changed raw state."""
import argparse,importlib.util,json,re,shutil,struct,tempfile
from pathlib import Path
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]={'sha256':m.pair.sha(f),'size':f.stat().st_size};p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();m.check(a.original,a.native)
 tests=['stale-native','wrong-candidate','prefix-only-audit','missing-cold-chunk','missing-exit','missing-return','wrong-original-count','wrong-original-dangling-end','wrong-original-cw','stdout','native-exit','native-order','native-module','native-identity','native-count','native-dangling-end','native-last-error','native-cw','native-disabled','native-status']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='completion-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temporary:
   d=Path(temporary);o=d/'original';n=d/'native';b=d/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for f in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(m.PROJECT/'build'/f,b/f)
   if test=='stale-native':
    f=n/'receipt.json';r=json.loads(f.read_text());r['native_sha256']='0'*64;f.write_text(json.dumps(r))
   elif test=='wrong-candidate':
    f=n/'candidate.json';r=json.loads(f.read_text());r['functions']=[];f.write_text(json.dumps(r));rehash(n,f.name)
   elif test in ('prefix-only-audit','missing-cold-chunk'):
    f=b/'trial-audit.json';r=json.loads(f.read_text())
    if test=='prefix-only-audit':r['checked_functions']=2
    else:r['owned_code_chunks']['original']['functions']['_c1_crt_entry@12'].pop()
    f.write_text(json.dumps(r));f=b/'trial-audit-binding.json';r=json.loads(f.read_text());r['report_sha256']=m.pair.sha(b/'trial-audit.json');f.write_text(json.dumps(r))
   elif test in ('missing-exit','missing-return','wrong-original-cw'):
    f=o/'events.txt';r=f.read_text()
    if test=='missing-exit':r=re.sub(r'^exit=.*\n','',r,flags=re.M)
    elif test=='missing-return':r=re.sub(r'^crt-return .*\n','',r,count=1,flags=re.M)
    else:r=r.replace('x87-control=ffff027f','x87-control=ffff037f')
    f.write_text(r);rehash(o,f.name)
   elif test.startswith('wrong-original-'):
    raw=(o/'events.txt').read_text();seq=re.findall(r'^crt-return seq=(\d+)',raw,re.M)[-1];f=o/('state-%03d.bin'%int(seq));r=bytearray(f.read_bytes());struct.pack_into('<I',r,0x90 if test=='wrong-original-count' else 0x94,1);f.write_bytes(r);rehash(o,f.name)
   elif test=='stdout':f=n/'stdout.bin';f.write_bytes(b'wrong');rehash(n,f.name)
   else:
    f=n/'state.json';r=json.loads(f.read_text());returns=[v for v in r['events'] if v['kind']=='return'];last=returns[-1]
    if test=='native-exit':r['exit']=1
    elif test=='native-order':r['events'][0],r['events'][1]=r['events'][1],r['events'][0]
    elif test=='native-module':r['events'][0]['args']['module']=0
    elif test=='native-identity':last['module_identity']['sha256']='0'*64
    elif test=='native-count':last['crt_attached']=1
    elif test=='native-dangling-end':last['onexit_end']=0
    elif test=='native-last-error':last['win32_last_error']=0
    elif test=='native-cw':last['x87_control']=0x037f
    elif test=='native-disabled':last['thread_calls_disabled']=0
    elif test=='native-status':last['status']=0
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:m.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted negative '+test)
 print(json.dumps({'count':len(tests),'rejected':tests}))
if __name__=='__main__':main()
