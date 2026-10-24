#!/usr/bin/env python3
"""Fresh generated-state and authenticated public-entry rejection controls."""
import argparse,copy,importlib.util,json,re,tempfile,time
from pathlib import Path
import checksum_state

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter()
 spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial);trial.check(a.original,a.native)
 candidate=json.loads((a.original/'candidate.json').read_text());report=json.loads(Path('build/trial-audit.json').read_text());state=json.loads((a.native/'state.json').read_text());events=(a.original/'events.txt').read_text()
 labels=['low-last-byte','high-last-byte','transform-last-byte','neighbor-state','snapshot-changed','wrong-category','locale-pointer-alias','locale-full-input','missing-formal','wrong-formal-type','raw-category','raw-locale','native-provider-pc','already-entered-body','native-caller','native-argc','native-return','missing-checksum-entry','checksum-formal','checksum-return','checksum-stack-cleanup','checksum-callee-save','checksum-feedback-taps','missing-checksum-return','checksum-return-state','missing-public-entry','provider-address','provider-iat','original-category','original-empty-input','original-checksum-return','original-checksum-stack','original-provider-input','original-output-neighbor','restore-seed-not-cleared']
 with tempfile.TemporaryDirectory(prefix='checksums-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
  root=Path(temporary);o=root/'original';n=root/'native'
  for source,destination in ((a.original,o),(a.native,n)):
   destination.mkdir()
   for f in source.iterdir():
    if f.is_file():(destination/f.name).symlink_to(f.resolve())
  returned_seq=int(re.search(r'^checksum-return seq=(\d+)',events,re.M)[1])
  tracked=[n/'state.json',n/'checksum-entries.json',n/'checksum-returns.json',n/'restore-observations.json',o/'events.txt',o/'setlocale-input-000.bin',o/('compiler-bss-%03d.bin'%returned_seq)]
  saved={f:f.read_bytes()for f in tracked if f.exists()}
  for label in labels:
   for f,raw in saved.items():
    if f.is_symlink():f.unlink()
    f.write_bytes(raw)
   v=copy.deepcopy(state)
   if label.endswith('-last-byte'):v['compiler_state']['checksum_state'][label[:-10]][-1]^=1
   elif label=='neighbor-state':v['compiler_state']['bss']['arenas'][0]['reset_value']+=1
   elif label=='snapshot-changed':v['compiler_state']['snapshot']['compiler_flags']^=1
   elif label=='wrong-category':v['args']['category']=1
   elif label=='locale-pointer-alias':v['args']['locale']+=1
   elif label=='locale-full-input':v['public_entry']['locale_bytes']=[65,0]
   elif label=='missing-formal':v['formal_inventory'].pop()
   elif label=='wrong-formal-type':v['formal_inventory'][0]['type']='unsigned int'
   elif label=='raw-category':v['public_entry']['stack'][1]=1
   elif label=='raw-locale':v['public_entry']['stack'][2]+=1
   elif label=='native-provider-pc':v['pc']=1
   elif label=='already-entered-body':v['pc']+=1
   elif label=='native-caller':v['caller']['symbol']='c1_main'
   elif label=='native-argc':v['invocation']['args']['argc']-=1
   elif label=='native-return':v['public_entry']['return']+=1
   elif label in ('missing-checksum-entry','checksum-formal','checksum-return','checksum-stack-cleanup','checksum-callee-save','checksum-feedback-taps'):
    f=n/'checksum-entries.json';rows=json.loads(f.read_text())
    if label=='missing-checksum-entry':rows=[]
    elif label=='checksum-formal':rows[0]['formal_inventory']=[dict(name='invented',type='int')]
    elif label=='checksum-return':rows[0]['return']+=1
    elif label=='checksum-stack-cleanup':rows[0]['esp']+=4
    elif label=='checksum-callee-save':rows[0]['preserved']['ebx']^=1
    else:rows[0]['feedback_taps'][-1]=0
    f.write_text(json.dumps(rows))
   elif label in ('missing-checksum-return','checksum-return-state'):
    f=n/'checksum-returns.json';rows=json.loads(f.read_text())
    if label=='missing-checksum-return':rows=[]
    else:rows[0]['compiler_state']['checksum_state']['low'][0]^=1
    f.write_text(json.dumps(rows))
   elif label=='original-provider-input':(o/'setlocale-input-000.bin').write_bytes(b'A\0')
   elif label=='original-output-neighbor':
    f=o/('compiler-bss-%03d.bin'%returned_seq);raw=bytearray(f.read_bytes());raw[0]^=1;f.write_bytes(raw)
   elif label=='restore-seed-not-cleared':
    f=n/'restore-observations.json';obs=json.loads(f.read_text());obs['post_restore']['checksum_state']['high'][-1]=1;f.write_text(json.dumps(obs))
   else:
    text=events
    if label=='missing-public-entry':text=re.sub(r'^public-entry .*\n','',text,flags=re.M)
    elif label=='provider-address':text=text.replace('provider=78005729','provider=7800572a')
    elif label=='provider-iat':text=text.replace('iat=0005d120','iat=0005d124')
    elif label=='original-category':text=text.replace('category=2 locale=','category=1 locale=')
    elif label=='original-empty-input':text=text.replace('locale=106601c1','locale=106601c2')
    elif label=='original-checksum-return':text=re.sub(r'(^checksum-return .*pc=)[0-9a-f]+',r'\g<1>10612b67',text,flags=re.M)
    elif label=='original-checksum-stack':text=re.sub(r'(^checksum-return .*esp=)([0-9a-f]+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),text,flags=re.M)
    else:raise AssertionError('unknown control '+label)
    assert text!=events;(o/'events.txt').write_text(text)
   (n/'state.json').write_text(json.dumps(v))
   try:checksum_state.validate(o,n,candidate,report)
   except(AssertionError,KeyError,ValueError,IndexError):continue
   raise AssertionError('accepted '+label)
 print(json.dumps(dict(status='PASS',fresh_target_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,scope='Fresh full restore positive, complete generated tables/reset, true checksum entry/return and qualified two-argument public-import entry; locale success excluded.'),indent=2))
if __name__=='__main__':main()
