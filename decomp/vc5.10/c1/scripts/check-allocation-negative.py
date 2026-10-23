#!/usr/bin/env python3
"""Fail closed on allocator ABI, provider preservation, regions and defined bytes."""
import argparse,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import allocation_imports
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(directory,name):
 p=directory/'receipt.json';value=json.loads(p.read_text());f=directory/name;value['artifacts'][name]=dict(size=f.stat().st_size,sha256=trial.pair.sha(f));p.write_text(json.dumps(value))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();trial.check(a.original,a.native)
 tests=['report-cached-site','report-missing-site','report-extra-site','report-import-name','report-contract','report-unknown','report-rebuilt-count',
 'native-helper-count','native-helper-formal','native-helper-argument','native-helper-pc','native-helper-return','native-helper-stack','native-helper-esi','native-helper-result','native-helper-state',
 'native-api-count','native-api-formal','native-api-argument','native-api-pc','native-api-return','native-api-stack','native-api-edi','native-api-esi','native-api-result','native-api-error','native-api-fpu','native-region-protect','native-committed-byte',
 'native-final-data','native-final-limit','native-final-command','native-final-callback','native-final-phase','native-final-formal',
 'original-provider-file','original-provider-identity','original-api-args','original-api-edi','original-api-stack','original-api-result','original-api-error','original-region-state','original-region-size','original-region-protect','original-region-owner','original-committed-byte','original-helper-return','original-helper-esi','original-helper-stack','original-final-bss','original-final-data','original-final-snapshot']
 for test in tests:
  if test.startswith('report-'):
   report=json.loads((trial.PROJECT/'build/trial-audit.json').read_text());fn=report['functions']['_reserve_work_buffer']['register_imports']
   if test=='report-cached-site':fn['original']['resolved_register_calls'][str(0x10612889+1)]=fn['original']['resolved_register_calls'].pop(str(0x10612889))
   elif test=='report-missing-site':fn['original']['resolved_register_calls'].pop(str(0x106128a2))
   elif test=='report-extra-site':fn['original']['resolved_register_calls']['1']=dict(dll='kernel32.dll',name='VirtualAlloc')
   elif test=='report-import-name':fn['original']['resolved_register_calls'][str(0x10612889)]['name']='VirtualFree'
   elif test=='report-contract':report['register_import_inputs']['preservation_contracts'][0]['preserves']=[]
   elif test=='report-unknown':fn['original']['unresolved_register_calls'].append(1)
   else:fn['rebuilt']['direct_import_calls'].pop(next(iter(fn['rebuilt']['direct_import_calls'])))
   try:allocation_imports.check_report(report)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
  with tempfile.TemporaryDirectory(prefix='allocation-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temp:
   root=Path(temp);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ['trial-audit.json','trial-audit-binding.json']:shutil.copy2(trial.PROJECT/'build'/name,b/name)
   if test.startswith('native-helper-'):
    returns=test in ('native-helper-return','native-helper-stack','native-helper-esi','native-helper-result','native-helper-state');f=n/('work-reserve-returns.json'if returns else 'work-reserve-entries.json');v=json.loads(f.read_text());r=v[0]
    if test=='native-helper-count':v.pop()
    elif test=='native-helper-formal':r['formal_inventory'].pop()
    elif test=='native-helper-argument':r['raw_args'][1]+=1
    elif test=='native-helper-pc':r['pc']+=1
    elif test=='native-helper-return':r['pc']+=1
    elif test=='native-helper-stack':r['esp']+=4
    elif test=='native-helper-esi':r['esi']^=1
    elif test=='native-helper-result':r['eax']+=0x1000
    else:r['compiler_state']['bss']['work_buffers'][0]['data']=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('native-api-') or test=='native-region-protect':
    returned=test in ('native-api-return','native-api-stack','native-api-edi','native-api-esi','native-api-result','native-api-error','native-api-fpu','native-region-protect');f=n/('work-api-returns.json'if returned else 'work-api-entries.json');v=json.loads(f.read_text());r=v[0]
    if test=='native-api-count':v.pop()
    elif test=='native-api-formal':r['formal_inventory'].pop()
    elif test=='native-api-argument':r['args'][2]=0x1000
    elif test=='native-api-pc':r['pc']+=1
    elif test=='native-region-protect':r['maps']=r['maps'].replace('---p','rw-p')
    else:r[{'native-api-return':'pc','native-api-stack':'esp','native-api-edi':'edi','native-api-esi':'esi','native-api-result':'eax','native-api-error':'last_error','native-api-fpu':'fpu'}[test]]^=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('native-final-'):
    f=n/'state.json';v=json.loads(f.read_text());state=v['compiler_state']
    if test=='native-final-data':state['bss']['work_buffers'][1]['data']+=1
    elif test=='native-final-limit':state['bss']['work_buffers'][0]['limit']+=1
    elif test=='native-final-command':state['bss']['command_argc']+=1
    elif test=='native-final-callback':state['bss']['cleanup_callbacks'][10]^=1
    elif test=='native-final-phase':state['initialized']['compiler_phase']+=1
    else:v['formal_inventory'].pop()
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test in ('native-committed-byte','original-committed-byte','original-provider-file'):
    d=n if test.startswith('native')else o;f=d/('loaded-kernel32.bin'if test=='original-provider-file'else 'work-committed-000.bin');v=bytearray(f.read_bytes());v[0]^=1;f.write_bytes(v);rehash(d,f.name)
   elif test.startswith('original-final-'):
    events=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);region='compiler-bss'if test=='original-final-bss'else 'compiler-data'if test=='original-final-data'else 'compiler-snapshot';f=o/('%s-%03d.bin'%(region,seq));v=bytearray(f.read_bytes());v[0x100]^=1;f.write_bytes(v);rehash(o,f.name)
   else:
    f=o/'events.txt';v=f.read_text()
    pattern,old,new={
      'original-provider-identity':(r'^allocation-provider .*$', 'resolved=', 'resolved=1'),
      'original-api-args':(r'^allocation-before index=0 .*$', '00500000','00500001'),
      'original-api-edi':(r'^allocation-after index=0 .*$', ' edi=', ' edi=1'),
      'original-api-stack':(r'^allocation-after index=0 .*$', ' esp=', ' esp=1'),
      'original-api-result':(r'^allocation-after index=0 .*$', ' eax=', ' eax=1'),
      'original-api-error':(r'^allocation-error index=0 .*$', 'value=0','value=1'),
      'original-region-state':(r'^allocation-region index=0 .*$', 'state=00002000','state=00001000'),
      'original-region-size':(r'^allocation-region index=0 .*$', 'bytes=00500000','bytes=00400000'),
      'original-region-protect':(r'^allocation-region index=1 part=0 .*$', 'protect=00000004','protect=00000002'),
      'original-region-owner':(r'^allocation-region index=1 part=0 .*$', 'allocation=','allocation=1'),
      'original-helper-return':(r'^allocation-helper-return index=0 .*$', 'pc=','pc=1'),
      'original-helper-esi':(r'^allocation-helper-return index=0 .*$', 'esi=','esi=1'),
      'original-helper-stack':(r'^allocation-helper-return index=0 .*$', 'esp=','esp=1')}[test]
    m=re.search(pattern,v,re.M);assert m;v=v[:m.start()]+m[0].replace(old,new,1)+v[m.end():];f.write_text(v);rehash(o,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-started,rejected=tests)))
if __name__=='__main__':main()
