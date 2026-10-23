#!/usr/bin/env python3
"""Reject pool receiver/argument/RET4, storage and nonzero reset corruption."""
import argparse,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
import pool_state
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(directory,name):
 p=directory/'receipt.json';r=json.loads(p.read_text());f=directory/name
 r['artifacts'][name]={'sha256':trial.pair.sha(f),'size':f.stat().st_size};p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--only',action='append',help='only these affected controls, reporting other names unexecuted');a=p.parse_args();started=time.perf_counter();trial.check(a.original,a.native)
 aux=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 member=any(row['name']==pool_state.METHOD for row in json.loads((a.original/'candidate.json').read_text())['functions'])
 fields=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text());last_offset=fields['pools'][-1]['original_object_rva']-0x50000
 tests=['missing-formals','extra-formal','entry-missing-formal','entry-extra-formal','entry-index','receiver-alias','missing-new-entry','missing-new-return']+['field-'+key for key in pool_state.MEMBERS]+['missing-pool','layout','receiver','argument','entry-ecx','entry-argument','entry-pc','entry-return','missing-entry','original-ecx','original-argument','original-default','original-final','metadata-base','metadata-alias','metadata-extent','metadata-linkage','ret4']
 if member:
  tests.remove('receiver');tests.remove('argument')
  tests+=['return-eax','return-esp','return-pc','return-values','missing-return','original-return-eax','original-return-esp','missing-original-return','original-neighbor','entry-values']
 if aux and member:tests+=['missing-member-seed','wrong-member-probe','post-pdb-observer-pc']
 if aux:tests+=['before-clear-'+key for key in pool_state.MEMBERS]+['after-clear','pre-seed','before-late-callback','missing-seed','wrong-probe']
 all_tests=list(tests)
 if a.only:
  assert set(a.only)<=set(tests);tests=[test for test in tests if test in a.only]
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='pool-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as tmp:
   root=Path(tmp);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(trial.PROJECT/'build'/name,b/name)
   if test.startswith('metadata-') or test=='ret4':
    fields=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text());candidate=json.loads((o/'candidate.json').read_text());state=json.loads((n/'state.json').read_text());seq=int(re.findall(r'^trap seq=(\d+)',(o/'events.txt').read_text(),re.M)[-1])
    if test=='metadata-base':fields['pools'][-1]['original_object_rva']+=4
    elif test=='metadata-alias':fields['pools'][-1]['native_offset']=540
    elif test=='metadata-extent':fields['pools'][-1]['original_object_extent_unknown']=False
    elif test=='metadata-linkage':fields['pools'][-1]['native_linkage_symbol']='wrong_method'
    saved=pool_state.subprocess.check_output
    if test=='ret4':
     def badret(command,**kwargs):
      value=saved(command,**kwargs)
      return value.replace('ret    0x4','ret    0x0') if command[0]=='objdump' else value
     pool_state.subprocess.check_output=badret
    try:pool_state.validate(o,n,fields,state,seq,'restore' if aux else 'normal',candidate,trial.PROJECT)
    except (AssertionError,KeyError,ValueError):continue
    finally:pool_state.subprocess.check_output=saved
    raise AssertionError('accepted '+test)
   elif test.startswith('return-') or test in ('missing-return','missing-new-return'):
    f=n/'pool-return-observations.json';r=json.loads(f.read_text())
    if test=='missing-return':r=[]
    elif test=='missing-new-return':r.pop()
    elif test=='return-values':r[-1]['values'][2]^=1
    else:r[-1][test[len('return-'):]]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test.startswith('entry-') or test in ('missing-entry','missing-new-entry','receiver-alias'):
    f=n/'pool-entry-observations.json';r=json.loads(f.read_text())
    if test=='missing-entry':r=[]
    elif test=='missing-new-entry':r.pop()
    elif test=='receiver-alias':r[-1]['ecx']=r[0]['ecx'] if len(r)>1 else r[-1]['ecx']+16
    elif test=='entry-missing-formal':r[-1]['formal_inventory'].pop()
    elif test=='entry-extra-formal':r[-1]['formal_inventory'].append({'name':'third_argument','type':'unsigned int'})
    elif test=='entry-values':r[-1]['values'][1]^=1
    else:r[-1][test[len('entry-'):]]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test.startswith('original-') or test in ('missing-seed','missing-member-seed','missing-original-return'):
    if test in ('original-default','original-final','original-neighbor'):
     seq=0 if test=='original-default' else int(re.findall(r'^trap seq=(\d+)',(o/'events.txt').read_text(),re.M)[-1]);f=o/('compiler-bss-%03d.bin'%seq);r=bytearray(f.read_bytes());r[last_offset if test!='original-neighbor' else last_offset-1]^=1;f.write_bytes(r)
    else:
     f=o/'events.txt';r=f.read_text()
     if test=='original-ecx':r=re.sub(r'(^trap seq=\d+ rva=000126fc .*ecx=)[0-9a-f]+',r'\g<1>00000001',r,flags=re.M)
     elif test=='original-return-eax':r=re.sub(r'(^pool-return .*eax=)[0-9a-f]+',r'\g<1>00000001',r,flags=re.M)
     elif test=='original-return-esp':r=re.sub(r'(^pool-return .*esp=)[0-9a-f]+',r'\g<1>00000001',r,flags=re.M)
     elif test=='missing-original-return':r=re.sub(r'^pool-return .*\n','',r,flags=re.M)
     elif test=='missing-member-seed':r=re.sub(r'^pool-member-seed .*\n','',r,flags=re.M)
     elif test=='original-argument':r=re.sub(r'(^trap seq=\d+ rva=000126fc .*stack=[0-9a-f]+,)[0-9a-f]+',r'\g<1>00000002',r,flags=re.M)
     else:r=re.sub(r'^pool-seed .*\n','',r,flags=re.M)
     f.write_text(r)
    rehash(o,f.name)
   elif test in ('wrong-probe','wrong-member-probe'):
    f=o/('pools-probe.txt' if test=='wrong-probe' else 'pool-member-probe.txt');f.write_text('5507c\n');rehash(o,f.name)
   elif test.startswith('before-clear-') or test in ('after-clear','pre-seed','before-late-callback','post-pdb-observer-pc'):
    f=n/'restore-observations.json';r=json.loads(f.read_text())
    if test=='post-pdb-observer-pc':
     r['after_output_pdb_context_pc']=1;f.write_text(json.dumps(r));rehash(n,f.name)
     try:trial.check(o,n,b)
     except (AssertionError,KeyError,ValueError):continue
     raise AssertionError('accepted '+test)
    phase={'after-clear':'post_restore','pre-seed':'pre_seed','before-late-callback':'before_output_pdb_context'}.get(test,'before')
    key=test[len('before-clear-'):] if test.startswith('before-clear-') else 'cursor'
    r[phase]['bss']['pools'][-1][key]^=1;f.write_text(json.dumps(r));rehash(n,f.name)
   else:
    f=n/'state.json';r=json.loads(f.read_text());c=r['compiler_state']
    if test=='missing-formals':del r['formal_inventory']
    elif test=='extra-formal':r['formal_inventory'].append({'name':'missing_argument','type':'int'})
    elif test.startswith('field-'):c['bss']['pools'][-1][test[6:]]^=1
    elif test=='layout':c['native_layout']['pool_size']=20
    elif test=='missing-pool':c['bss']['pools']=[]
    else:r['args']['this' if test=='receiver' else 'minimum_pages']+=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps({'fresh_affected_count':len(tests),'not_executed':[test for test in all_tests if test not in tests],'seconds':time.perf_counter()-started,'rejected':tests}))
if __name__=='__main__':main()
