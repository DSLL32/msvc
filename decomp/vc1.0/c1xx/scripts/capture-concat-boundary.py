"""Explicit strncat entry/return proof, outside cached fixtures."""
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
    parser.add_argument('--reuse-original-observation',type=Path)
    args=parser.parse_args()
    boundary=load_boundary();sha=boundary.sha
    spec=importlib.util.spec_from_file_location('c1xx_string_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    config=json.loads((BINARY/'functions.json').read_text())
    before=json.loads((args.original_capture/'receipt.json').read_text())
    assert before['candidate']=='_write_debug_member_name@20' and before['terminal']=='_strncat'
    assert before['case']==args.case and before['configuration_sha256']==sha(BINARY/'functions.json')
    identity=boundary.identities(canonical=True,names=['_strncat'])
    original=PE(tool.TC/'bin/C1XX3232.EXE')
    directory=Path(tempfile.mkdtemp(prefix='concat-boundary-',dir=CAPTURE_ROOT))
    plan=directory/'restore-plan.txt'
    plan.write_text(''.join('%x %x %x %s\n'%(r['record']['start'],original.offset(r['record']['start']),r['record']['end']-r['record']['start'],r['name']) for r in identity['records']))
    _,fixture,_,_=paths(config,args.case)
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    canonical=phase['canonical_working_directory']
    if args.reuse_original_observation:
        source=args.reuse_original_observation
        for filename in ('original-stdout.bin','original-stderr.bin','original-events.txt','original-observation.txt'):
            shutil.copy2(source/filename,directory/filename)
        shutil.copytree(source/'original-case',directory/'original-case')
        original_seconds=0.0;original_setup_seconds=0.0
        flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC/'bin'),include=tool.windows(tool.TC/'include'))
        program=phase['command_line_template'].format(bin=tool.windows(tool.TC/'bin'))
    else:
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
        command=['wine',tool.windows(BINARY/'build/reference-concat-observer.exe'),tool.windows(args.original_capture/'masked-original.exe'),'',
            tool.windows(directory/'original-stdout.bin'),tool.windows(directory/'original-stderr.bin'),tool.windows(directory/'original-events.txt'),
            canonical,'MSC_CMD_FLAGS='+flags,program,'','',tool.windows(args.original_capture/'entries.plan')]
        env=tool.environment();env.update(C1XX_CONCAT_PLAN=tool.windows(plan),C1XX_CONCAT_LOG=tool.windows(directory/'original-observation.txt'),C1XX_CONCAT_IMAGE=tool.windows(tool.TC/'bin/C1XX3232.EXE'))
        original_setup_seconds=time.perf_counter()-setup_started
        started=time.perf_counter();subprocess.run(command,env=env,check=True,timeout=90,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);original_seconds=time.perf_counter()-started
        shutil.copytree(original_work,directory/'original-case')
        fcntl.flock(lock,fcntl.LOCK_UN);lock.close()
    raw=(directory/'original-observation.txt').read_text()
    observed={k:int(re.search(r'^'+k+r' (-?\d+)$',raw,re.M)[1]) for k in ('count','returned_destination')}
    for key in ('return_va','destination','source'):
        observed[key]=int(re.search(r'^'+key+r' ([0-9a-f]+)$',raw,re.M)[1],16)
    for stage in ('before_destination','before_source','after_destination','after_source'):
        observed[stage]=re.search(r'^'+stage+r' (.*)$',raw,re.M)[1]
    observed['runtime_order']=re.findall(r'^runtime (\S+)$',raw,re.M)
    setup_started=time.perf_counter();work,drive_environment=native_work(directory/'native-drive',canonical)
    for item in phase['inputs']:shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
    commands=['break write_debug_member_name','run > '+str(directory/'native-stdout.bin')+' 2> '+str(directory/'native-stderr.bin'),
        'disable 1',"break *'strncat@plt'",'continue','python','import gdb,json',
        'destination=int(gdb.parse_and_eval("*(unsigned*)($esp+4)"))',
        'source=int(gdb.parse_and_eval("*(unsigned*)($esp+8)"))',
        'count=int(gdb.parse_and_eval("*(unsigned*)($esp+12)"))',
        'entry=gdb.newest_frame().name()',
        'entry_pc=int(gdb.parse_and_eval("$pc")); entry_library=gdb.solib_name(entry_pc)',
        'memory=gdb.selected_inferior()',
        'def contents(address):',
        '    result=bytearray()',
        '    for offset in range(512):',
        '        value=bytes(memory.read_memory(address+offset,1))[0]; result.append(value)',
        '        if not value: return result.hex()',
        '    raise AssertionError("bounded C string exceeds512bytes")',
        'before_destination=contents(destination); before_source=contents(source)',
        'gdb.execute("set $concat_return = *(unsigned*)$esp")', 'end','stepi','python',
        'implementation_pc=int(gdb.parse_and_eval("$pc")); implementation_library=gdb.solib_name(implementation_pc)',
        'end','disable 2','tbreak *$concat_return','continue','python',
        'result=int(gdb.parse_and_eval("(unsigned)$eax"))',
        'observed=dict(count=count,returned_destination=int(result==destination),before_destination=before_destination,before_source=before_source,after_destination=contents(destination),after_source=contents(source),entry=entry,entry_pc=entry_pc,entry_library=entry_library,entry_requested="strncat@plt",implementation_pc=implementation_pc,implementation_library=implementation_library,caller=gdb.newest_frame().name())',
        'open('+repr(str(directory/'native-observation.json'))+',"w").write(json.dumps(observed,indent=2)+"\\n")','end']
    script=directory/'native.gdb';script.write_text('\n'.join(commands)+'\n')
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
    env.update(MSC_CMD_FLAGS=flags,RECON_PROGRAM_PATH=program,TZ='PST8PDT',LD_BIND_NOW='1')
    env.update(drive_environment)
    native_setup_seconds=time.perf_counter()-setup_started
    started=time.perf_counter();result=subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/C1XX3232.EXE')],env=env,cwd=work,stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=60);native_seconds=time.perf_counter()-started
    (directory/'native-gdb.log').write_text(result.stdout+result.stderr)
    print(directory)
    assert result.returncode==0,result.stdout+result.stderr
    native=json.loads((directory/'native-observation.json').read_text())
    assert observed['return_va']==0x6ed00 and native['caller']=='write_debug_member_name'
    assert observed['runtime_order']==['_strncat'] and native['implementation_library']
    for key in ('count','returned_destination','before_destination','before_source','after_destination','after_source'):
        assert observed[key]==native[key], (key,observed,native)
    assert observed['returned_destination']==1
    destination=bytes.fromhex(observed['before_destination'])
    source=bytes.fromhex(observed['before_source'])
    assert destination.endswith(b'\0') and source.endswith(b'\0')
    assert bytes.fromhex(observed['after_destination'])==destination[:-1]+source[:-1][:observed['count']]+b'\0'
    assert observed['before_source']==observed['after_source']
    expected={p.name:p.read_bytes() for p in (directory/'original-case').iterdir() if p.is_file()}
    actual={p.name:p.read_bytes() for p in work.iterdir() if p.is_file()}
    assert expected==actual,'copy boundary files differ'
    for stream in ('stdout.bin','stderr.bin'):assert (directory/('original-'+stream)).read_bytes()==(directory/('native-'+stream)).read_bytes()
    receipt=dict(kind='strncat',case=args.case,scope='actual conversion-name bounded append: complete destination/source strings, count, returned destination and resulting string; no CRT source reconstruction',
        target_sha256=config['target']['sha256'],configuration_sha256=sha(BINARY/'functions.json'),accepted_before=len(config['accepted']),runtime_order=['_strncat'],
        original=observed,native=native,original_capture=str(args.original_capture),original_capture_sha256=sha(args.original_capture/'receipt.json'),
        original_seconds=original_seconds,native_seconds=native_seconds,reused_original_observation=str(args.reuse_original_observation) if args.reuse_original_observation else None,original_setup_seconds=original_setup_seconds,native_setup_seconds=native_setup_seconds,identity=identity)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':main()
