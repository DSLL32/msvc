#!/usr/bin/env python3
"""Untimed per-function report extracted from UNCHANGED legacy Python audits."""
import contextlib
import io
import json
from pathlib import Path
import runpy
import sys

project=Path.cwd()
profile=sys.argv[1]
sys.path.insert(0,str(project/'scripts'))
with contextlib.redirect_stdout(io.StringIO()):
    ns=runpy.run_path(str(project/'scripts/audit-graph.py'),run_name='__main__')
    if profile=='rcpp': cfg,original,rebuilt,oi,ri=ns['indexes']()
    elif profile=='rc': original,rebuilt,cfg,oi,ri=ns['compare'].indexes()
    else: original,rebuilt,cfg,oi,ri=ns['indexes']()
    functions=list(cfg['functions'] if profile=='rcpp' else cfg['driver'])
    if profile=='link':
        functions += [r for r in oi if r.get('evidence') and not any(f['name']==r['name'] for f in functions)]
    report={}
    for record in functions:
        if record['name'] not in cfg['accepted']:continue
        if profile=='rc':
            l=ns['describe'](original,record)
            aliases=ns['aliases'](oi)
            refs={a for a in l['references'] if a in aliases and a not in l['calls']}
            report[record['name']]=dict(calls=len(l['calls']),references=len(refs),call_vertices=[dict(address=a,aliases=sorted(aliases[a])) for a in sorted(l['calls'])],reference_vertices=[dict(address=a,aliases=sorted(aliases[a])) for a in sorted(refs)],returns=sorted(ns['return_cleanup'](original,record,oi)),indirect=l['indirect'])
        else:
            edges=ns['edges'](original,oi,record)
            if profile=='rcpp':edges=edges[0]
            report[record['name']]=dict(edges=sorted(edges))
print(json.dumps(dict(checked_functions=len(report),functions=report),sort_keys=True))
