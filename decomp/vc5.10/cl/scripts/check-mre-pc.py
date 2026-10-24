#!/usr/bin/env python3
"""Focused ELF/PC authentication controls; old execution bytes are restored."""
import argparse,json,time
from pathlib import Path
from mre_state import verify
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--receipt',type=Path,required=True)
a=p.parse_args();passed=[];started=time.perf_counter()
verify(a.original,a.native)
def negative(name,filename,change):
 path=a.native/filename;raw=path.read_bytes()
 try:
  record=json.loads(raw);change(record);path.write_text(json.dumps(record,indent=2)+'\n')
  try:verify(a.original,a.native)
  except(AssertionError,KeyError,ValueError):passed.append(name)
  else:raise AssertionError('accepted '+name)
 finally:path.write_bytes(raw)
for i in(1,2,3):
 negative('actual wrapper entry '+str(i),'native-mre-%d-before-gdb.json'%i,lambda r:r['general'].__setitem__('eip',r['general']['eip']+1))
 negative('actual caller return '+str(i),'native-mre-%d-before-gdb.json'%i,lambda r:r['arguments'].__setitem__('return',r['arguments']['return']+1))
negative('frontier authentic INT3','native-mre-frontier-gdb.json',lambda r:r['general'].__setitem__('eip',r['general']['eip']+1))
negative('frontier exact cdecl slot','native-mre-frontier-gdb.json',lambda r:r['general'].__setitem__('ebp',r['general']['ebp']+4))
negative('data-derived load bias','native-mre-1-before-gdb.json',lambda r:r['data_addresses'].__setitem__('option_buffer',r['data_addresses']['option_buffer']+4096))
negative('after data symbol ownership','native-mre-2-after-gdb.json',lambda r:r['symbols'].__setitem__('driver',r['symbols']['driver']+4))
negative('ELF execution hash','native-prefix.json',lambda r:r.__setitem__('native_sha256','0'*64))
verify(a.original,a.native)
report={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-started,
 'scope':'Monotonic PC/ELF audit only; previous57 state/ABI/effect controls reused by exact source/raw hashes. Initial frontier saved return bytes remain explicitly unobserved.'}
a.receipt.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
