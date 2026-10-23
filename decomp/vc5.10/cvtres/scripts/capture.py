#!/usr/bin/env python3
"""Explicit original-only /help capture. Not a fixture dependency."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import time
from mask import BINARY,ROOT,TARGET,build,sha
from wine_tool_loader import tool
from inputs import fingerprints
from crt_layout import verify as verify_crt

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',required=True,type=Path)
p.add_argument('--initial',action='store_true')
p.add_argument('--unmasked',action='store_true')
p.add_argument('--candidate',action='append',default=[])
a=p.parse_args();assert not(a.initial and a.unmasked)
start=time.perf_counter(); out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
for name,path in [('original-build.json',BINARY/'build/original-inputs.json'),('capture-host-build.json',BINARY/'build/capture-host-inputs.json')]:
 if path.exists():shutil.copyfile(path,out/name)
(out/'configuration.json').write_bytes((BINARY/'functions.json').read_bytes())
image=out/'CVTRES.EXE';manifest=build(image,a.initial,a.candidate,a.unmasked)
provider=ROOT/'toolchains/5.10/bin/MSVCRT.DLL'
expected='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
assert provider.stat().st_size==277776 and sha(provider.read_bytes())==expected
shutil.copyfile(provider,out/'MSVCRT.DLL')
(out/'restore-plan').write_text(''.join('%x %x %x\n'%(r['start'],r['end'],r['offset']) for r in manifest['entry_ranges']))
(out/'mask.json').write_text(json.dumps(manifest,indent=2)+'\n')
helper=BINARY/'build/capture-host.exe';assert helper.is_file()
command=['wine',str(helper),tool.windows(image),tool.windows(out/'stdout.bin'),
         tool.windows(out/'stderr.bin'),tool.windows(out/'events.txt'),tool.windows(out),
         tool.windows(TARGET),tool.windows(out/'restore-plan')]
env=tool.environment()
config=json.loads((out/'configuration.json').read_text())
verify_crt(config.get('crt_data',[]))
env['CVTRES_CAPTURE_CRT_STATE']=','.join('%x:%d:%s'%(item['rva'],item['size'],{'bytes':'b','cstring':'p','file':'f'}[item['kind']]) for item in config.get('crt_data',[]))
env['CVTRES_OBSERVATIONS']=','.join('%x:%x'%(item['address'],load_offset) for item in config.get('observations',[]) for load_offset in [__import__('binary').PE(TARGET).offset(item['address'])])
env['CVTRES_CAPTURE_STATE']=','.join('%x:%d'%(item['original'],item['size']) for item in config.get('data',[]))
setup=time.perf_counter()-start;t=time.perf_counter()
try:
 r=subprocess.run(command,cwd=BINARY,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)
 status=r.returncode;hostout=r.stdout;hosterr=r.stderr;timed_out=False
except subprocess.TimeoutExpired as e:
 status=None;hostout=e.stdout or b'';hosterr=e.stderr or b'';timed_out=True
(out/'host.stdout').write_bytes(hostout);(out/'host.stderr').write_bytes(hosterr)
execution=time.perf_counter()-t
assert sha((BINARY/'functions.json').read_bytes())==manifest['configuration_sha256'],'config changed during capture'
actual=out/'loaded-msvcrt.dll';actualhash=sha(actual.read_bytes()) if actual.exists() else None
receipt=dict(schema=1,purpose='fresh original /help evidence',case='help',argv=['/help'],
 original_sha256=sha(TARGET.read_bytes()),masked_sha256=sha(image.read_bytes()),
 configuration_sha256=manifest['configuration_sha256'],candidates=a.candidate,initial=a.initial,unmasked=a.unmasked,rebuilt_sha256=sha((BINARY/'build/CVTRES.EXE').read_bytes()),map_sha256=sha((BINARY/'build/cvtres.map').read_bytes()),helper_sha256=sha(helper.read_bytes()),
 native_sha256_at_capture=sha((BINARY/'build/native/CVTRES').read_bytes()),source_inputs_at_capture=fingerprints(),
 command=command,helper_status=status,timed_out=timed_out,provider_expected=expected,raw_sha256={name:sha((out/name).read_bytes()) for name in ['events.txt','stdout.bin','stderr.bin'] if (out/name).is_file()},
 provider_actual_hfile_sha256=actualhash,provider_verified=actualhash==expected,loaded_image_hfile_sha256=sha((out/'loaded-image.exe').read_bytes()) if (out/'loaded-image.exe').exists() else None,
 seconds=dict(setup=setup,execution=execution,total=time.perf_counter()-start))
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,indent=2))
assert not timed_out and status==0,'original helper failure; no evidence acceptance'
assert actualhash==expected,'canonical provider was not verified from actual LOAD_DLL hFile'
