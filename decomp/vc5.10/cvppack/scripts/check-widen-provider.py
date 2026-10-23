#!/usr/bin/env python3
"""Native-only typed-provider success, tail and semantic rejection controls."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time
from state import BINARY,ROOT,sha

spec=importlib.util.spec_from_file_location('native_widen',BINARY/'scripts/native-widen.py')
encoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(encoder)
begin=time.perf_counter()
common=['-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-fms-extensions','-O0','-g']
cpp=['clang++',*common,'-std=gnu++98','-nostdinc++','-fno-rtti','-fno-exceptions','-c',str(BINARY/'scripts/native-widen-test.cpp'),'-o',str(BINARY/'build/native-widen-test.o')]
result=subprocess.run(cpp,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
assert result.returncode==0,result.stdout.decode('latin1')
command=['clang',*common,'-std=gnu89','-fasm-blocks',str(BINARY/'build/native-widen-test.o'),str(BINARY/'scripts/native-widen.c'),
 *[str(p) for p in sorted((ROOT/'lib/recon/src').glob('*.c'))],'-o',str(BINARY/'build/widen-provider-test')]
result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
(BINARY/'build/widen-provider-build.log').write_bytes(result.stdout)
assert result.returncode==0,result.stdout.decode('latin1')
build_seconds=time.perf_counter()-begin
contract=json.loads((BINARY/'cases/hello-debug-dll/widen-contract.json').read_text())
with tempfile.TemporaryDirectory(dir=Path.home()/'.ai-tmp') as temporary:
    work=Path(temporary)
    receipt=encoder.prepare(work/'responses.rwp')
    requests=bytearray(b'RWQ1'+struct.pack('<I',21))
    for row in contract['instances']:
        q,e=row['query'],row['emit']
        requests+=struct.pack('<5I',q['input_bytes'],q['output_capacity'],q['fixup_capacity'],e['written_bytes'],e['written_fixups'])
        requests+=bytes.fromhex(q['input'])+bytes.fromhex(e['output'])+bytes.fromhex(e['fixups'])
    (work/'requests').write_bytes(requests)
    input_source=BINARY/'cases/hello-debug-dll/original-probe-0001/input.dll'
    env={k:v for k,v in os.environ.items() if not k.startswith(('WINE','RECON_'))};env.update(PATH='',LC_ALL='C',TZ='UTC')
    checks=[];execution_start=time.perf_counter()
    modes=['positive','bad-last-error','bad-factory-capacity','bad-factory-flags','unknown-slot','unreached-record-slot','destroy-before-emit',
      'emit-before-query','bad-query-input','bad-query-description','wrong-object','bad-query-flags','query-twice',
      'bad-emit-input','bad-emit-capacity','bad-output-alias','bad-emit-size','bad-emit-flags','duplicate-destroy','incomplete-sequence','extra-factory',
      'wrong-complete-DLL-input','wrong-provider-module','wrong-provider-header']
    baseline=(work/'responses.rwp').read_bytes()
    for mode in modes:
        shutil.copyfile(input_source,work/'hello.dll');(work/'responses.rwp').write_bytes(baseline)
        actual=mode
        if mode=='wrong-complete-DLL-input':
            raw=bytearray((work/'hello.dll').read_bytes());raw[200]^=1;(work/'hello.dll').write_bytes(raw);actual='positive'
        if mode in ('wrong-provider-module','wrong-provider-header'):
            raw=bytearray(baseline);raw[12 if mode=='wrong-provider-module' else 0]^=1;(work/'responses.rwp').write_bytes(raw);actual='positive'
        result=subprocess.run([str(BINARY/'build/widen-provider-test'),str(work/'responses.rwp'),str(work/'requests'),actual],cwd=work,env=env,
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
        assert result.returncode==(0 if mode=='positive' else 125),(mode,result.returncode,result.stdout,result.stderr)
        checks.append(dict(name=mode,status=result.returncode))
    seconds=time.perf_counter()-execution_start
report=dict(schema=1,native_only=True,provider=receipt,positive_lifetimes=21,positive_tail_pattern='0xa5 through full maximum allocations',checks=checks,
            seconds=dict(build=build_seconds,execution_checks=seconds,total=time.perf_counter()-begin))
(BINARY/'cases/hello-debug-dll/provider-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
