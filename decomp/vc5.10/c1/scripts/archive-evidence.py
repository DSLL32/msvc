#!/usr/bin/env python3
"""Preserve small raw evidence and external identities without copying target code."""
import argparse,json,shutil
from pathlib import Path
from provenance import PROJECT,sha

def main():
 p=argparse.ArgumentParser();p.add_argument('destination',type=Path);p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();a.destination.mkdir(parents=True,exist_ok=False);external={}
 for side,source in [('original',a.original),('native',a.native)]:
  destination=a.destination/side;destination.mkdir()
  for f in source.iterdir():
   if not f.is_file():continue
   if f.name.startswith(('compiler-data-','compiler-bss-','compiler-snapshot-')) or f.suffix.lower() in ('.exe','.dll') or f.name in ('loaded-target.bin','loaded-msvcrt.bin','relocated-text.bin','loaded-c1.bin','invoke-environment.bin','restore-original-input.bin'):
    external[side+'/'+f.name]={'path':str(f),'sha256':sha(f),'size':f.stat().st_size}
   else:shutil.copy2(f,destination/f.name)
 for name in ('trial-audit.json','trial-audit-binding.json','trial-validation.json'):
  if (PROJECT/'build'/name).exists():shutil.copy2(PROJECT/'build'/name,a.destination/name)
 (a.destination/'external-analysis-artifacts.json').write_text(json.dumps(external,indent=2)+'\n')
if __name__=='__main__':main()
