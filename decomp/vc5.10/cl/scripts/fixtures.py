#!/usr/bin/env python3
"""Run all cached CL regressions natively; no original capture or debugger."""
import argparse
import json
from pathlib import Path
import subprocess
import time

PROJECT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--config',type=Path,help='Explicit held trial for completed-case pre-promotion review')
args=parser.parse_args();started=time.perf_counter();reports=[]
for case in ('help','minimal','response-options','usage'):
    command=['python3','scripts/fixture-'+case+'.py']if case in ('help','minimal')else['python3','scripts/fixture-driver.py',case]
    if case!='help'and args.config:command+=['--config',str(args.config)]
    result=subprocess.run(command,cwd=PROJECT,capture_output=True,text=True)
    if result.returncode:
        print(result.stdout,end='');print(result.stderr,end='');raise SystemExit(result.returncode)
    reports.append(json.loads(result.stdout))
report={'passed':True,'cases':reports,'total_seconds':time.perf_counter()-started}
(PROJECT/'build/fixture-times.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
