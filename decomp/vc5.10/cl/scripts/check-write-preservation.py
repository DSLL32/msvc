#!/usr/bin/env python3
"""Check qualified cached _write calls with the shared resolver, without Wine."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import tempfile
from write_preservation import verify,verify_report
PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('receipt',type=Path);args=parser.parse_args()
receipt=json.loads(args.receipt.read_text());run=receipt['passes']['qualified_imports'];report=json.loads(Path(run['report']['path']).read_text());verify_report(report,PROJECT,ROOT);passed=[]
def denied(label,callback):
    try:callback()
    except (AssertionError,KeyError):passed.append(label)
    else:raise AssertionError('accepted altered import proof: '+label)
proof=json.loads((PROJECT/'cases/link-object/write-preservation.json').read_text())
for label,change in [('wrong IAT',lambda p:p.update(iat=0x40a294)),('wrong callsite',lambda p:p.update(calls=[0x405288,0x40529d])),('provider identity',lambda p:p['provider'].update(sha256='0'*64)),('unproved provider bytes',lambda p:p['provider']['ranges'][0].update(sha256='0'*64))]:
    changed=copy.deepcopy(proof);change(changed);denied(label,lambda changed=changed:verify(PROJECT,ROOT,changed))
for label,change in [('missing second call',lambda f:f['original']['resolved_register_calls'].pop(str(0x40529c))),('wrong qualified import',lambda f:f['original']['resolved_register_calls'][str(0x405288)].update(name='_read')),('extra unknown original',lambda f:f['original']['other_unresolved_calls'].append(0x405288)),('extra unknown rebuilt',lambda f:f['rebuilt']['other_unresolved_calls'].append(1))]:
    changed=copy.deepcopy(report);change(changed['functions']['_write_link_message']['register_imports']);denied(label,lambda changed=changed:verify_report(changed,PROJECT,ROOT))
with tempfile.TemporaryDirectory(prefix='write-import-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    for label,change in [('removed write preservation',lambda m:m['register_imports']['preservation_contracts'].pop()),('wrong write preserved register',lambda m:m['register_imports']['preservation_contracts'][1].update(preserves=['edi']))]:
        metadata=json.loads((PROJECT/'graph-metadata.json').read_text());change(metadata);path=Path(temporary)/(label.replace(' ','-')+'.json');path.write_text(json.dumps(metadata));command=list(run['command']);command[command.index('--metadata')+1]=str(path);result=subprocess.run(command,cwd=PROJECT,capture_output=True,timeout=15)
        assert result.returncode,'common resolver accepted unproved crossed-call preservation';passed.append(label)
print(json.dumps({'passed':True,'negative_controls':len(passed),'controls':passed}))
