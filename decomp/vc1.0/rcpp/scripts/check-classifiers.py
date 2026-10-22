#!/usr/bin/env python3
"""Directly call original and reconstructed leaf classifiers with their ABI."""
import hashlib
import json
import subprocess
from pathlib import Path
from compare import wt, publics, PE, SCRATCH

functions = ['_rc_integer_nondecimal_kind@4','_rc_integer_decimal_kind@4',
             '_rc_long_kind@4','_rc_unsigned_kind@4']
original_rvas = [0xcb40,0xcb60,0xcb80,0xcba0]
rec = PE('build/rcpp.exe')
names = publics()
rec_rvas = [names[name]-rec.base for name in functions]
expected = ''.join('classifier %d: 10012 values returned %d\r\n' % (i,result)
                   for i,result in enumerate([7,7,8,9])).encode()
report = dict(probe='direct leaf ABI calls; independent of the trapped whole-program case',
              samples_per_function=10012,convention='stdcall(unsigned long) -> unsigned char',
              boundary_values=['00000000','00000001','00000009','00007fff','00008000','0000ffff',
                               '00010000','7ffffffe','7fffffff','80000000','fffffffe','ffffffff'],
              lcg=dict(seed='13579bdf',multiplier=1664525,increment=1013904223,bits=32),runs={})
for label, executable, rvas in [('original',wt.TC/'bin/RCPP.EXE',original_rvas),
                                ('reconstruction',Path('build/rcpp.exe'),rec_rvas)]:
    output = SCRATCH/('classifiers-'+label+'.stdout')
    with output.open('wb') as stream:
        result = subprocess.run(['wine','build/classifier-check.exe',wt.windows(executable)]+
                                ['%x' % n for n in rvas],env=wt.environment(),
                                stdin=subprocess.DEVNULL,stdout=stream,stderr=subprocess.DEVNULL,
                                timeout=30)
    assert result.returncode == 0, (label,result.returncode,output.read_bytes())
    assert output.read_bytes() == expected, (label,output.read_bytes())
    report['runs'][label] = dict(sha256=hashlib.sha256(Path(executable).read_bytes()).hexdigest(),
                                 functions=[dict(name=n,rva=r)for n,r in zip(functions,rvas)])
Path('classifier-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('Original/reconstruction classifier ABIs match for 40048 direct calls each.')
