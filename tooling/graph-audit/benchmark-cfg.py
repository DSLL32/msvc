#!/usr/bin/env python3
"""Benchmark consolidated discovery independently of legacy parity mode."""
import argparse
import json
import os
from pathlib import Path
import statistics
import subprocess
import time
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--snapshot',type=Path,required=True)
parser.add_argument('--binary',type=Path,default=Path(__file__).parent/'target/release/graph-audit')
parser.add_argument('--report',type=Path,required=True)
parser.add_argument('--repetitions',type=int,default=7)
parser.add_argument('--discovery',choices=('cfg','legacy'),default='cfg')
a=parser.parse_args();root=a.snapshot.resolve();binary=a.binary.resolve();report={}
started=__import__('datetime').datetime.now(__import__('datetime').timezone.utc).isoformat()
for tool in ('cl','c1','c1xx','c2','link','rc','rcpp','cvpack'):
    project=root/'decomp/vc1.0'/tool
    command=[str(binary),'--profile',tool,'--repo',str(root),'--project',str(project),'--discovery',a.discovery]
    if tool=='cvpack':command+=['--rebuilt-runtime',str(project/'graph-identities.json')]
    values=[];result=None
    for n in range(a.repetitions+1):
        start=time.perf_counter();p=subprocess.run(command,capture_output=True,text=True,cwd=project);elapsed=time.perf_counter()-start
        assert p.returncode==0,(tool,p.stderr)
        current=json.loads(p.stdout)
        if result is not None:assert result==current
        result=current
        if n:values.append(elapsed)
    report[tool]=dict(median_seconds=statistics.median(values),min_seconds=min(values),max_seconds=max(values),samples_seconds=values,checked_functions=result['checked_functions'],canonical_report_sha256=__import__('hashlib').sha256(json.dumps(result,sort_keys=True).encode()).hexdigest())
    print(tool,f"{report[tool]['median_seconds']:.6f}s",flush=True)
a.report.write_text(json.dumps(dict(started_utc=started,finished_utc=__import__('datetime').datetime.now(__import__('datetime').timezone.utc).isoformat(),binary_sha256=__import__('hashlib').sha256(binary.read_bytes()).hexdigest(),discovery=a.discovery,load_average=os.getloadavg(),results=report),indent=2)+'\n')
