#!/usr/bin/env python3
"""Permit exactly one advisory label correction and reject unrelated deltas."""
import argparse,copy,json,time
from pathlib import Path
from path_fold_layout import PACKET,derive
from provenance import PROJECT,sha

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();started=time.perf_counter()
 packet=json.loads(PACKET.read_text());old=json.loads((PROJECT/packet['original_field_map']['path']).read_text());new=copy.deepcopy(old);new['command_line']['native_bss_size']=2372;derive(old,new,packet)
 mutations=[('wrong-label',lambda x:x['command_line'].__setitem__('native_bss_size',2373)),
  ('top-level-size',lambda x:x.__setitem__('native_bss_size',2373)),('initializer-size',lambda x:x.__setitem__('allocation_bytes',6549)),
  ('known-default',lambda x:x['modeled_initialized'][0].__setitem__('default',0)),('bss-default',lambda x:x['modeled_bss'][-1]['default'].__setitem__(0,1)),
  ('word-extent',lambda x:x['modeled_bss'][-1].__setitem__('bytes',514)),('word-offset',lambda x:x['modeled_bss'][-1].__setitem__('native_offset',1862)),
  ('word-type',lambda x:x['modeled_bss'][-1].__setitem__('c_type','SHORT[256]')),('word-alias',lambda x:x['modeled_bss'][-1].__setitem__('native_field','c1_bss.checksums')),
  ('original-address',lambda x:x['modeled_bss'][-1].__setitem__('original_rva',0x5592a)),('snapshot-role',lambda x:x['modeled_bss'][-1].__setitem__('snapshot',True)),
  ('unknown-consumer-hold',lambda x:x['value_copy'].__setitem__('hard_frontier_rva',0x12f88)),('callback-order',lambda x:x['callbacks'].reverse()),
  ('new-extra-property',lambda x:x.__setitem__('guessed_state',0)),('original-BSS-extent',lambda x:x['original_bss_region_rva'].__setitem__(1,x['original_bss_region_rva'][1]+4))]
 results=[]
 for name,mutate in mutations:
  current=copy.deepcopy(new);mutate(current)
  try:derive(old,current,packet)
  except AssertionError as error:results.append(dict(control=name,rejection=str(error)))
  else:raise AssertionError('accepted unrelated delta '+name)
 result=dict(status='PASS',kind='validator-only one-label normalization controls; no execution',packet_sha256=sha(PACKET),checker_sha256=sha(Path(__file__).with_name('path_fold_layout.py')),controller_sha256=sha(Path(__file__)),original_field_map_sha256=packet['original_field_map']['sha256'],controls=results,seconds=time.perf_counter()-started)
 assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(results),seconds=result['seconds'])))

if __name__=='__main__':main()
