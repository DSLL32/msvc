#!/usr/bin/env python3
"""Frozen bare? reference driver, original-only and never a native fixture.
Historical output path must be fresh; this script does not authorize recapture.
"""
from pathlib import Path
import json,os,subprocess,time,hashlib,re,signal
root=Path('/root/msvc2');binary=root/'decomp/watcom10.0a/wlib';planfile=binary/'cases/patched-help/plan.json';plan=json.loads(planfile.read_text());out=Path('/root/.ai-tmp/msvc2/watcom10.0a/wlib/patched-help/run-01');cwd=Path(plan['proposed_launcher_cwd']);helper=Path(plan['guards']['runtime_helper_version']);target=root/plan['target'];original=root/'toolchains/watcom10.0a/binnt/WLIB.EXE';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for path,expected in [(helper,plan['guards']['runtime_helper_sha256']),(target,plan['target_sha256']),(binary/'cases/patched-minus-question/guard.txt',plan['guards']['descriptor_sha256']),(binary/'cases/patched-minus-question/sites.txt',plan['guards']['sites_sha256']),(original,'f20631839a9c1c38969f0c5930e26010361c6175790992181455b5c0ffb1cbb3')]:
 if sha(path)!=expected:raise SystemExit('reviewed identity changed '+str(path))
assert all(ord(c)!=92 or i+1==len(s) or ord(s[i+1])!=92 for s in plan['proposed_explicit_command'] for i,c in enumerate(s))
out.mkdir(parents=True,exist_ok=False);cwd.mkdir();(out/'stdin.bin').write_bytes(b'\n'*64)
env=os.environ.copy();env.update(plan['proposed_environment_overrides']);removed={key:env.pop(key) for key in plan['removed_defaults'] if key in env}
for name,value in [('host-environment.json',env),('removed-host-defaults.json',removed)]:
 p=out/name;p.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n');p.chmod(0o600)
command=plan['proposed_explicit_command'];receipt=dict(status='running reviewed explicitly new patched bare? help case',plan_sha256=sha(planfile),argv=command,cwd=str(cwd),target_sha256_before=sha(target),original_sha256_before=sha(original),helper_sha256=sha(helper),descriptor_sha256=sha(binary/'cases/patched-minus-question/guard.txt'),sites_sha256=sha(binary/'cases/patched-minus-question/sites.txt'),host_environment_sha256=sha(out/'host-environment.json'),target_Windows_environment='not completely observed',stdin=dict(kind='64LF pipe then EOF',size=64,sha256=sha(out/'stdin.bin')),files_before=[],application_functions_accepted=0,masked_replay=False)
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');began=time.monotonic()
with(out/'launcher.stdout.bin').open('wb') as stdout,(out/'launcher.stderr.bin').open('wb') as stderr:
 p=subprocess.Popen(command,cwd=cwd,env=env,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr,start_new_session=True);receipt['launcher_unix_pid']=p.pid
 try:p.wait(timeout=45)
 except subprocess.TimeoutExpired:
  receipt['outer_timeout']=True;os.killpg(p.pid,signal.SIGKILL);p.wait()
receipt.update(launcher_status=p.returncode,elapsed_seconds=time.monotonic()-began,target_sha256_after=sha(target),original_sha256_after=sha(original),files_after=[str(p.relative_to(cwd)) for p in cwd.rglob('*')])
events=(out/'events.txt').read_text(errors='replace') if(out/'events.txt').exists() else '';normal=re.findall(r'^exit status=(\d+)$',events,re.M)
receipt.update(actual_normal_target_exit_status=int(normal[-1]) if normal else None,guard_owner_entries=len(re.findall(r'^guard-owner result=0 ',events,re.M)),guard_reader_entries=len(re.findall(r'^guard-reader result=0 ',events,re.M)),reader_returns=len(re.findall(r'^guard-reader-return result=0 ',events,re.M)),owner_returns=len(re.findall(r'^guard-owner-return result=0 ',events,re.M)),rearms=len(re.findall(r'^guard-step-rearmed ',events,re.M)),actual_main_arguments_hex=re.findall(r'^actual-main-argument address=[^ ]+ bytes=([0-9a-f]+) length=\d+$',events,re.M),actual_resource_open_paths_hex=re.findall(r'^actual-CreateFileA-filename address=[^ ]+ bytes=([0-9a-f]+) length=\d+$',events,re.M),actual_createprocess_command=re.findall(r'^owned-child .* command=(.*)$',events,re.M),guard_unknown_owners=re.findall(r'^guard-unknown-owner caller=([0-9a-f]+)$',events,re.M),exit_cleanup=re.findall(r'^(?:owned-exit-confirmed|failure-owned-child).*$',events,re.M))
receipt['status']='normal patched bare? help completion' if normal and 'guard-final-segments ' in events and not receipt.get('outer_timeout') else 'guard/transport HOLD; no dependent run permitted'
receipt['files']={f.name:dict(size=f.stat().st_size,sha256=sha(f)) for f in out.iterdir() if f.is_file() and f.name not in ['receipt.json','host-environment.json','removed-host-defaults.json']}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,indent=2));print('LAST EVENTS\n'+'\n'.join(events.splitlines()[-18:]))
