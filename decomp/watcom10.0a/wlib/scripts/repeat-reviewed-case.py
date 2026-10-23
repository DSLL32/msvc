#!/usr/bin/env python3
"""Explicit original-only reference repeat, never invoked by native fixtures.
New original executions require case approval; this script does not grant it.
"""
from pathlib import Path
import sys,json,os,subprocess,time,signal,hashlib,re,shutil
out=Path(sys.argv[1]);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();first=json.loads((out/'receipt.json').read_text());assert first['actual_normal_target_exit_status'] is not None and 'normal' in first['status'];archive=out/'attempt-01';archive.mkdir(exist_ok=False)
for name in ['receipt.json','stdin.bin']:shutil.copyfile(out/name,archive/name)
for name in ['events.txt','stdout.bin','stderr.bin','launcher.stdout.bin','launcher.stderr.bin']:(out/name).rename(archive/name)
env=json.loads((out/'host-environment.json').read_text());assert sha(out/'host-environment.json')==first['host_environment_sha256'];target=Path('/root/msvc2/toolchains/watcom10.0a/.patched/WLIB.EXE');original=target.parent.parent/'binnt/WLIB.EXE';helper=Path('/root/.ai-tmp/msvc2/watcom10.0a/wlib/resource-guard-build-04-indirect-exit/diagnose-startup.exe');assert sha(target)==first['target_sha256_after'] and sha(original)==first['original_sha256_after'] and sha(helper)==first['helper_sha256'];cwd=Path(first['cwd']);assert not list(cwd.iterdir())
# Rehash the actual reviewed configuration files for every explicit repeat.
def shared_path(value):
 drive,tail=value[:2],value[3:]
 base={'R:':Path('/root/msvc2'),'S:':Path('/root/.ai-tmp/msvc2')}[drive]
 return base/Path(tail.replace(chr(92),'/'))
assert sha(shared_path(first['argv'][8]))==first['sites_sha256']
assert sha(shared_path(first['argv'][13]))==first['descriptor_sha256']
start=time.monotonic()
with(out/'launcher.stdout.bin').open('wb') as stdout,(out/'launcher.stderr.bin').open('wb') as stderr:
 p=subprocess.Popen(first['argv'],cwd=cwd,env=env,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr,start_new_session=True)
 try:p.wait(timeout=45)
 except subprocess.TimeoutExpired:
  os.killpg(p.pid,signal.SIGKILL);p.wait();raise SystemExit('repeat outer timeout HOLD')
elapsed=time.monotonic()-start;events=(out/'events.txt').read_text();normal=re.findall(r'^exit status=(\d+)$',events,re.M);artifacts=[str(x.relative_to(cwd)) for x in cwd.rglob('*')];second={k:v for k,v in first.items() if k not in ['files','exit_cleanup','guard_unknown_owners','actual_main_arguments_hex','actual_resource_open_paths_hex','actual_createprocess_command']};second.update(launcher_unix_pid=p.pid,launcher_status=p.returncode,elapsed_seconds=elapsed,actual_normal_target_exit_status=int(normal[-1]) if normal else None,files_after=artifacts,target_sha256_after=sha(target),original_sha256_after=sha(original),repeat_of=str(archive/'receipt.json'),repeated_input='literal same helper command, actual target argv/cwd, frozen complete host environment, exact64LF pipe and fresh outputhandles; earlier raw outputs moved intact to attempt-01 to permit same outputfilenames')
for key,pattern in [('guard_owner_entries',r'^guard-owner result=0 '),('guard_reader_entries',r'^guard-reader result=0 '),('reader_returns',r'^guard-reader-return result=0 '),('owner_returns',r'^guard-owner-return result=0 '),('rearms',r'^guard-step-rearmed ')]:second[key]=len(re.findall(pattern,events,re.M))
for key,pattern in [('actual_main_arguments_hex',r'^actual-main-argument address=[^ ]+ bytes=([0-9a-f]+) length=\d+$'),('actual_resource_open_paths_hex',r'^actual-CreateFileA-filename address=[^ ]+ bytes=([0-9a-f]+) length=\d+$'),('actual_createprocess_command',r'^owned-child .* command=(.*)$'),('guard_unknown_owners',r'^guard-unknown-owner caller=([0-9a-f]+)$'),('exit_cleanup',r'^(?:owned-exit-confirmed|failure-owned-child).*$')]:second[key]=re.findall(pattern,events,re.M)
second['files']={n:dict(size=(out/n).stat().st_size,sha256=sha(out/n)) for n in ['stdin.bin','events.txt','stdout.bin','stderr.bin','launcher.stdout.bin','launcher.stderr.bin']}
checks=dict(identical_streams=all(sha(out/n)==sha(archive/n) for n in ['stdin.bin','stdout.bin','stderr.bin']),same_exit=second['actual_normal_target_exit_status']==first['actual_normal_target_exit_status'],same_artifacts=artifacts==first['files_after'],same_guard_counts=all(second[k]==first[k] for k in ['guard_owner_entries','guard_reader_entries','reader_returns','owner_returns','rearms']),final_guard='guard-final-segments ' in events,normal_event=bool(normal),no_forced_termination='terminate-owned-child ' not in events,unchanged_targets=second['original_sha256_after']==first['original_sha256_after'] and second['target_sha256_after']==first['target_sha256_after'],same_observed_main_arguments=second['actual_main_arguments_hex']==first['actual_main_arguments_hex'],same_observed_resource_open_path=second['actual_resource_open_paths_hex']==first['actual_resource_open_paths_hex']);second['reproducibility_checks']=checks;second['actual_observations_parsed_from_own_events']=True;second['status']='normal patched case reproducible' if all(checks.values()) else 'reproducibility HOLD; no dependent run';(out/'repeat-receipt.json').write_text(json.dumps(second,indent=2)+'\n');repeat=out/'attempt-02';repeat.mkdir()
for name in ['stdin.bin','events.txt','stdout.bin','stderr.bin','launcher.stdout.bin','launcher.stderr.bin']:shutil.copyfile(out/name,repeat/name)
shutil.copyfile(out/'repeat-receipt.json',repeat/'receipt.json');print(json.dumps(second,indent=2))
