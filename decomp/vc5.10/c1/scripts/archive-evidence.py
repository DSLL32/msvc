#!/usr/bin/env python3
"""Preserve small raw evidence and external identities without copying target code."""
import argparse,json,shutil
from pathlib import Path
from provenance import PROJECT,sha,captured_native_binary

def main():
 p=argparse.ArgumentParser();p.add_argument('destination',type=Path);p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();a.destination.mkdir(parents=True,exist_ok=False);external={}
 for side,source in [('original',a.original),('native',a.native)]:
  destination=a.destination/side;destination.mkdir()
  for f in source.iterdir():
   if not f.is_file():continue
   external_raw=f.name.startswith(('state-','fpu-')) or (f.stat().st_size>32768 and f.name not in ('receipt.json','state.json','events.txt','source-snapshot.json'))
   if external_raw or '-CRT-' in f.name or '-Invoke-argv' in f.name or f.name.startswith('grouped-') or f.name.startswith('path-fold-')and f.suffix=='.bin' or f.name.startswith(('compiler-data-','compiler-bss-','compiler-snapshot-','include-alias-committed-')) or f.suffix.lower() in ('.exe','.dll') or f.name.startswith('loaded-') or f.name in ('relocated-text.bin','loaded-c1.bin','invoke-environment.bin','restore-original-input.bin','original-crt-argv.bin','original-crt-environment.bin','native-crt-argv.bin','native-crt-environment.bin'):
    external[side+'/'+f.name]={'path':str(f),'sha256':sha(f),'size':f.stat().st_size}
   else:shutil.copy2(f,destination/f.name)
 native_elf=captured_native_binary(a.native)
 external['native/captured-elf']={'path':str(native_elf),'sha256':sha(native_elf),'size':native_elf.stat().st_size}
 for name in ('trial-audit.json','trial-audit-binding.json','trial-validation.json'):
  if (PROJECT/'build'/name).exists():shutil.copy2(PROJECT/'build'/name,a.destination/name)
 (a.destination/'external-analysis-artifacts.json').write_text(json.dumps(external,indent=2)+'\n')
if __name__=='__main__':main()
