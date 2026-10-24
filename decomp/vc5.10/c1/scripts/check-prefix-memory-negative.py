#!/usr/bin/env python3
"""Boundary identity, single-read and genuinely later sentinel denials."""
import argparse,hashlib,json,tempfile,time
from pathlib import Path
from provenance import captured_native_binary,sha,PROJECT
from allocation_state import symbols
from prefix_memory_state import validate_pool,validate_alias
from importlib.util import spec_from_file_location,module_from_spec
spec=spec_from_file_location('normalizer_controls',Path(__file__).with_name('check-normalizer-negative.py'));mod=module_from_spec(spec);spec.loader.exec_module(mod);Overlay=mod.Overlay

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.perf_counter();names=symbols(captured_native_binary(a.native))
 pool=json.loads((a.native/'command-line-entry-observations.json').read_text())[0];alias=json.loads((a.native/'alias-initializer-entries.json').read_text())[0];base=pool['compiler_state']['bss']['arenas'][0]['base'];root=alias['compiler_state']['bss']['arenas'][0]['cursor'];controls=[]
 def validate(n):
  validate_pool(n,names,base,pool['caller']['pc'],58)
  validate_alias(n,names,base,root,json.loads((a.native/'checksum-entries.json').read_text())[0]['caller']['pc'])
 validate(a.native)
 def reject(label,files):
  try:validate(Overlay(a.native,files))
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(label)
 for file in ('pool-final-memory-boundary.json','include-alias-memory-boundary.json'):
  for key,value in [('boundary','push_packing'),('pc',names['push_packing'][0]),('caller','initialize_packing'),('return',0),('owner','c1_bss.arenas[1]'),('base',0),('bytes',65535),('reads',2),('sha256','0'*64)]:
   data=json.loads((a.native/file).read_text());data[key]=value
   with tempfile.TemporaryDirectory(prefix='prefix-memory-control-',dir=a.native.parent)as tmp:
    path=Path(tmp)/file;path.write_text(json.dumps(data));reject(file+':'+key,{file:path})
 # The final object is genuinely nonzero. Replacing either earlier snapshot
 # with those later bytes fails even when its content hash is reauthenticated.
 late=json.loads((a.native/'packing-frame-ready.json').read_text());object_address=late['compiler_state']['option_state']['initialized']['packing_stack'];sentinel=bytes(late['object']);assert len(sentinel)==8 and any(sentinel)
 for rawname,metaname in [('pool-final-committed-000.bin','pool-final-memory-boundary.json'),('include-alias-committed-000.bin','include-alias-memory-boundary.json')]:
  raw=bytearray((a.native/rawname).read_bytes());offset=object_address-base;assert 0<=offset<=len(raw)-len(sentinel);raw[offset:offset+len(sentinel)]=sentinel
  meta=json.loads((a.native/metaname).read_text());meta['sha256']=hashlib.sha256(raw).hexdigest()
  with tempfile.TemporaryDirectory(prefix='prefix-memory-control-',dir=a.native.parent)as tmp:
   r=Path(tmp)/rawname;m=Path(tmp)/metaname;r.write_bytes(raw);m.write_text(json.dumps(meta));reject('late-object-rewrite:'+rawname,{rawname:r,metaname:m})
 result=dict(status='all phase/owner/single-read/late-sentinel controls rejected',count=len(controls),seconds=time.perf_counter()-start,controls=controls,native_receipt_sha256=sha(a.native/'receipt.json'),later_nonzero_object_bytes=len(sentinel),unchanged_earlier_bytes=65536,validators={str(f.relative_to(PROJECT)):sha(f)for f in [Path(__file__),PROJECT/'scripts/prefix_memory_state.py']},observer_sources={str(f.relative_to(PROJECT)):sha(f)for f in [PROJECT/'scripts/pool_cycle_probe.py',PROJECT/'scripts/alias_init_probe.py',PROJECT/'scripts/capture-native.py']});a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))
if __name__=='__main__':main()
