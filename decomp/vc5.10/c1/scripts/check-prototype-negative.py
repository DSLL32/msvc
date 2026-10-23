#!/usr/bin/env python3
"""Reject wrong prototype extent, reset, seed, neighbors and original store evidence."""
import argparse,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import prototype_state
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]={'sha256':trial.pair.sha(f),'size':f.stat().st_size};p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();start=time.perf_counter();trial.check(a.original,a.native)
 aux=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 tests=['metadata-extent','metadata-base','metadata-alias','metadata-default','metadata-snapshot','metadata-larger-record','metadata-duplicate-store','native-first','native-last','native-length','original-default-first','original-default-last','original-final-first','original-final-last','original-neighbor','wrong-original-store']
 if aux:tests+=['before-clear-first','before-clear-last','after-clear-last','callback-first','callback-last','callback-pc','native-neighbor','pre-seed','missing-seed','wrong-probe','original-before-clear','original-before-callback']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='prototype-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temporary:
   root=Path(temporary);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(trial.PROJECT/'build'/name,b/name)
   if test.startswith('metadata-'):
    fields=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text());m=fields['empty_record_prototype'];row=next(r for r in fields['modeled_bss'] if r['name']=='empty_record_prototype')
    if test=='metadata-extent':m['bytes']=24
    elif test=='metadata-base':m['original_rva']+=4
    elif test=='metadata-alias':m['native_offset']=544
    elif test=='metadata-default':row['default'][-1]=1
    elif test=='metadata-snapshot':m['included_in_initialized_snapshot']=True
    elif test=='metadata-larger-record':m['larger_record_layout_known']=True
    else:m['original_store_offsets'].pop()
    try:prototype_state.expected_fields(fields)
    except (AssertionError,KeyError,ValueError):continue
    raise AssertionError('accepted '+test)
   elif test.startswith('original-') or test=='wrong-original-store':
    events=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);offset=0x8f50
    if test.startswith('original-default'):seq=0
    elif test=='original-before-clear':seq=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0])
    elif test=='original-before-callback':seq=int(re.findall(r'^trap seq=(\d+) rva=0001277b ',events,re.M)[0])
    if test.endswith('-last'):offset+=19
    elif test=='original-neighbor':offset+=20
    f=o/('compiler-bss-%03d.bin'%seq)
    if test=='wrong-original-store':f=o/'relocated-text.bin';offset=0x1277d-0x1000
    r=bytearray(f.read_bytes());r[offset]^=1;f.write_bytes(r);rehash(o,f.name)
   elif test=='missing-seed':
    f=o/'events.txt';f.write_text(re.sub(r'^prototype-seed .*\n','',f.read_text(),flags=re.M));rehash(o,f.name)
   elif test=='wrong-probe':f=o/'prototype-probe.txt';f.write_text('58f50 18 01 a0\n');rehash(o,f.name)
   elif test.startswith('native-') and test!='native-neighbor':
    f=n/'state.json';r=json.loads(f.read_text());v=r['compiler_state']['bss']['empty_record_prototype']
    if test=='native-length':v.pop()
    else:v[0 if test=='native-first' else -1]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   else:
    f=n/'restore-observations.json';r=json.loads(f.read_text())
    if test=='callback-pc':r['prototype_pc']=1
    elif test=='native-neighbor':r['before_prototype']['bss']['pdb_signature']^=1
    else:
     phase='before' if test.startswith('before-clear') else 'post_restore' if test=='after-clear-last' else 'pre_seed' if test=='pre-seed' else 'before_prototype'
     r[phase]['bss']['empty_record_prototype'][0 if test.endswith('first') else -1]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps({'fresh_count':len(tests),'seconds':time.perf_counter()-start,'rejected':tests}))
if __name__=='__main__':main()
