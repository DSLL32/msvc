#!/usr/bin/env python3
"""Mutate work-entry, allocator ABI and new typed reset state; reject each."""
import argparse,copy,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
import work_state
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]=dict(sha256=trial.pair.sha(f),size=f.stat().st_size);p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();trial.check(a.original,a.native)
 aux=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 tests=['metadata-size','metadata-original-alias','metadata-native-alias','metadata-default','metadata-model','entry-missing','entry-duplicate','entry-rva','entry-symbol','entry-pc','entry-return','entry-formal-invented','entry-args-invented','entry-caller','entry-caller-pc','entry-caller-argc','entry-final-snapshot','entry-phase','entry-buffer','entry-crt','final-buffer','final-neighbor','final-phase','final-reserve','final-commit','final-formal-omitted','final-caller','final-caller-pc','original-work-missing','original-work-order','original-work-return','original-reserve','original-commit','original-first-call','original-second-call','original-limit','original-field-store','original-default','original-work-state','original-final-buffer','original-final-neighbor']
 if aux:tests+=['aux-native-before','aux-native-after','aux-original-before','aux-missing-seed','aux-original-default']
 for test in tests:
  if test.startswith('metadata-'):
   fields=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text());row=next(r for r in fields['modeled_bss'] if r['name']=='work_buffers[0].limit')
   if test=='metadata-size':row['bytes']=8
   elif test=='metadata-original-alias':row['original_rva']=0x51850
   elif test=='metadata-native-alias':row['native_offset']=788
   elif test=='metadata-default':row['default']=1
   else:fields['work_buffers']['limit_offset']+=1
   try:work_state.expected_fields(fields)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
  with tempfile.TemporaryDirectory(prefix='work-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temp:
   root=Path(temp);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ['trial-audit.json','trial-audit-binding.json']:shutil.copy2(trial.PROJECT/'build'/name,b/name)
   events=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);work_seq=int(re.findall(r'^trap seq=(\d+) rva=00012830 ',events,re.M)[0])
   if test.startswith('entry-'):
    f=n/'work-entry-observations.json';v=json.loads(f.read_text());r=v[0]
    if test=='entry-missing':v=[]
    elif test=='entry-duplicate':v.append(copy.deepcopy(r))
    elif test=='entry-rva':r['boundary_rva']=0x12873
    elif test=='entry-symbol':r['symbol']='reserve_work_buffer'
    elif test=='entry-pc':r['pc']=json.loads((n/'state.json').read_text())['pc']
    elif test=='entry-return':r['return']+=1
    elif test=='entry-formal-invented':r['formal_inventory']=[dict(name='invented',type='int')]
    elif test=='entry-args-invented':r['args']={'invented':1}
    elif test=='entry-caller':r['caller']['symbol']='wrong'
    elif test=='entry-caller-pc':r['caller']['pc']+=1
    elif test=='entry-caller-argc':r['caller']['args']['argc']+=1
    elif test=='entry-final-snapshot':r['compiler_state']=json.loads((n/'main-entry-observations.json').read_text())[0]['compiler_state']
    elif test=='entry-phase':r['compiler_state']['initialized']['compiler_phase']^=1
    elif test=='entry-buffer':r['compiler_state']['bss']['work_buffers'][1]['limit']=1
    else:r['crt_state']['crt_attached']^=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('final-'):
    f=n/'state.json';r=json.loads(f.read_text())
    if test=='final-buffer':r['compiler_state']['bss']['work_buffers'][0]['data']=1
    elif test=='final-neighbor':r['compiler_state']['bss']['command_argc']=1
    elif test=='final-phase':r['compiler_state']['initialized']['compiler_phase']^=1
    elif test=='final-reserve':r['args']['reserve_bytes']+=1
    elif test=='final-commit':r['args']['commit_bytes']+=1
    elif test=='final-formal-omitted':r['formal_inventory'].pop()
    elif test=='final-caller':r['caller']['symbol']='wrong'
    else:r['caller']['pc']+=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test.startswith('original-') or test.startswith('aux-original-'):
    if test in ('original-work-missing','original-work-order','original-work-return','original-reserve','original-commit'):
     f=o/'events.txt'
     if test=='original-work-missing':events=re.sub(r'^trap seq=\d+ rva=00012830 .*\n','',events,flags=re.M)
     elif test=='original-work-order':events=re.sub(r'(^trap seq=)\d+( rva=00012830 )',r'\g<1>'+str(seq)+r'\g<2>',events,flags=re.M)
     else:
      rva=0x12830 if test=='original-work-return' else 0x12873
      match=re.search(r'(^trap seq=\d+ rva=%08x .*stack=)([0-9a-f,]+)'%rva,events,re.M);words=match[2].split(',');index=0 if test=='original-work-return' else 1 if test=='original-reserve' else 2;words[index]='00000001';events=events[:match.start(2)]+','.join(words)+events[match.end(2):]
     f.write_text(events);rehash(o,f.name)
    elif test in ('original-first-call','original-second-call','original-limit','original-field-store'):
     f=o/'relocated-text.bin';v=bytearray(f.read_bytes());offset={'original-first-call':0x1283a,'original-second-call':0x1285b,'original-limit':0x12848,'original-field-store':0x12843}[test];v[offset-0x1000]^=1;f.write_bytes(v);rehash(o,f.name)
    else:
     chosen=0 if test in ('original-default','aux-original-default') else work_seq if test=='original-work-state' else seq
     if test=='aux-original-before':chosen=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0])
     f=o/('compiler-bss-%03d.bin'%chosen);v=bytearray(f.read_bytes());v[0x18 if test=='original-final-neighbor' else 0x1850]^=1;f.write_bytes(v);rehash(o,f.name)
   elif test=='aux-missing-seed':
    f=o/'events.txt';f.write_text(re.sub(r'^work-buffers-seed .*\n','',events,flags=re.M));rehash(o,f.name)
   else:
    f=n/'restore-observations.json';v=json.loads(f.read_text());v['before' if test=='aux-native-before' else 'post_restore']['bss']['work_buffers'][0]['data']^=1;f.write_text(json.dumps(v));rehash(n,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-started,rejected=tests)))
if __name__=='__main__':main()
