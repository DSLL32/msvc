#!/usr/bin/env python3
"""Independently exercise every 16-bit message code and boundary identities."""
import hashlib,json,subprocess
from pathlib import Path
from binary import PE
from compare import wt,HERE,SCRATCH,publics
cfg=json.loads(Path('functions.json').read_text())
original=PE(wt.ROOT/cfg['target']['path'])
reconstructed=PE('build/rcpp.exe')
entry=next(f['start'] for f in cfg['functions'] if f['name']=='_rc_message@4')
report={}
outputs=[]
for label,binary,rva in [('original',wt.ROOT/cfg['target']['path'],entry-original.base),
                         ('reconstructed',HERE/'build/rcpp.exe',publics()['_rc_message@4']-reconstructed.base)]:
    output=SCRATCH/('message-check-'+label+'.bin')
    subprocess.run(['wine','build/message-check.exe',wt.windows(binary),'%x'%rva,wt.windows(output)],
                   env=wt.environment(),check=True,timeout=90,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    data=output.read_bytes();outputs.append(data)
    report[label]=dict(binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),rva=rva,
                       calls=131080,distinct_code_arguments=65538,
                       result_sha256=hashlib.sha256(data).hexdigest())
assert outputs[0]==outputs[1]
Path('message-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('Message lookup ABI/text/identity match for all 65536 16-bit codes and two boundary values.')
