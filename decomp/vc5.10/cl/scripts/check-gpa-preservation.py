#!/usr/bin/env python3
"""Native-only common-resolver and exact-site controls; no function acceptance."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from gpa_preservation import verify

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('receipt',type=Path)
args=parser.parse_args()
receipt=json.loads(args.receipt.read_text())
run=receipt['passes']['qualified_imports']
report=json.loads(Path(run['report']['path']).read_text())
verify(report,PROJECT,ROOT)
passed=[];started=time.perf_counter()
def denied(name,value,project=PROJECT):
    try:verify(value,project,ROOT)
    except (AssertionError,KeyError,ValueError):passed.append(name)
    else:raise AssertionError('accepted GPA mutation: '+name)
scratch=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
with tempfile.TemporaryDirectory(prefix='gpa-controls-',dir=scratch) as work:
    work=Path(work)
    metadata=json.loads((PROJECT/'graph-metadata.json').read_text())
    for label,mutate in (
        ('removed preservation contract',lambda m:m['register_imports'].update(preservation_contracts=[])),
        ('wrong preserved register',lambda m:m['register_imports']['preservation_contracts'][0].update(preserves=['ebx']))):
        altered=copy.deepcopy(metadata);mutate(altered);path=work/(label.replace(' ','-')+'.json');path.write_text(json.dumps(altered))
        command=list(run['command']);command[command.index('--metadata')+1]=str(path)
        result=subprocess.run(command,cwd=PROJECT,capture_output=True,timeout=15)
        if result.returncode:passed.append(label)
        else:denied(label,json.loads(result.stdout))
    value=copy.deepcopy(report);value['functions']['_execute_pass']['register_imports']['original']['resolved_register_calls'].pop(str(0x405660))
    denied('lost second cached site hidden by existing first import',value)
    value=copy.deepcopy(report);value['functions']['_execute_pass']['register_imports']['original']['resolved_register_calls'][str(0x405655)]={'dll':'kernel32.dll','name':'FreeLibrary'}
    denied('wrong resolved cached import identity',value)
    value=copy.deepcopy(report);value['functions']['_execute_pass']['register_imports']['original']['other_unresolved_calls'].append(0x405655)
    denied('extra unknown original site',value)
    value=copy.deepcopy(report);value['functions']['_execute_pass']['register_imports']['rebuilt']['other_unresolved_calls'].append(0x12345678)
    denied('extra unknown rebuilt site',value)
    mirror=work/'project'
    shutil.copytree(PROJECT/'cases/minimal-c/gpa-preservation',mirror/'cases/minimal-c/gpa-preservation')
    for filename in ('cases/minimal-c/getprocaddress-preservation.json','build/cl.exe','build/cl.map','scripts/identify-runtime.py','graph-metadata.json'):
        target=mirror/filename;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(PROJECT/filename,target)
    witness=mirror/'cases/minimal-c/getprocaddress-preservation.json';original=witness.read_text()
    for label,mutate in (
        ('wrong witnessed qualified IAT slot',lambda p:p['original'].update(iat=0x40a1dc)),
        ('wrong witnessed cached callsite',lambda p:p['original'].update(calls=[0x405655,0x405661]))):
        proof=json.loads(original);mutate(proof);witness.write_text(json.dumps(proof));denied(label,copy.deepcopy(report),mirror)
    witness.write_text(original)
    raw=mirror/'cases/minimal-c/gpa-preservation/get-invoke-after-context.bin'
    data=bytearray(raw.read_bytes());data[156]^=1;raw.write_bytes(data)
    denied('actual crossed-call EDI evidence changed',copy.deepcopy(report),mirror)
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'scope':'common resolver plus exact case-bound import-site witness; no acceptance'}))
