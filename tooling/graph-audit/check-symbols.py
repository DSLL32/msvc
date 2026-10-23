#!/usr/bin/env python3
"""Synthetic CL-profile logical-to-MAP mapping controls; no source wrappers,
canonical MAP rewrites, target execution, compilers or Wine involved.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',type=Path,default=Path(__file__).parent/'target/release/graph-audit')
p.add_argument('--scratch',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
a=p.parse_args();binary=a.binary.resolve();scratch=a.scratch.resolve()
assert scratch.is_relative_to(Path.home()/'.ai-tmp');scratch.mkdir(parents=True,exist_ok=True)
b=bytearray(0x600)
def word(at,value):struct.pack_into('<I',b,at,value)
b[:2]=b'MZ';word(60,0x80);b[0x80:0x84]=b'PE\0\0';struct.pack_into('<HH',b,0x84,0x14c,1);struct.pack_into('<H',b,0x94,0xe0)
struct.pack_into('<H',b,0x98,0x10b);word(0xb4,0x400000);b[0x178:0x17d]=b'.text'
for at,value in [(8,0x400),(12,0x1000),(16,0x400),(20,0x200),(36,0x60000020)]:word(0x178+at,value)
b[0x200:0x240]=b'\xcc'*0x40;b[0x200:0x206]=b'\xe8\x1b\0\0\0\xc3';b[0x220]=0xc3
original=scratch/'original.exe';rebuilt=scratch/'rebuilt.exe';original.write_bytes(b);rebuilt.write_bytes(b)
publics=[('_main',0x401000),('_main_end',0x401006),('_helper',0x401020),('_helper_alias',0x401020),('_helper_end',0x401021)]
(scratch/'fixture.map').write_text(''.join(' 0001:%08x %s %08x\n'%(address-0x400000,name,address)for name,address in publics))
(scratch/'runtime.json').write_text('[]\n');(scratch/'metadata.json').write_text('{"schema":1,"functions":{},"callback_tables":[]}\n')
cfg=dict(target=dict(path=str(original),sha256=hashlib.sha256(b).hexdigest()),driver=[dict(name='_historical_main',symbol='_main',start=0x401000,end=0x401006),dict(name='_historical_helper',symbol='_helper',start=0x401020,end=0x401021)],accepted=['_historical_main','_historical_helper'])
command=[str(binary),'--profile','cl','--repo',str(scratch),'--project',str(scratch),'--original',str(original),'--rebuilt',str(rebuilt),'--map',str(scratch/'fixture.map'),'--runtime',str(scratch/'runtime.json'),'--rebuilt-runtime',str(scratch/'runtime.json'),'--config',str(scratch/'functions.json'),'--metadata',str(scratch/'metadata.json'),'--check-ret','--check-imports','--check-references','--check-ownership','--check-extents','--check-accepted-extents']
results=[]
def run(label,config,expect=True):
    (scratch/'functions.json').write_text(json.dumps(config));begin=time.perf_counter();p=subprocess.run(command,capture_output=True,text=True);elapsed=time.perf_counter()-begin
    assert(p.returncode==0)==expect,(label,p.stderr)
    report=json.loads(p.stdout)if p.returncode==0 else None
    results.append(dict(case=label,accepted=p.returncode==0,seconds=elapsed,diagnostic=p.stderr.strip()))
    return report
positive=run('historical_logical_names_actual_conventional_MAP',cfg)
assert positive['functions']['_historical_main']['edges']==['_historical_helper']
identity=copy.deepcopy(cfg);identity['driver'][0]['name']='_main';identity['accepted'][0]='_main';run('explicit_identity_mapping_is_noop',identity)
physical=copy.deepcopy(cfg);physical['driver'][1]['symbol']='_helper_alias';run('distinct_MAP_keys_can_share_address',physical)
plain=copy.deepcopy(cfg)
for record in plain['driver']:record['name']=record.pop('symbol')
plain['accepted']=['_main','_helper'];run('no_symbol_default_unchanged',plain)
for label in ['missing_actual','empty_symbol','nonstring_symbol','null_symbol','logical_public_conflict','duplicate_actual','implicit_actual_conflict','duplicate_logical','alias_chain','empty_logical']:
    config=copy.deepcopy(cfg);main,helper=config['driver']
    if label=='missing_actual':main['symbol']='_absent'
    elif label=='empty_symbol':main['symbol']=''
    elif label=='nonstring_symbol':main['symbol']=12
    elif label=='null_symbol':main['symbol']=None
    elif label=='logical_public_conflict':main['name']='_helper'
    elif label=='duplicate_actual':helper['symbol']='_main'
    elif label=='implicit_actual_conflict':main['name']='_main';main.pop('symbol');helper['symbol']='_main'
    elif label=='duplicate_logical':helper['name']=main['name']
    elif label=='alias_chain':helper['symbol']=main['name']
    else:main['name']=''
    run(label,config,False)
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),positive_report=positive,results=results),indent=2)+'\n')
print('Logical symbol CLI controls: 4 positives and 10 rejections passed.')
