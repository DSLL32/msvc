#!/usr/bin/env python3
"""Mutate ONLY a pre-existing scratch snapshot, compare legacy accept/reject.

No worker files or original executables are changed. Restores every mutated
snapshot input after each test; reports preserved legacy blind spots explicitly.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from contextlib import contextmanager

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--snapshot',type=Path,required=True)
parser.add_argument('--binary',type=Path,default=Path(__file__).parent/'target/release/graph-audit')
parser.add_argument('--report',type=Path,required=True)
parser.add_argument('--tools',nargs='+',choices=('cl','c1','c1xx','c2','link','rc','rcpp','cvpack'),default=('cl','c1','c1xx','c2','link','rc','rcpp','cvpack'))
args=parser.parse_args()
root=args.snapshot.resolve();binary=args.binary.resolve();here=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
@contextmanager
def modified(paths):
    saved={p:p.read_bytes() for p in paths}
    try:yield
    finally:
        for p,data in saved.items():p.write_bytes(data)
def run(cmd,cwd):
    p=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True)
    return dict(accepted=p.returncode==0,status=p.returncode,diagnostic=(p.stderr or p.stdout)[-2500:])
report=[]
for tool in args.tools:
    project=root/'decomp/vc1.0'/tool
    config=project/'functions.json';exe=project/('build/CVPACK.EXE' if tool=='cvpack' else f'build/{tool}.exe');mapfile=project/f'build/{tool}.map'
    py=[sys.executable,'scripts/audit-graph.py']
    rs=[str(binary),'--profile',tool,'--repo',str(root),'--project',str(project),'--discovery','legacy']
    cache=project/'graph-identities.json'
    if tool=='cvpack':
        py=[sys.executable,str(here/'cvpack-reference.py'),'--project',str(project),'--cache',str(cache)]
        rs+=['--rebuilt-runtime',str(cache)]
    p=subprocess.run([sys.executable,str(here/'mutation-info.py'),tool],cwd=project,capture_output=True,text=True,check=True)
    info=json.loads(p.stdout)
    cfg=json.loads(config.read_text())
    def compare(label,expected=False):
        left=run(py,project);right=run(rs,project)
        assert left['accepted']==right['accepted'],(tool,label,left,right)
        assert expected is None or left['accepted']==expected,(tool,label,left,right)
        report.append(dict(tool=tool,mutation=label,python=left,rust=right,binary_sha256=sha(binary),captured_utc=__import__('datetime').datetime.now(__import__('datetime').timezone.utc).isoformat()))
        print(tool,label,'accept'if left['accepted'] else'reject',flush=True)
    def rebind_cache():
        if tool=='cvpack':
            c=json.loads(cache.read_text());c['provenance']['rebuilt_sha256']=sha(exe);c['provenance']['functions_sha256']=sha(config);cache.write_text(json.dumps(c))
    paths=[config,exe,mapfile]+([cache]if tool=='cvpack' else[])
    assert info['call'],tool
    with modified(paths):
        b=bytearray(exe.read_bytes());call=info['call'];struct.pack_into('<I',b,call['offset']+1,(0xdeadbeef-call['address']-5)&0xffffffff);exe.write_bytes(b);rebind_cache();compare('wrong-direct-callee')
    with modified(paths):
        b=bytearray(exe.read_bytes())
        for call in info['call']['all_calls']:b[call['offset']:call['offset']+5]=b'\x90'*5
        exe.write_bytes(b);rebind_cache();compare('missing-direct-edges')
    if tool!='cvpack':
        with modified(paths):
            target=info['first']['name'];f=next(f for f in cfg['functions'if tool=='rcpp'else'driver']if f['name']==target);symbol=f.get('symbol',target)
            text=mapfile.read_text();text,count=re.subn(r'(?m)^(\s+[0-9a-f]{4}:[0-9a-f]{8}\s+)'+re.escape(symbol)+r'(\s+[0-9a-f]{8}.*)$',r'\1__graph_audit_missing_public\2',text);assert count,(tool,symbol);mapfile.write_text(text);compare('missing-public-mapping')
    if info['truncated']:
        with modified(paths):
            c=json.loads(config.read_text());key='functions'if tool in('rcpp','cvpack')else'driver';r=next(r for r in c[key]if r['name']==info['truncated']['name'])
            if tool=='cvpack':r['extent'][1]=info['truncated']['end']
            else:r['end']=info['truncated']['end']
            config.write_text(json.dumps(c));rebind_cache();compare('split-ret-operand',None if tool=='cl'else False)
    if tool=='c1xx':
        with modified(paths):
            c=json.loads(config.read_text());value=c['runtime_data'][0];value['value']+=1<<(8*value['bytes']);config.write_text(json.dumps(c));compare('oversized-crt-literal')
    if tool=='cvpack':
        with modified(paths):
            c=json.loads(config.read_text());nonstub=[r for r in c['functions']if r['status']!='stub'];nonstub[0]['extent'][1]=nonstub[1]['original']+1;config.write_text(json.dumps(c));rebind_cache();compare('overlapping-application-ownership')
        with modified(paths):
            c=json.loads(config.read_text());next(r for r in c['functions']if r['status']!='stub')['stack_cleanup']=4;config.write_text(json.dumps(c));rebind_cache();compare('wrong-declared-ret-cleanup')
        with modified(paths):
            c=json.loads(cache.read_text());c['provenance']['rebuilt_sha256']='0'*64;cache.write_text(json.dumps(c));compare('stale-runtime-identity-cache')
args.report.write_text(json.dumps(report,indent=2)+'\n')
