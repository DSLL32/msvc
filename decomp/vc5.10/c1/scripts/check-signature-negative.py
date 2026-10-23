#!/usr/bin/env python3
"""Reject signature width/default/state/seed/ABI corruptions independently."""
import argparse,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]={'sha256':trial.pair.sha(f),'size':f.stat().st_size};p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();start=time.perf_counter();trial.check(a.original,a.native)
 aux=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 tests=['native-signature','native-offset','native-alias','original-signature','original-default','metadata-width','metadata-default','metadata-alias']
 if aux:tests+=['missing-seed','wrong-probe','zero-reset-input','wrong-reset-result','wrong-callback-input','wrong-callback-pc','missing-after-copy','wrong-after-copy-pc','wrong-after-copy-state','changed-neighbor']
 for name in tests:
  with tempfile.TemporaryDirectory(prefix='signature-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as tmp:
   root=Path(tmp);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for f in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(trial.PROJECT/'build'/f,b/f)
   if name.startswith('metadata-'):
    import signature_state
    field=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text())
    key,value={'metadata-width':('bytes',8),'metadata-default':('default',1),'metadata-alias':('original_rva',0x518a0)}[name]
    field['pdb_signature'][key]=value
    try:signature_state.expected_fields(field)
    except AssertionError:continue
    raise AssertionError('accepted '+name)
   elif name.startswith('original-'):
    seq=0 if name=='original-default' else int(re.findall(r'^trap seq=(\d+)',(o/'events.txt').read_text(),re.M)[-1]);f=o/('compiler-bss-%03d.bin'%seq);r=bytearray(f.read_bytes());r[0x4de0]^=1;f.write_bytes(r);rehash(o,f.name)
   elif name=='missing-seed':
    f=o/'events.txt';f.write_text(re.sub(r'^signature-seed .*\n','',f.read_text(),flags=re.M));rehash(o,f.name)
   elif name=='wrong-probe':
    f=o/'signature-probe.txt';f.write_text('0 0\n');rehash(o,f.name)
   elif name.startswith('native-'):
    f=n/'state.json';r=json.loads(f.read_text());c=r['compiler_state']
    if name=='native-signature':c['bss']['pdb_signature']=0
    else:c['native_layout']['pdb_signature']=536 if name=='native-alias' else 541
    f.write_text(json.dumps(r));rehash(n,f.name)
   else:
    f=n/'restore-observations.json';r=json.loads(f.read_text())
    if name=='missing-after-copy':del r['after_value_copy']
    elif name=='wrong-after-copy-pc':r['after_value_copy_pc']=1
    elif name=='wrong-callback-pc':r['signature_pc']=1
    else:
     phase,key={'zero-reset-input':('before','pdb_signature'),'wrong-reset-result':('post_restore','pdb_signature'),'wrong-callback-input':('before_signature','pdb_signature'),'wrong-after-copy-state':('after_value_copy','saved_literal'),'changed-neighbor':('before_signature','cleanup_count')}[name]
     r[phase]['bss'][key]^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+name)
 print(json.dumps({'fresh_affected_count':len(tests),'seconds':time.perf_counter()-start,'rejected':tests}))
if __name__=='__main__':main()
