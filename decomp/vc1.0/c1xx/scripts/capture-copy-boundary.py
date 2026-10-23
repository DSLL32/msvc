"""Explicit overlapping CRT copy entry/return proof, outside cached fixtures."""
import argparse
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from binary import PE
from case_paths import paths, CAPTURE_ROOT, canonical_parts, SCRATCH, native_work
from runtime_boundary_loader import load_boundary

BINARY=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original_capture',type=Path)
    parser.add_argument('--case',required=True)
    args=parser.parse_args()
    boundary=load_boundary();sha=boundary.sha
    spec=importlib.util.spec_from_file_location('c1xx_copy_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    config=json.loads((BINARY/'functions.json').read_text())
    before=json.loads((args.original_capture/'receipt.json').read_text())
    assert before['candidate']=='_virtual_base_access_level@8' and before['terminal']=='_memcpy'
    assert before['case']==args.case and before['configuration_sha256']==sha(BINARY/'functions.json')
    identity=boundary.identities(canonical=True,names=['_memcpy'])
    original=PE(tool.TC/'bin/C1XX3232.EXE')
    directory=Path(tempfile.mkdtemp(prefix='copy-boundary-',dir=CAPTURE_ROOT))
    plan=directory/'restore-plan.txt'
    plan.write_text(''.join('%x %x %x %s\n'%(r['record']['start'],original.offset(r['record']['start']),r['record']['end']-r['record']['start'],r['name']) for r in identity['records']))
    _,fixture,_,_=paths(config,args.case)
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    lock=(CAPTURE_ROOT/'capture.lock').open('a'); fcntl.flock(lock,fcntl.LOCK_EX)
    canonical=phase['canonical_working_directory']
    original_work=SCRATCH.joinpath(*canonical_parts(canonical))
    original_work.mkdir(parents=True,exist_ok=True)
    for entry in original_work.iterdir():
        assert entry.is_file() and not entry.is_symlink(),str(entry)
        entry.unlink()

    flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC/'bin'),include=tool.windows(tool.TC/'include'))
    program=phase['command_line_template'].format(bin=tool.windows(tool.TC/'bin'))
    setup_started=time.perf_counter()
    work=original_work
    for item in phase['inputs']:shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
    command=['wine',tool.windows(BINARY/'build/reference-copy-observer.exe'),tool.windows(args.original_capture/'masked-original.exe'),'',
        tool.windows(directory/'original-stdout.bin'),tool.windows(directory/'original-stderr.bin'),tool.windows(directory/'original-events.txt'),
        canonical,'MSC_CMD_FLAGS='+flags,program]
    env=tool.environment();env.update(C1XX_COPY_PLAN=tool.windows(plan),C1XX_COPY_LOG=tool.windows(directory/'original-observation.txt'),C1XX_COPY_IMAGE=tool.windows(tool.TC/'bin/C1XX3232.EXE'))
    original_setup_seconds=time.perf_counter()-setup_started
    started=time.perf_counter();subprocess.run(command,env=env,check=True,timeout=90,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);original_seconds=time.perf_counter()-started
    shutil.copytree(original_work,directory/'original-case')
    fcntl.flock(lock,fcntl.LOCK_UN);lock.close()
    raw=(directory/'original-observation.txt').read_text()
    observed={k:int(re.search(r'^'+k+r' (-?\d+)$',raw,re.M)[1]) for k in ('size','delta','returned_destination')}
    observed['return_va']=int(re.search(r'^return_va ([0-9a-f]+)$',raw,re.M)[1],16)
    for stage in ('before','after'):
        observed[stage]=re.search(r'^'+stage+r' (.*)$',raw,re.M)[1]
        observed[stage+'_names']=[name for _,name in re.findall(r'^'+stage+r'_name (\d+) (.*)$',raw,re.M)]
    observed['runtime_order']=re.findall(r'^runtime (\S+)$',raw,re.M)
    setup_started=time.perf_counter();work,drive_environment=native_work(directory/'native-drive',canonical)
    for item in phase['inputs']:shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
    commands=['break add_inline_pointer','run > '+str(directory/'native-stdout.bin')+' 2> '+str(directory/'native-stderr.bin'),
        'disable 1','break memmove','continue','python','import gdb,json',
        'destination=int(gdb.parse_and_eval("*(unsigned*)($esp+4)"))',
        'source=int(gdb.parse_and_eval("*(unsigned*)($esp+8)"))',
        'size=int(gdb.parse_and_eval("*(unsigned*)($esp+12)"))',
        'entry=gdb.newest_frame().name()',
        'entry_pc=int(gdb.parse_and_eval("$pc")); entry_library=gdb.solib_name(entry_pc)',
        'memory=gdb.selected_inferior()',
        'before=bytes(memory.read_memory(source,size)).hex()',
        'def names(data): return [gdb.Value(int.from_bytes(bytes.fromhex(data)[i:i+4],"little")).cast(gdb.lookup_type("CppSymbol").pointer())["name"].string() for i in range(0,len(bytes.fromhex(data)),4)]',
        'before_names=names(before)','end','finish','python',
        'result=int(gdb.parse_and_eval("(unsigned)$eax"))',
        'after=bytes(memory.read_memory(destination,size)).hex()',
        'observed=dict(size=size,delta=destination-source,returned_destination=int(result==destination),before=before,after=after,before_names=before_names,after_names=names(after),entry=entry,entry_pc=entry_pc,entry_library=entry_library,entry_requested="memmove",entry_binding="GDB memmove breakpoint resolved implementation",caller=gdb.newest_frame().name())',
        'open('+repr(str(directory/'native-observation.json'))+',"w").write(json.dumps(observed,indent=2)+"\\n")','end']
    script=directory/'native.gdb';script.write_text('\n'.join(commands)+'\n')
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
    env.update(MSC_CMD_FLAGS=flags,RECON_PROGRAM_PATH=program,TZ='PST8PDT')
    env.update(drive_environment)
    native_setup_seconds=time.perf_counter()-setup_started
    started=time.perf_counter();result=subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/C1XX3232.EXE')],env=env,cwd=work,stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=60);native_seconds=time.perf_counter()-started
    (directory/'native-gdb.log').write_text(result.stdout+result.stderr)
    print(directory)
    assert result.returncode==0,result.stdout+result.stderr
    native=json.loads((directory/'native-observation.json').read_text())
    assert observed['return_va']==0x4e4ad and native['caller']=='add_inline_pointer'
    assert observed['runtime_order']==['_memcpy']
    for key,value in [('size',8),('delta',4),('returned_destination',1)]:assert observed[key]==native[key]==value,(key,observed,native)
    for side in (observed,native):assert side['before']==side['after'] and side['before_names']==side['after_names'],'overlapping copy lost input bytes'
    assert observed['before_names']==native['before_names'],'copied class-tag identities differ'
    expected={p.name:p.read_bytes() for p in (directory/'original-case').iterdir() if p.is_file()}
    actual={p.name:p.read_bytes() for p in work.iterdir() if p.is_file()}
    assert expected==actual,'copy boundary files differ'
    for stream in ('stdout.bin','stderr.bin'):assert (directory/('original-'+stream)).read_bytes()==(directory/('native-'+stream)).read_bytes()
    receipt=dict(kind='overlap_copy',case=args.case,scope='explicit overlapping suffix move; exactly two named class-tag pointers, raw before/after preservation on each side, returned destination identity',
        target_sha256=config['target']['sha256'],configuration_sha256=sha(BINARY/'functions.json'),accepted_before=len(config['accepted']),runtime_order=['_memcpy'],
        original=observed,native=native,original_capture=str(args.original_capture),original_capture_sha256=sha(args.original_capture/'receipt.json'),
        original_seconds=original_seconds,native_seconds=native_seconds,original_setup_seconds=original_setup_seconds,native_setup_seconds=native_setup_seconds,identity=identity)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':main()
