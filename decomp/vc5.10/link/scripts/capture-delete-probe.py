#!/usr/bin/env python3
"""Explicit original-only delete probe; fixtures never call this script."""
import hashlib,importlib,json,struct,subprocess,tempfile,time,shutil
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
transport=importlib.import_module('wine-tool')
sha=lambda data:hashlib.sha256(data).hexdigest()
subprocess.run(['make','-q','build/delete-probe.exe'],cwd=BINARY,check=True)
probe=BINARY/'build/delete-probe.exe';helper=BINARY/'build/capture-help.exe'
scratch=Path(tempfile.mkdtemp(prefix='delete-probe-',dir=transport.SCRATCH/'vc5.10/link'))
raw=scratch/'raw';raw.mkdir();work=scratch/'work';work.mkdir()
(raw/'state-plan.txt').write_text('');(raw/'entries.txt').write_text('')
canonical_path=BINARY.parents[2]/'toolchains/5.10/bin/MSVCRT.DLL'
assert sha(canonical_path.read_bytes())=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
shutil.copyfile(probe,work/'delete-probe.exe');shutil.copyfile(canonical_path,work/'MSVCRT.DLL')
launched_probe=work/'delete-probe.exe'
command=['wine',transport.windows(helper),transport.windows(launched_probe),transport.windows(raw),'-',transport.windows(raw/'entries.txt'),transport.windows(work)]
start=time.perf_counter();result=subprocess.run(command,cwd=work,env=transport.environment(),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30);elapsed=time.perf_counter()-start
(raw/'helper.stdout').write_bytes(result.stdout);(raw/'helper.stderr').write_bytes(result.stderr)
files={p.name:dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))for p in raw.iterdir()if p.is_file()}
canonical='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
provider=[n for n,v in files.items()if n.startswith('module-')and v['sha256']==canonical]
receipt=dict(kind='explicit-original-only-owned-new-delete-matrix',command=command,status=result.returncode,elapsed_seconds=elapsed,target_sha256=sha(probe.read_bytes()),actual_target_hfile_verified=files.get('module-000.bin',{}).get('sha256')==sha(probe.read_bytes()),actual_loaded_canonical_provider=provider,files=files)
(raw/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(scratch,flush=True)
assert result.returncode==0 and receipt['actual_target_hfile_verified'] and provider,'probe/provider capture failed; retain raw'
events=[json.loads(line)for line in (raw/'events.jsonl').read_text().splitlines()];assert events[-1]==dict(event='summary',failed=0,status=0)
artifact=(work/'delete-probe.bin').read_bytes();cursor=8;assert struct.unpack_from('<2I',artifact)==(0x31504452,1)
records=[];sizes=[12288,4000,4096]
for scenario in range(4):
 for target in [2,1,0]:
  values=struct.unpack_from('<9I',artifact,cursor);cursor+=36
  eno=111 if scenario&1 else 0;last=0x12345678 if scenario&2 else 0
  assert values==(scenario,target,sizes[target],eno,last,eno,last,(1<<target)-1,target),values
  live=[]
  for slot in range(target):
   assert struct.unpack_from('<2I',artifact,cursor)==(slot,sizes[slot]);cursor+=8
   data=artifact[cursor:cursor+sizes[slot]];cursor+=sizes[slot]
   assert data==bytes((index*13+slot*17+scenario)&255 for index in range(sizes[slot]))
   live.append(dict(slot=slot,bytes=len(data),sha256=sha(data)))
  records.append(dict(scenario=scenario,deleted_slot=target,deleted_bytes=sizes[target],errno_before=values[3],last_error_before=values[4],errno_after=values[5],last_error_after=values[6],surviving_allocations=live))
assert cursor==len(artifact)
report=dict(status='pass',receipt_sha256=sha((raw/'receipt.json').read_bytes()),artifact_sha256=sha(artifact),artifact_bytes=len(artifact),records=records,elapsed_seconds=elapsed)
(scratch/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status='pass',calls=len(records),artifact_bytes=len(artifact),elapsed_seconds=elapsed)))
