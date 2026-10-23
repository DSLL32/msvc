#!/usr/bin/env python3
"""Reject changed typed fields, callback slots, restore observations and allocation ABI."""
import argparse,importlib.util,json,re,shutil,struct,tempfile
from pathlib import Path
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]={'sha256':m.pair.sha(f),'size':f.stat().st_size};p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();m.check(a.original,a.native)
 restore=json.loads((a.original/'receipt.json').read_text()).get('probe')=='restore'
 tests=['missing-frontier','candidate-only-prefix','snapshot-first','live-first','bss-count','bss-callback','missing-callback-slot','swapped-callback','snapshot-live-alias','native-layout','original-default','original-snapshot-field','field-map-overlap','field-map-alias','field-map-snapshot-offset']
 tests+=['missing-postpoint','restore-zero-input','restore-post-field','restore-post-count','restore-post-callback','restore-pointer-identity','restore-pc','restore-line','original-post-field','original-clear-gap'] if restore else ['allocation-size','allocation-pc','original-allocation-size']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='state-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temporary:
   d=Path(temporary);o=d/'original';n=d/'native';b=d/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for f in ('trial-audit.json','trial-audit-binding.json'):shutil.copy2(m.PROJECT/'build'/f,b/f)
   if test=='candidate-only-prefix':
    f=b/'trial-audit.json';r=json.loads(f.read_text());r['checked_functions']-=1;f.write_text(json.dumps(r));f=b/'trial-audit-binding.json';r=json.loads(f.read_text());r['report_sha256']=m.pair.sha(b/'trial-audit.json');f.write_text(json.dumps(r))
   elif test.startswith('field-map-'):
    import compiler_state
    fake=d/'project';(fake/'cases/minimal').mkdir(parents=True);(fake/'build/native').mkdir(parents=True);(fake/'src').mkdir()
    (fake/'build/native/c1-host').symlink_to(m.PROJECT/'build/native/c1-host');shutil.copy2(m.PROJECT/'src/pass.c',fake/'src/pass.c')
    candidate=json.loads((o/'candidate.json').read_text());field=json.loads((m.PROJECT/candidate['field_map']).read_text())
    if test=='field-map-overlap':field['modeled_bss'][1]['original_rva']=field['modeled_bss'][0]['original_rva']
    elif test=='field-map-alias':field['modeled_bss'][1]['native_offset']=0
    else:field['modeled_initialized'][0]['snapshot_native_offset']=4
    (fake/candidate['field_map']).write_text(json.dumps(field));previous=compiler_state.PROJECT;compiler_state.PROJECT=fake
    raw=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',raw,re.M)[-1]);base=int(re.search(r'module name=c1n32_l.dll actual=([0-9a-f]+)',raw)[1],16)
    try:compiler_state.validate(o,n,json.loads((n/'state.json').read_text()),seq,base,candidate,candidate['expected_frontier'],'restore' if restore else 'normal')
    except (AssertionError,KeyError,ValueError):continue
    finally:compiler_state.PROJECT=previous
    raise AssertionError('accepted negative '+test)
   elif test in ('missing-frontier','missing-postpoint','original-allocation-size'):
    f=o/'events.txt';r=f.read_text()
    if test=='missing-frontier':r=re.sub(r'^frontier .*\n','',r,flags=re.M)
    elif test=='missing-postpoint':r=re.sub(r'^trap seq=\d+ rva=000303d5 .*\n','',r,flags=re.M)
    else:
     r=re.sub(r'(^trap seq=\d+ rva=0000e799 .*stack=[0-9a-f]+,)00001994',r'\g<1>00000004',r,flags=re.M)
    f.write_text(r);rehash(o,f.name)
   elif test.startswith('original-'):
    raw=(o/'events.txt').read_text();last=int(re.findall(r'^trap seq=(\d+)',raw,re.M)[-1])
    if test=='original-default':f=o/'compiler-data-000.bin';offset=0x1530
    elif test=='original-snapshot-field':f=o/('compiler-snapshot-%03d.bin'%last);offset=0x1530
    else:
     post=int(re.findall(r'^trap seq=(\d+) rva=000303d5 ',raw,re.M)[0]);f=o/(('compiler-data-' if test=='original-post-field' else 'compiler-bss-')+'%03d.bin'%post);offset=0x1530 if test=='original-post-field' else 0x40
    r=bytearray(f.read_bytes());struct.pack_into('<I',r,offset,0 if test!='original-clear-gap' else 1);f.write_bytes(r);rehash(o,f.name)
   elif test.startswith('restore-'):
    f=n/'restore-observations.json';r=json.loads(f.read_text());post=r['post_restore']
    if test=='restore-zero-input':r['before']['snapshot']['first_call']=0
    elif test=='restore-post-field':post['initialized']['first_call']=0
    elif test=='restore-post-count':post['bss']['cleanup_count']=7
    elif test=='restore-post-callback':post['bss']['cleanup_callbacks'][0]=1
    elif test=='restore-pointer-identity':post['bss']['saved_data']+=4
    elif test=='restore-pc':r['post_restore_pc']=1
    elif test=='restore-line':r['post_restore_line']+=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test.startswith('allocation-'):
    f=n/'allocation-observations.json';r=json.loads(f.read_text());r[0]['size' if test=='allocation-size' else 'pc']=4 if test=='allocation-size' else 1;f.write_text(json.dumps(r));rehash(n,f.name)
   else:
    f=n/'state.json';r=json.loads(f.read_text());c=r['compiler_state']
    if test=='snapshot-first':c['snapshot']['first_call']=0
    elif test=='live-first':c['initialized']['first_call']=1
    elif test=='bss-count':c['bss']['cleanup_count']=7
    elif test=='bss-callback':c['bss']['cleanup_callbacks'][0]=1
    elif test=='missing-callback-slot':c['initialization_callbacks'].pop()
    elif test=='swapped-callback':c['initialization_callbacks'][1],c['initialization_callbacks'][2]=c['initialization_callbacks'][2],c['initialization_callbacks'][1]
    elif test=='snapshot-live-alias':c['bss']['saved_data']=c['live_addresses']['initialized']
    elif test=='native-layout':c['native_layout']['cleanup_callbacks']=4
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:m.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted negative '+test)
 print(json.dumps({'count':len(tests),'probe':'restore' if restore else 'normal','rejected':tests}))
if __name__=='__main__':main()
