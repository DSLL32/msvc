#!/usr/bin/env python3
"""Negative controls for the explicitly limited value-copy projection."""
import argparse,copy,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
import value_state
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'))
trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(directory,name):
 p=directory/'receipt.json';r=json.loads(p.read_text());f=directory/name
 r['artifacts'][name]={'sha256':trial.pair.sha(f),'size':f.stat().st_size};p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args()
 started=time.perf_counter();trial.check(a.original,a.native)
 restore=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 tests=['native-source-low','native-source-high','native-destination-low','native-destination-sign','native-flag','native-value-width','native-source-alias','original-source-low','original-source-high','original-destination-high','original-flag','original-default-value','original-copy-tail','missing-copy-entry','field-tail-equivalence','field-size','field-frontier']
 if restore:tests+=['wrong-sign','zero-before-clear','wrong-post-clear','wrong-copy-input','wrong-cached-input','wrong-flag-input','wrong-copy-pc','missing-copy-observation','missing-seed-log','changed-probe-input','changed-adjacent-value']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='value-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as tmp:
   d=Path(tmp);o=d/'original';n=d/'native';b=d/'audit'
   shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(trial.PROJECT/'build'/name,b/name)
   if test.startswith('field-'):
    field=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text())
    key,value={'field-tail-equivalence':('modeled_integer_bytes',24),'field-size':('native_value_size',24),'field-frontier':('hard_frontier_rva',0x127a7)}[test]
    field['value_copy'][key]=value
    try:value_state.expected_fields(field)
    except AssertionError:continue
    raise AssertionError('accepted '+test)
   if test=='wrong-sign':
    f=n/'receipt.json';r=json.loads(f.read_text());r['value_copy_sign']='positive' if r['value_copy_sign']=='negative' else 'negative';f.write_text(json.dumps(r))
   elif test=='changed-probe-input':
    f=o/'value-probe.txt';f.write_text('0'+f.read_text());rehash(o,f.name)
   elif test in ('missing-copy-entry','missing-seed-log'):
    f=o/'events.txt';r=f.read_text();pattern=r'^trap seq=\d+ rva=00012646 .*\n' if test=='missing-copy-entry' else r'^value-seed .*\n'
    f.write_text(re.sub(pattern,'',r,flags=re.M));rehash(o,f.name)
   elif test.startswith('original-'):
    events=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    if test=='original-default-value':seq=0
    f=o/('compiler-bss-%03d.bin'%seq);raw=bytearray(f.read_bytes())
    offset={'original-source-low':0x18a0,'original-source-high':0x18a4,'original-destination-high':0x510c,'original-flag':0x5100,'original-default-value':0x18a0,'original-copy-tail':0x5114}[test]
    raw[offset]^=1;f.write_bytes(raw);rehash(o,f.name)
   elif test.startswith('native-'):
    f=n/'state.json';r=json.loads(f.read_text());s=r['compiler_state']
    if test=='native-value-width':s['native_layout']['value_size']=24
    elif test=='native-source-alias':s['native_layout']['saved_literal']=520
    else:
     key,mask={'native-source-low':('literal',1),'native-source-high':('literal',1<<32),'native-destination-low':('saved_literal',1),'native-destination-sign':('saved_literal',1<<63),'native-flag':('saved_literal_available',1)}[test]
     s['bss'][key]^=mask
    f.write_text(json.dumps(r));rehash(n,f.name)
   else:
    f=n/'restore-observations.json';r=json.loads(f.read_text())
    if test=='wrong-copy-pc':r['value_copy_pc']=1
    elif test=='missing-copy-observation':del r['before_value_copy']
    else:
     phase,key={'zero-before-clear':('before','literal'),'wrong-post-clear':('post_restore','saved_literal'),'wrong-copy-input':('before_value_copy','literal'),'wrong-cached-input':('before_value_copy','saved_literal'),'wrong-flag-input':('before_value_copy','saved_literal_available'),'changed-adjacent-value':('before_value_copy','cleanup_count')}[test]
     r[phase]['bss'][key]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps({'count':len(tests),'seconds':time.perf_counter()-started,'rejected':tests}))
if __name__=='__main__':main()
