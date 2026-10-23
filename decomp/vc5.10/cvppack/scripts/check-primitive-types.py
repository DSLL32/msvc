#!/usr/bin/env python3
"""Exhaust the primitive encodings against the original's semantic type table."""
import json
from pathlib import Path
import struct
import subprocess
import time
from state import BINARY, ROOT, sha
from binary import PE
start=time.perf_counter();work=BINARY/'build/primitive-types';work.mkdir(parents=True,exist_ok=True)
source=work/'driver.c';source.write_text('''#include <stdio.h>
extern unsigned map_primitive_type(unsigned);
int main(void) {
    unsigned input, output;
    while (fread(&input, sizeof(input), 1, stdin) == 1) {
        output = map_primitive_type(input);
        if (fwrite(&output, sizeof(output), 1, stdout) != 1) return 1;
    }
    return 0;
}
''')
command=['clang','-m32','-O0','-g','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST',
         '-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),
         str(BINARY/'src/primitive-types.c'),str(source),'-o',str(work/'check')]
subprocess.run(command,check=True);build=time.perf_counter()-start
inputs=list(range(512))+[0x10000,0x10080,0x12345678,0xffffff80,0xffffffff]
begin=time.perf_counter();run=subprocess.run([str(work/'check')],input=struct.pack('<%dI'%len(inputs),*inputs),stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
execution=time.perf_counter()-begin;assert not run.stderr
pe=PE(ROOT/'toolchains/5.10/bin/CVPACK.EXE');table=struct.unpack('<128H',pe.read(0x412018,256));basic=struct.unpack('<3H',pe.read(0x412010,6))
expected=[table[x&127] if x&128 else basic[x] if x<3 else 7 for x in inputs]
assert struct.unpack('<%dI'%len(inputs),run.stdout)==tuple(expected)
report=dict(schema=1,native_only=True,original_sha256=sha(ROOT/'toolchains/5.10/bin/CVPACK.EXE'),
    table_extent=[0x412018,0x412118],basic_extent=[0x412010,0x412016],checked=len(inputs),
    source_sha256=sha(BINARY/'src/primitive-types.c'),seconds=dict(build=build,execution=execution,total=time.perf_counter()-start))
(BINARY/'cases/hello-debug-dll/primitive-type-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
