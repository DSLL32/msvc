#!/usr/bin/env python3
"""Reject incomplete PCH state, invented formals and wrong tail/RET/reset evidence."""
import argparse,copy,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
import pch_state
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'))
trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(directory,name):
 receipt=directory/'receipt.json';value=json.loads(receipt.read_text());path=directory/name
 value['artifacts'][name]=dict(sha256=trial.pair.sha(path),size=path.stat().st_size);receipt.write_text(json.dumps(value))
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args()
 start=time.perf_counter();trial.check(args.original,args.native)
 aux=json.loads((args.native/'receipt.json').read_text())['probe']=='restore'
 tests=['metadata-base','metadata-size','metadata-native-offset','metadata-handle-count','metadata-member-offset','metadata-default','metadata-larger-layout','native-formal-omitted','native-formal-added','native-entry-pc','native-receiver','native-return-address','native-return-eax','native-return-esp','native-return-edi','native-return-values','native-return-neighbor','native-final-first','native-final-last','original-entry-receiver','original-entry-inherited-return','original-return-eax','original-return-esp','original-return-edi','original-wrapper-opcode','original-member-opcode','original-return-neighbor','original-final-first','original-final-last','original-default','original-prospective-kind','native-prospective-kind','missing-original-member-return','missing-native-member-return']
 if aux:tests+=['before-clear-scalar','before-clear-pointer','post-clear-pointer','member-entry-scalar','member-entry-pointer','original-member-scalar','original-member-pointer','missing-member-seed','wrong-member-probe']
 for test in tests:
  if test.startswith('metadata-'):
   fields=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text());model=fields['pch_state'];row=next(r for r in fields['modeled_bss'] if r['name']=='pch_state')
   if test=='metadata-base':model['original_rva']+=4
   elif test=='metadata-size':model['modeled_bytes']=64
   elif test=='metadata-native-offset':model['native_offset']-=4
   elif test=='metadata-handle-count':model['mapping_handle_count']=5
   elif test=='metadata-member-offset':model['members']['mapped_pages']=60
   elif test=='metadata-default':row['default']['mapping_handles'][-1]=1
   else:model['larger_image_arena_layouts_known']=True
   try:pch_state.expected_fields(fields)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
  with tempfile.TemporaryDirectory(prefix='pch-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temporary:
   root=Path(temporary);o=root/'original';n=root/'native';b=root/'audit'
   shutil.copytree(args.original,o);shutil.copytree(args.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(trial.PROJECT/'build'/name,b/name)
   events=(o/'events.txt').read_text();last=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);entered=int(re.findall(r'^trap seq=(\d+) rva=00011e2a ',events,re.M)[0]);returned=int(re.findall(r'^pch-member-return seq=(\d+) ',events,re.M)[0])
   if test in ('original-prospective-kind','native-prospective-kind'):
    d=o if test.startswith('original') else n;f=d/'receipt.json';r=json.loads(f.read_text());r['kind']='original-prospective-tail-chain' if d==o else 'native-prospective-tail-chain';f.write_text(json.dumps(r))
   elif test.startswith('native-formal') or test in ('native-entry-pc','native-receiver','native-return-address','member-entry-scalar','member-entry-pointer'):
    f=n/'pch-entry-observations.json';r=json.loads(f.read_text());v=r[0]
    if test=='native-formal-omitted':v['formal_inventory']=[]
    elif test=='native-formal-added':v['formal_inventory'].append(dict(name='invented',type='int'))
    elif test=='native-entry-pc':v['pc']+=1
    elif test=='native-receiver':v['ecx']+=4
    elif test=='native-return-address':v['return']+=1
    elif test=='member-entry-scalar':v['values'][0]^=1
    else:v['values'][-1]^=8
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test.startswith('native-return-') or test=='missing-native-member-return':
    f=n/'pch-return-observations.json';r=json.loads(f.read_text())
    if test=='missing-native-member-return':r=[]
    elif test=='native-return-eax':r[0]['eax']+=4
    elif test=='native-return-esp':r[0]['esp']+=4
    elif test=='native-return-edi':r[0]['edi']^=1
    elif test=='native-return-values':r[0]['values'][-1]=1
    else:r[0]['bss_bytes'][707]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test.startswith('native-final-'):
    f=n/'state.json';r=json.loads(f.read_text());r['compiler_state']['bss']['pch_state']['image_count' if test.endswith('first') else 'mapped_pages']=1;f.write_text(json.dumps(r));rehash(n,f.name)
   elif test in ('original-entry-receiver','original-entry-inherited-return'):
    f=o/'events.txt';key='ecx' if test=='original-entry-receiver' else 'stack';f.write_text(re.sub(r'(^trap seq=\d+ rva=00011e2a .*'+key+'=)[0-9a-f]+',r'\g<1>00000001',events,flags=re.M));rehash(o,f.name)
   elif test.startswith('original-return-') and test!='original-return-neighbor':
    f=o/'events.txt';key=test.rsplit('-',1)[1];f.write_text(re.sub(r'(^pch-member-return .* '+key+'=)[0-9a-f]+',r'\g<1>00000001',events,flags=re.M));rehash(o,f.name)
   elif test in ('original-wrapper-opcode','original-member-opcode'):
    f=o/'relocated-text.bin';r=bytearray(f.read_bytes());r[(0x127a1 if test=='original-wrapper-opcode' else 0x11e2b)-0x1000]^=1;f.write_bytes(r);rehash(o,f.name)
   elif test.startswith('original-'):
    seq=0 if test=='original-default' else returned if test=='original-return-neighbor' else entered if test.startswith('original-member') else last
    f=o/('compiler-bss-%03d.bin'%seq);r=bytearray(f.read_bytes());offset=0x8f64 if test=='original-return-neighbor' else 0x8fa8 if test.endswith('last') or test.endswith('pointer') else 0x8f68;r[offset]^=1;f.write_bytes(r);rehash(o,f.name)
   elif test.startswith(('before-clear','post-clear')):
    f=n/'restore-observations.json';r=json.loads(f.read_text());phase='post_restore' if test.startswith('post') else 'before';key='image_count' if test.endswith('scalar') else 'mapped_pages';r[phase]['bss']['pch_state'][key]^=1;f.write_text(json.dumps(r));rehash(n,f.name)
   elif test=='missing-original-member-return' or test=='missing-member-seed':
    f=o/'events.txt';prefix='pch-member-return' if test=='missing-original-member-return' else 'pch-member-seed';f.write_text(re.sub(r'^'+prefix+r' .*\n','',events,flags=re.M));rehash(o,f.name)
   elif test=='wrong-member-probe':
    f=o/'pch-member-probe.txt';f.write_text('11e2a 58f68 40\n');rehash(o,f.name)
   else:raise AssertionError(test)
   try:trial.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-start,rejected=tests)))
if __name__=='__main__':main()
