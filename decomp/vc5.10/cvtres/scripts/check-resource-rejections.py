#!/usr/bin/env python3
"""Native-only negative guards over actual typed resource debugger snapshots."""
import argparse,copy,json,struct
from pathlib import Path
from resource_state import original,canonical
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
old=original((a.original/'events.txt').read_text());state=json.loads((a.native/'state.json').read_text());actual=state['globals']['__heap__'];expected=canonical(old);assert canonical(actual)==expected

def change_node(record,kind,offset,value=None):
 node=next(n for n in record['nodes'] if n['kind']==kind and (kind!=1 or int.from_bytes(bytes.fromhex(n['raw_hex'])[:4],'little')==1))
 raw=bytearray.fromhex(node['raw_hex'])
 if value is None:raw[offset]^=1
 else:struct.pack_into('<I',raw,offset,value)
 node['raw_hex']=raw.hex()

def directory(record):record['directory_hex']='01'+record['directory_hex'][2:]
def alias(record):record['roots']['resource_files']=next(n['pointer'] for n in record['nodes'] if n['kind']==3 and n['pointer']!=record['roots']['resource_files'])
tests=[('header-field',lambda r:change_node(r,4,0)),('resource-offset',lambda r:change_node(r,3,8)),('named-UTF16',lambda r:change_node(r,1,16)),('wrong-root-alias',alias),('missing-node',lambda r:r['nodes'].pop()),('directory-byte',directory)]
results=[]
for name,edit in tests:
 changed=copy.deepcopy(actual);edit(changed)
 try:assert canonical(changed)==expected,'typed resource heap/alias/directory state differs'
 except AssertionError as error:results.append(dict(name=name,rejected=True,reason=str(error)))
 else:raise AssertionError(name+' accepted')
a.output.write_text(json.dumps(dict(scope='Actual immutable resource frontier; no synthetic execution or original rerun',original=str(a.original),native=str(a.native),cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
