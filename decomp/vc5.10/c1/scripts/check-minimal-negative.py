#!/usr/bin/env python3
"""Fail-closed controls for compiler-pass input/ABI/frontier acceptance."""
import argparse,importlib.util,json,re,shutil,tempfile
from pathlib import Path
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def rehash(d,name):
 f=d/'receipt.json';r=json.loads(f.read_text());p=d/name;r['artifacts'][name]={'sha256':m.pair.sha(p),'size':p.stat().st_size};f.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();m.check(a.original,a.native)
 tests=['stale-native','missing-candidate-audit','missing-frontier','wrong-frontier','missing-invoke','original-argv','original-input','native-input','native-caller','native-argc','native-mode','native-argv','native-terminator','premature-mode','wrong-pc','wrong-module','wrong-heap-alias','wrong-cw']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='minimal-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temp:
   d=Path(temp);o=d/'original';n=d/'native';b=d/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for f in ['trial-audit.json','trial-audit-binding.json']:shutil.copy2(m.PROJECT/'build'/f,b/f)
   if test=='stale-native':
    f=n/'receipt.json';v=json.loads(f.read_text());v['native_sha256']='0'*64;f.write_text(json.dumps(v))
   elif test=='missing-candidate-audit':
    f=b/'trial-audit.json';v=json.loads(f.read_text());v['checked_functions']-=1;f.write_text(json.dumps(v));f=b/'trial-audit-binding.json';v=json.loads(f.read_text());v['report_sha256']=m.pair.sha(b/'trial-audit.json');f.write_text(json.dumps(v))
   elif test in ['missing-frontier','wrong-frontier','missing-invoke']:
    f=o/'events.txt';v=f.read_text()
    if test=='missing-frontier':v=re.sub(r'^frontier .*\n','',v,flags=re.M)
    elif test=='wrong-frontier':v=re.sub(r'^frontier .*$', 'frontier rva=deadbeef',v,flags=re.M)
    else:v=re.sub(r'^trap seq=\d+ rva=00012229 .*\n','',v,flags=re.M)
    f.write_text(v);rehash(o,f.name)
   elif test=='original-argv':f=o/'invoke-arg-000.bin';f.write_bytes(b'changed\0');rehash(o,f.name)
   elif test in ['original-input','native-input']:
    side=o if test=='original-input' else n;f=side/'test.c';f.write_bytes(b'int main(){return 1;}\n');rehash(side,f.name)
   else:
    f=n/'state.json';v=json.loads(f.read_text());caller=v['caller']
    if test=='native-caller':caller['symbol']='wrong'
    elif test=='native-argc':caller['args']['argc']+=1
    elif test=='native-mode':caller['args']['mode']=0
    elif test=='native-argv':caller['argv'][0]='changed'
    elif test=='native-terminator':caller['argv_terminator']=1
    elif test=='premature-mode':v['pass_mode']=1
    elif test=='wrong-pc':v['pc']=1
    elif test=='wrong-module':v['module_identity']['sha256']='0'*64
    elif test=='wrong-heap-alias':v['onexit_end']+=4
    elif test=='wrong-cw':v['x87_control']=0x037f
    f.write_text(json.dumps(v));rehash(n,f.name)
   try:m.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted negative '+test)
 print(json.dumps({'count':len(tests),'rejected':tests}))
if __name__=='__main__':main()
