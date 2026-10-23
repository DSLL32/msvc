#!/usr/bin/env python3
"""Reject wrong main boundary, typed defaults, snapshot transfer and first write."""
import argparse,copy,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path
import main_fields
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
def rehash(d,name):
 p=d/'receipt.json';r=json.loads(p.read_text());f=d/name;r['artifacts'][name]=dict(sha256=trial.pair.sha(f),size=f.stat().st_size);p.write_text(json.dumps(r))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();trial.check(a.original,a.native)
 aux=json.loads((a.native/'receipt.json').read_text())['probe']=='restore'
 tests=['metadata-model-size','metadata-sdk-base','entry-helper-snapshot','entry-crt-count','entry-crt-hook','entry-crt-error','entry-crt-fpu','entry-crt-onexit','entry-crt-instance','metadata-original-overlap','metadata-native-overlap','metadata-initialized-size','metadata-default','metadata-unmodeled','metadata-system-offset','metadata-command-alias','entry-missing','entry-duplicate','entry-helper-rva','entry-helper-symbol','entry-helper-pc','entry-return','entry-formal-omitted','entry-argc','entry-argv-pointer','entry-argv-bytes','entry-argv-terminator','entry-phase-after-write','entry-system-page','entry-snapshot-flags','native-final-phase','native-final-flags','native-final-system-word','native-final-command','native-caller-pc','original-main-entry-missing','original-entry-at-helper-sequence','original-entry-return','original-call-main','original-mode-store','original-phase-store','original-final-phase','original-final-unknown-byte','original-final-bss-neighbor','original-default-system-page','original-default-system-granularity','original-default-flags','original-entry-system-word','original-snapshot-system-page','original-unknown-snapshot-change']
 if aux:tests+=['aux-live-phase','aux-live-flags','aux-live-system-page','aux-live-owned-pointer','aux-saved-phase','aux-saved-system-word','aux-saved-owned-pointer','aux-postrestore-phase','aux-postrestore-system-page','aux-command-before','aux-command-after','aux-original-live-phase','aux-original-saved-owned-pointer','aux-missing-seed','aux-wrong-probe']
 for test in tests:
  if test.startswith('metadata-'):
   fields=json.loads((trial.PROJECT/'cases/minimal/field-map.json').read_text());rows={r['name']:r for r in fields['modeled_initialized']}
   if test=='metadata-model-size':fields['main_fields']['native_initialized_size']=56
   elif test=='metadata-sdk-base':fields['main_fields']['system_info_original_rva']+=4
   elif test=='metadata-original-overlap':rows['compiler_phase']['original_rva']=rows['first_call']['original_rva']
   elif test=='metadata-native-overlap':rows['c9_il']['native_offset']=4
   elif test=='metadata-initialized-size':rows['system_info']['bytes']=40
   elif test=='metadata-default':rows['system_info']['default']['dwPageSize']=8192
   elif test=='metadata-unmodeled':fields['main_fields']['initialized_unknown_bytes']=0
   elif test=='metadata-system-offset':fields['main_fields']['system_info_members']['dwPageSize']=0
   else:next(r for r in fields['modeled_bss'] if r['name']=='command_argv')['native_offset']=780
   try:main_fields.expected_fields(fields)
   except (AssertionError,KeyError,ValueError):continue
   raise AssertionError('accepted '+test)
  with tempfile.TemporaryDirectory(prefix='main-state-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temp:
   root=Path(temp);o=root/'original';n=root/'native';b=root/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);b.mkdir()
   for name in ['trial-audit.json','trial-audit-binding.json']:shutil.copy2(trial.PROJECT/'build'/name,b/name)
   events=(o/'events.txt').read_text();seq=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);entry_seq=int(re.findall(r'^trap seq=(\d+) rva=000127a7 ',events,re.M)[0])
   if test.startswith('entry-'):
    f=n/'main-entry-observations.json';v=json.loads(f.read_text());r=v[0]
    if test=='entry-helper-snapshot':r['compiler_state']=json.loads((n/'state.json').read_text())['compiler_state']
    elif test.startswith('entry-crt-'):
     key={'entry-crt-count':'crt_attached','entry-crt-hook':'raw_dllmain','entry-crt-error':'win32_last_error','entry-crt-fpu':'x87_control','entry-crt-onexit':'onexit_first','entry-crt-instance':'dll_instance'}[test];r['crt_state'][key]^=1
    elif test=='entry-missing':v=[]
    elif test=='entry-duplicate':v.append(copy.deepcopy(r))
    elif test=='entry-helper-rva':r['boundary_rva']=0x12830
    elif test=='entry-helper-symbol':r['symbol']='initialize_work_buffers'
    elif test=='entry-helper-pc':r['pc']=json.loads((n/'state.json').read_text())['pc']
    elif test=='entry-return':r['return']+=1
    elif test=='entry-formal-omitted':r['formal_inventory'].pop()
    elif test=='entry-argc':r['args']['argc']+=1
    elif test=='entry-argv-pointer':r['args']['argv']+=4
    elif test=='entry-argv-bytes':r['argv'][0]='wrong.dll'
    elif test=='entry-argv-terminator':r['argv_terminator']=1
    elif test=='entry-phase-after-write':r['compiler_state']['initialized']['compiler_phase']=1
    elif test=='entry-system-page':r['compiler_state']['initialized']['system_info']['dwPageSize']^=1
    else:r['compiler_state']['snapshot']['compiler_flags']^=1
    f.write_text(json.dumps(v));rehash(n,f.name)
   elif test.startswith('native-'):
    f=n/'state.json';r=json.loads(f.read_text())
    if test=='native-final-phase':r['compiler_state']['initialized']['compiler_phase']=2
    elif test=='native-final-flags':r['compiler_state']['initialized']['compiler_flags']^=1
    elif test=='native-final-system-word':r['compiler_state']['initialized']['system_info']['wProcessorRevision']^=1
    elif test=='native-final-command':r['compiler_state']['bss']['command_argc']=11
    else:r['caller']['pc']+=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   elif test in ('original-main-entry-missing','original-entry-at-helper-sequence','original-entry-return','aux-missing-seed'):
    f=o/'events.txt'
    if test=='original-main-entry-missing':events=re.sub(r'^trap seq=\d+ rva=000127a7 .*\n','',events,flags=re.M)
    elif test=='original-entry-at-helper-sequence':events=re.sub(r'(^trap seq=)\d+( rva=000127a7 )',r'\g<1>'+str(seq)+r'\g<2>',events,flags=re.M)
    elif test=='original-entry-return':events=re.sub(r'(^trap seq=\d+ rva=000127a7 .*stack=)[0-9a-f]+',r'\g<1>00000001',events,flags=re.M)
    else:events=re.sub(r'^main-fields-seed .*\n','',events,flags=re.M)
    f.write_text(events);rehash(o,f.name)
   elif test in ('original-call-main','original-mode-store','original-phase-store'):
    f=o/'relocated-text.bin';r=bytearray(f.read_bytes());r[({'original-call-main':0x1226f,'original-mode-store':0x1225c,'original-phase-store':0x127af}[test])-0x1000]^=1;f.write_bytes(r);rehash(o,f.name)
   elif test.startswith('original-') or test in ('aux-original-live-phase','aux-original-saved-owned-pointer'):
    chosen=seq;region='compiler-data';offset=0x13d8
    if test=='original-final-unknown-byte':offset=0x100
    elif test=='original-final-bss-neighbor':region='compiler-bss';offset=0x700
    elif test=='original-default-system-page':chosen=0;offset=0x150c
    elif test=='original-default-system-granularity':chosen=0;offset=0x1524
    elif test=='original-default-flags':chosen=0;offset=0xde4
    elif test=='original-entry-system-word':chosen=entry_seq;offset=0x152a
    elif test=='original-snapshot-system-page':region='compiler-snapshot';offset=0x150c
    elif test=='original-unknown-snapshot-change':region='compiler-snapshot';offset=0x100
    elif test=='aux-original-live-phase':chosen=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0])
    elif test=='aux-original-saved-owned-pointer':region='compiler-snapshot';offset=0x1510
    f=o/('%s-%03d.bin'%(region,chosen));r=bytearray(f.read_bytes());r[offset]^=1;f.write_bytes(r);rehash(o,f.name)
   elif test=='aux-wrong-probe':f=o/'main-fields-probe.txt';f.write_text('wrong\n');rehash(o,f.name)
   else:
    f=n/'restore-observations.json';r=json.loads(f.read_text())
    if test=='aux-command-before':r['before']['bss']['command_argc']^=1
    elif test=='aux-command-after':r['post_restore']['bss']['command_argv']=1
    else:
     phase='post_restore' if test.startswith('aux-postrestore') else 'before'
     values=r[phase]['snapshot' if test.startswith('aux-saved') else 'initialized']
     if test.endswith('phase'):values['compiler_phase']^=1
     elif test.endswith('flags'):values['compiler_flags']^=1
     else:values['system_info']['dwPageSize' if test.endswith('page') else 'lpMinimumApplicationAddress' if test.endswith('pointer') else 'wProcessorRevision']^=1
    f.write_text(json.dumps(r));rehash(n,f.name)
   try:trial.check(o,n,b)
   except (AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-started,rejected=tests)))
if __name__=='__main__':main()
