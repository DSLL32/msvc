#!/usr/bin/env python3
"""Reject persistent-session lifetime, ownership, branch and field corruption."""
import argparse,importlib.util,json,re,shutil,tempfile,time,struct
from pathlib import Path
import pdb_state
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]={'sha256':trial.pair.sha(f),'size':f.stat().st_size};p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();trial.check(a.original,a.native)
 aux=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 tests=['native-'+key for key in pdb_state.MEMBERS]+['native-layout','native-alias','original-final','original-default','metadata-gap','metadata-provider','metadata-field-alias']
 if aux:tests+=['before-clear-lifetime','after-clear-lifetime','before-helper-alias','before-helper-mode','helper-pc','missing-context-seed','missing-helper-seed','wrong-probe','original-gap-write','neighbor-change','wrong-branch']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='pdb-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as tmp:
   root=Path(tmp);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(trial.PROJECT/'build'/name,b/name)
   if test.startswith('metadata-'):
    field=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text());candidate=json.loads((o/'candidate.json').read_text());state=json.loads((n/'state.json').read_text());seq=int(re.findall(r'^trap seq=(\d+)',(o/'events.txt').read_text(),re.M)[-1])
    if test=='metadata-gap':field['pdb_context']['original_unmodeled_offsets']=[]
    elif test=='metadata-provider':field['pdb_context']['provider_layout_claimed']=True
    else:field['modeled_persistent'][1]['native_offset']=0
    try:pdb_state.validate(o,n,field,state,seq,'restore' if aux else 'normal',candidate,trial.PROJECT)
    except (AssertionError,KeyError):continue
    raise AssertionError('accepted '+test)
   elif test in ('original-final','original-default','original-gap-write'):
    seq=0 if test=='original-default' else int(re.findall(r'^trap seq=(\d+)',(o/'events.txt').read_text(),re.M)[-1]);f=o/('state-%03d.bin'%seq);r=bytearray(f.read_bytes());r[0x58 if test=='original-gap-write' else 0x48]^=1;f.write_bytes(r);rehash(o,f.name)
   elif test.startswith('missing-'):
    f=o/'events.txt';prefix='pdb-context-seed' if test=='missing-context-seed' else 'pdb-helper-seed';f.write_text(re.sub(r'^'+prefix+r' .*\n','',f.read_text(),flags=re.M));rehash(o,f.name)
   elif test=='wrong-probe':
    f=o/'pdb-context-probe.txt';f.write_text('2\n');rehash(o,f.name)
   elif test=='wrong-branch':
    f=n/'receipt.json';r=json.loads(f.read_text());r['pdb_reuse']^=1;f.write_text(json.dumps(r))
   elif test.startswith('native-'):
    f=n/'state.json';r=json.loads(f.read_text());c=r['compiler_state']
    if test=='native-layout':c['pdb_context_layout']['size']=28
    elif test=='native-alias':c['pdb_context_layout']['address']=c['live_addresses']['bss']+4
    else:c['pdb_context'][test[len('native-'):]]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   else:
    f=n/'restore-observations.json';r=json.loads(f.read_text())
    if test=='helper-pc':r['pdb_context_pc']=1
    elif test=='before-helper-mode':r['before_pdb_context']['reuse_state']^=1
    elif test=='neighbor-change':r['before_pdb_context']['bss']['pdb_signature']^=1
    else:
     phase={'before-clear-lifetime':'before','after-clear-lifetime':'post_restore','before-helper-alias':'before_pdb_context'}[test]
     r[phase]['pdb_context']['names']=r[phase]['pdb_context']['database']
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps({'fresh_affected_count':len(tests),'seconds':time.perf_counter()-started,'rejected':tests}))
if __name__=='__main__':main()
