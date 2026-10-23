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
 frontier=json.loads((a.original/'candidate.json').read_text())['expected_frontier']
 frontier_kind=frontier['kind']
 allocation=frontier_kind=='compiler-allocation'
 tests=['missing-frontier','candidate-only-prefix','snapshot-first','live-first','bss-count','bss-callback','missing-callback-slot','swapped-callback','snapshot-live-alias','native-layout','original-default','original-snapshot-field','field-map-overlap','field-map-alias','field-map-snapshot-offset','context-object','context-value','context-frame','original-context','current-context-object','current-context-value','current-context-frame','original-current-context','contexts-live-0','contexts-live-2','contexts-live-5','original-contexts','buffer-data','buffer-mode','buffer-descriptor','original-buffer-mode']
 if allocation:
  tests.remove('snapshot-first');tests.remove('original-snapshot-field');tests.append('premature-snapshot')
 tests+=['missing-postpoint','restore-zero-input','restore-post-field','restore-post-count','restore-post-callback','restore-pointer-identity','restore-pc','restore-line','original-post-field','original-clear-gap','restore-context-zero-input','restore-context-object','restore-context-value','restore-context-frame','restore-pre-seed-context','callback-seed-object','callback-seed-value','callback-seed-frame','callback-seed-pc','missing-callback-seed','restore-current-zero-input','restore-current-object','restore-current-value','restore-current-frame','current-seed-object','current-seed-value','current-seed-frame','current-seed-pc','restore-contexts-seed-0','restore-contexts-seed-2','restore-contexts-seed-5','restore-contexts-post-0','restore-contexts-post-2','restore-contexts-post-5','restore-contexts-callback-0','restore-contexts-callback-2','restore-contexts-callback-5','restore-contexts-pc','restore-buffer-seed-data','restore-buffer-seed-mode','restore-buffer-seed-descriptor','restore-buffer-post-data','restore-buffer-post-mode','restore-buffer-post-descriptor','restore-buffer-constructor-data','restore-buffer-constructor-mode','restore-buffer-constructor-descriptor','restore-buffer-constructor-pc'] if restore else ['allocation-size','allocation-pc','original-allocation-size']
 if frontier_kind=='initialization-helper':tests+=['wrong-helper-caller','wrong-original-helper-return']
 if frontier['rva']==0x123f6:tests+=['wrong-registration-callback','wrong-original-registration-callback']
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
   elif test in ('missing-frontier','missing-postpoint','original-allocation-size','missing-callback-seed','wrong-original-helper-return','wrong-original-registration-callback'):
    f=o/'events.txt';r=f.read_text()
    if test=='missing-frontier':r=re.sub(r'^frontier .*\n','',r,flags=re.M)
    elif test=='missing-postpoint':r=re.sub(r'^trap seq=\d+ rva=000303d5 .*\n','',r,flags=re.M)
    elif test=='wrong-original-helper-return':r=re.sub(r'(^trap seq=\d+ rva=%08x .*stack=)[0-9a-f]+'%frontier['rva'],r'\g<1>00000001',r,flags=re.M)
    elif test=='wrong-original-registration-callback':r=re.sub(r'(^trap seq=\d+ rva=000123f6 .*stack=[0-9a-f]+,)[0-9a-f]+',r'\g<1>00000001',r,flags=re.M)
    elif test=='missing-callback-seed':r=re.sub(r'^context-seed .*\n','',r,flags=re.M)
    else:
     r=re.sub(r'(^trap seq=\d+ rva=0000e799 .*stack=[0-9a-f]+,)00001994',r'\g<1>00000004',r,flags=re.M)
    f.write_text(r);rehash(o,f.name)
   elif test.startswith('original-'):
    raw=(o/'events.txt').read_text();last=int(re.findall(r'^trap seq=(\d+)',raw,re.M)[-1])
    if test=='original-default':f=o/'compiler-data-000.bin';offset=0x1530
    elif test=='original-snapshot-field':f=o/('compiler-snapshot-%03d.bin'%last);offset=0x1530
    elif test=='original-context':f=o/('compiler-bss-%03d.bin'%last);offset=0x18c8
    elif test=='original-current-context':f=o/('compiler-bss-%03d.bin'%last);offset=0x18b8
    elif test=='original-contexts':f=o/('compiler-bss-%03d.bin'%last);offset=0x1858
    elif test=='original-buffer-mode':f=o/('compiler-bss-%03d.bin'%last);offset=0x4054
    else:
     post=int(re.findall(r'^trap seq=(\d+) rva=000303d5 ',raw,re.M)[0]);f=o/(('compiler-data-' if test=='original-post-field' else 'compiler-bss-')+'%03d.bin'%post);offset=0x1530 if test=='original-post-field' else 0x40
    r=bytearray(f.read_bytes());struct.pack_into('<I',r,offset,1 if test in ('original-clear-gap','original-context','original-current-context','original-contexts','original-buffer-mode') else 0);f.write_bytes(r);rehash(o,f.name)
   elif test.startswith('restore-') or test.startswith('callback-seed-') or test.startswith('current-seed-'):
    f=n/'restore-observations.json';r=json.loads(f.read_text());post=r['post_restore']
    if test=='restore-zero-input':r['before']['snapshot']['first_call']=0
    elif test=='restore-post-field':post['initialized']['first_call']=0
    elif test=='restore-post-count':post['bss']['cleanup_count']=7
    elif test=='restore-post-callback':post['bss']['cleanup_callbacks'][0]=1
    elif test=='restore-pointer-identity':post['bss']['saved_data']+=4
    elif test=='restore-pc':r['post_restore_pc']=1
    elif test=='restore-line':r['post_restore_line']+=1
    elif test=='restore-context-zero-input':r['before']['bss']['context']['value']=0
    elif test=='restore-pre-seed-context':r['pre_seed']['bss']['context']['value']=1
    elif test=='callback-seed-pc':r['callback_pc']=1
    elif test.startswith('callback-seed-'):r['before_callback']['bss']['context'][{'callback-seed-object':'object','callback-seed-value':'value','callback-seed-frame':'saved_frame'}[test]]=0
    elif test=='restore-buffer-constructor-pc':r['buffer_constructor_pc']=1
    elif test.startswith('restore-buffer-'):
     key={'data':'data','mode':'mode','descriptor':'file_descriptor'}[test.rsplit('-',1)[1]]
     target=r['before'] if '-seed-' in test else r['before_buffer_constructor'] if '-constructor-' in test else post
     target['bss']['output_buffer'][key]=0 if '-seed-' in test or '-constructor-' in test else 1
    elif test=='restore-contexts-pc':r['contexts_callback_pc']=1
    elif test.startswith('restore-contexts-'):
     index=int(test.rsplit('-',1)[1]);key={0:'object',2:'value',5:'saved_frame'}[index]
     target=r['before'] if '-seed-' in test else r['before_contexts_callback'] if '-callback-' in test else post
     target['bss']['contexts'][index][key]=0 if '-seed-' in test or '-callback-' in test else 1
    elif test=='restore-current-zero-input':r['before']['bss']['current_context']['value']=0
    elif test.startswith('restore-current-'):post['bss']['current_context'][{'restore-current-object':'object','restore-current-value':'value','restore-current-frame':'saved_frame'}[test]]=1
    elif test=='current-seed-pc':r['current_callback_pc']=1
    elif test.startswith('current-seed-'):r['before_current_callback']['bss']['current_context'][{'current-seed-object':'object','current-seed-value':'value','current-seed-frame':'saved_frame'}[test]]=0
    elif test.startswith('restore-context-'):post['bss']['context'][{'restore-context-object':'object','restore-context-value':'value','restore-context-frame':'saved_frame'}[test]]=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test.startswith('allocation-'):
    f=n/'allocation-observations.json';r=json.loads(f.read_text());r[0]['size' if test=='allocation-size' else 'pc']=4 if test=='allocation-size' else 1;f.write_text(json.dumps(r));rehash(n,f.name)
   else:
    f=n/'state.json';r=json.loads(f.read_text());c=r['compiler_state']
    if test=='wrong-registration-callback':r['args']['callback']^=1
    elif test=='wrong-helper-caller':r['caller']['symbol']='wrong_caller'
    elif test=='snapshot-first':c['snapshot']['first_call']=0
    elif test=='premature-snapshot':c['snapshot']={'first_call':1}
    elif test=='live-first':c['initialized']['first_call']^=1
    elif test=='bss-count':c['bss']['cleanup_count']=7
    elif test=='bss-callback':c['bss']['cleanup_callbacks'][0]=1
    elif test=='missing-callback-slot':c['initialization_callbacks'].pop()
    elif test=='swapped-callback':c['initialization_callbacks'][1],c['initialization_callbacks'][2]=c['initialization_callbacks'][2],c['initialization_callbacks'][1]
    elif test=='snapshot-live-alias':c['bss']['saved_data']=c['live_addresses']['initialized']
    elif test=='native-layout':c['native_layout']['cleanup_callbacks']=4
    elif test.startswith('buffer-'):c['bss']['output_buffer'][{'buffer-data':'data','buffer-mode':'mode','buffer-descriptor':'file_descriptor'}[test]]=1
    elif test.startswith('contexts-live-'):
     index=int(test.rsplit('-',1)[1]);c['bss']['contexts'][index][{0:'object',2:'value',5:'saved_frame'}[index]]=1
    elif test.startswith('current-context-'):c['bss']['current_context'][{'current-context-object':'object','current-context-value':'value','current-context-frame':'saved_frame'}[test]]=1
    elif test.startswith('context-'):c['bss']['context'][{'context-object':'object','context-value':'value','context-frame':'saved_frame'}[test]]=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:m.check(o,n,b)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted negative '+test)
 print(json.dumps({'count':len(tests),'probe':'restore' if restore else 'normal','rejected':tests}))
if __name__=='__main__':main()
