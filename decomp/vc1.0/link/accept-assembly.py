#!/usr/bin/env python3
"""Record a matched frontier's complete extent and emit symbolic assembly.

Determine the extent and ABI from disassembly first. This helper does not add
the routine to accepted: compare.py --runtime must assert it again before
unmasking the implementation. Data definitions remain in contextual C files.
"""
import argparse,json,subprocess,sys
from pathlib import Path
from compare import compare
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('address',type=lambda x:int(x,16))
parser.add_argument('end',type=lambda x:int(x,16))
parser.add_argument('name',nargs='?')
parser.add_argument('source',nargs='?')
parser.add_argument('--case',default='/?')
args=parser.parse_args()
frontier=compare(case=args.case)
p=Path('functions.json');cfg=json.loads(p.read_text())
r=next(x for x in cfg['driver'] if x['start']==args.address)
assert r['name'] not in cfg['accepted']
assert frontier==r['name'],'frontier changed: '+frontier
if args.name:r['name']='_'+args.name
r['end']=args.end;r['implementation']='assembly'
if args.source:r['source']=args.source
assert 'source' in r,'specify the contextual C source owner'
p.write_text(json.dumps(cfg,indent=2)+'\n')
subprocess.run([sys.executable,'lift.py'],check=True)
