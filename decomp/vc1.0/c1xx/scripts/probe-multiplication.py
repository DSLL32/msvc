"""Bounded original/native multiplication returns and native stale-path denial.

Explicit reconstruction evidence only. One original process observes the real
lifetime invocation, then reuses its call stack for the listed diagnostic inputs.
"""
import fcntl
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import resource
import shutil
import signal
import subprocess
import tempfile
import time
from binary import PE
from case_paths import BINARY, SCRATCH, CAPTURE_ROOT, paths, canonical_parts, windows
from win32_indexes import imports, mask
from shared_build_inputs import snapshot, materialize

VECTORS=[(0xff,2,1,1),(0xffffff80,1,0,1),(0xffffff80,0xffffffff,0,1),
         (0xffff8000,1,0,2),(0xffff8000,0xffffffff,0,2),
         (0x80000000,1,0,4),(0x80000000,0xffffffff,0,4),
         (0x7fffffff,2,0,4),(0xffffffff,2,1,4)]
EXPECTED_RESULTS=[0,1,0,1,0,1,0,1,1,1]


def consumed_rows(raw):
    rows=[]
    for index,a,b,u,w,result in re.findall(r'^result (\d+) ([0-9a-f]+) ([0-9a-f]+) (\d+) (\d+) (\d+)$',raw,re.M):
        assert int(index)==len(rows),'missing/reordered observer row'
        rows.append((int(a,16),int(b,16),int(u),int(w),int(result)))
    return rows


def validate(rows):
    arguments=[(0,4,0,4)]+VECTORS
    assert len(rows)==len(arguments),'missing original observer evidence'
    assert [r[:4] for r in rows]==arguments,'consumed formal arguments differ'
    assert [r[4] for r in rows]==EXPECTED_RESULTS,'original arithmetic results differ'


def main():
    started=time.perf_counter();config_data=(BINARY/'functions.json').read_bytes();config=json.loads(config_data)
    assert config['cases']['stack_lifetime']['terminal']=='_multiplication_overflows@16'
    directory=Path(tempfile.mkdtemp(prefix='multiply-boundary-',dir=CAPTURE_ROOT))
    spec=importlib.util.spec_from_file_location('multiply_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    build_start=time.perf_counter()
    with (directory/'observer-build.log').open('wb') as log:
        subprocess.run(['make','-j4','build/reference-arithmetic-observer.exe'],cwd=BINARY,stdout=log,stderr=subprocess.STDOUT,check=True)
    observer_build_seconds=time.perf_counter()-build_start
    _,fixture,_,_=paths(config,'stack_lifetime')
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(tool.TC/'bin'),include=windows(tool.TC/'include'))
    program=phase['command_line_template'].format(bin=windows(tool.TC/'bin'))
    image=PE(tool.TC/'bin/C1XX3232.EXE');assert hashlib.sha256(image.data).hexdigest()==config['target']['sha256']
    drivers={r['start'] for r in config['driver']}
    records=[r for r in json.loads((BINARY/'runtime.json').read_text()) if r['start'] not in drivers]
    records+=imports(image)+config['driver']
    target=directory/'observed-original.exe';mask(image,records,config['accepted']+['_multiplication_overflows@16'],target)
    data=bytearray(target.read_bytes());assert image.read(0x74830,1)==b'\x83'
    data[image.offset(0x74830)]=0xcc;target.write_bytes(data)
    plan=directory/'vectors.plan'
    plan.write_text(''.join('%08x %08x %08x %08x\n'%(a,b,u,0xa5a50000|w) for a,b,u,w in VECTORS))
    with (CAPTURE_ROOT/'capture.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        work=SCRATCH.joinpath(*canonical_parts(phase['canonical_working_directory']));work.mkdir(exist_ok=True)
        for existing in work.iterdir():assert existing.is_file() and not existing.is_symlink();existing.unlink()
        for item in phase['inputs']:
            raw=(fixture/item['path']).read_bytes();assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256']
            (work/Path(item['path']).name).write_bytes(raw)
        environment=tool.environment();environment.update(C1XX_ARITHMETIC_PLAN=windows(plan),C1XX_ARITHMETIC_LOG=windows(directory/'original-rows.txt'))
        command=['wine',windows(BINARY/'build/reference-arithmetic-observer.exe'),windows(target),'',
            windows(directory/'original-stdout.bin'),windows(directory/'original-stderr.bin'),windows(directory/'original-events.txt'),
            phase['canonical_working_directory'],'MSC_CMD_FLAGS='+flags,program]
        original_start=time.perf_counter()
        with (directory/'original-launcher.log').open('wb') as log:
            subprocess.run(command,cwd=BINARY,env=environment,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
        original_seconds=time.perf_counter()-original_start
        shutil.copytree(work,directory/'original-files')
    rows=consumed_rows((directory/'original-rows.txt').read_text());validate(rows)
    # Link the same uninstrumented native object as the application. This
    # supplemental direct probe does not replace the fresh actual-case pair.
    matrix=[(0,4,0,4)]+VECTORS
    driver=directory/'native-driver.c'
    driver.write_text('#include <stdio.h>\nextern int __attribute__((stdcall)) multiplication_overflows(unsigned,unsigned,short,unsigned short);\n'
        +'static unsigned v[][4]={'+','.join('{%uU,%uU,%uU,%uU}'%r for r in matrix)+'};\n'
        +'int main(int argc,char **argv){unsigned i;int r;if(argc>1){multiplication_overflows(1,~0U,0,3);return 125;}for(i=0;i<sizeof(v)/sizeof(v[0]);++i){r=multiplication_overflows(v[i][0],v[i][1],(short)v[i][2],(unsigned short)v[i][3]);printf("result %u %08x %08x %u %u %d\\n",i,v[i][0],v[i][1],v[i][2],v[i][3],r);}return 0;}\n')
    native=directory/'native-probe';build_start=time.perf_counter()
    subprocess.run(['clang','-m32','-O0','-g',str(driver),str(BINARY/'build/native/arithmetic.o'),'-o',str(native)],check=True)
    native_build_seconds=time.perf_counter()-build_start
    native_start=time.perf_counter();r=subprocess.run([str(native)],capture_output=True,text=True,check=True)
    native_seconds=time.perf_counter()-native_start
    (directory/'native-rows.txt').write_text(r.stdout)
    actual=consumed_rows(r.stdout);validate(actual);assert actual==rows
    disable_core=lambda:resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    denial=subprocess.run([str(native),'unsupported-width'],capture_output=True,preexec_fn=disable_core)
    assert denial.returncode==-signal.SIGTRAP,'unsupported width did not stop at native guard'
    negative=[]
    for label,changed in [('missing-observer',[]),('wrong-consumed-width',[(*rows[0][:3],3,rows[0][4])]+rows[1:])]:
        try:validate(changed)
        except AssertionError:negative.append(label)
        else:raise AssertionError(label+' unexpectedly accepted')
    shared=snapshot();compiled=materialize(shared)
    source=(BINARY/'src/arithmetic.c').read_text();assert source.count('__asm { int 3 }')==1
    mutation=directory/'initialized-default.c';mutation.write_text(source.replace('__asm { int 3 }','threshold = 0;'))
    fabricated=directory/'fabricated-probe';mutation_start=time.perf_counter()
    mutation_command=['clang','-m32','-std=gnu89','-O0','-g','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST',
        '-I'+str(compiled/'include'),'-I'+str(compiled/'native'),'-I'+str(BINARY/'src'),str(driver),str(mutation),'-o',str(fabricated)]
    subprocess.run(mutation_command,check=True)
    mutation_build_seconds=time.perf_counter()-mutation_start
    invented=subprocess.run([str(fabricated),'unsupported-width'],capture_output=True,preexec_fn=disable_core)
    assert invented.returncode==125 and invented.returncode!=denial.returncode
    negative.append('fabricated-initialized-default fails guard-status oracle')
    report=dict(status='exact bounded original/native returns; unsupported native width denied',
        rows=[dict(left=a,right=b,unsigned_arithmetic=u,consumed_width=w,result=r) for a,b,u,w,r in rows],
        original_processes=1,observer_build_seconds=observer_build_seconds,original_execution_seconds=original_seconds,
        native_probe_build_seconds=native_build_seconds,native_probe_execution_seconds=native_seconds,
        mutation_build_seconds=mutation_build_seconds,total_seconds=time.perf_counter()-started,
        native_guard_status=denial.returncode,initialized_default_variant_status=invented.returncode,rejections=negative,
        config_sha256=hashlib.sha256(config_data).hexdigest(),original_sha256=config['target']['sha256'],
        native_object_sha256=hashlib.sha256((BINARY/'build/native/arithmetic.o').read_bytes()).hexdigest(),
        observer_sha256=hashlib.sha256((BINARY/'build/reference-arithmetic-observer.exe').read_bytes()).hexdigest(),
        shared_inputs=shared,scope='First0*4 invocation is unchanged lifetime case; following rows are bounded direct diagnostics reusing that call stack. Native direct calls use the application object. Actual-case fresh frontier pair remains required.',
        width_abi='unsigned short bytes consumes low16; original incidental highword retained raw and deliberately varied on supplemental probes, never equated with native parameter bits',
        limitation='signed opposite-positive-status unsupported widths retain original uninitialized Win32 threshold; native inline INT3, no universal malformed descriptor invariant claimed')
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(directory)


if __name__=='__main__':main()
