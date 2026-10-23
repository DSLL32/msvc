"""Explicit original/native realloc entry and return proof, outside fixtures."""
import argparse
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
from case_paths import paths
from runtime_boundary_loader import load_boundary

BINARY=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original_capture',type=Path)
    parser.add_argument('--case',required=True)
    parser.add_argument('--runtime-captures',type=Path,required=True)
    args=parser.parse_args()
    boundary=load_boundary();sha=boundary.sha
    spec=importlib.util.spec_from_file_location('c1xx_realloc_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    config=json.loads((BINARY/'functions.json').read_text())
    before=json.loads((args.original_capture/'receipt.json').read_text())
    assert before['candidate']=='_matches_virtual_override@12' and before['terminal']=='_realloc'
    assert before['case']==args.case and before['configuration_sha256']==sha(BINARY/'functions.json')
    runtime_captures=[Path(p).parent for p in json.loads(args.runtime_captures.read_text())]
    names=[json.loads((p/'receipt.json').read_text())['candidate'] for p in runtime_captures]
    assert names==['_realloc','__heap_expand_block']
    identity=boundary.identities(canonical=True,names=names)
    original=PE(tool.TC/'bin/C1XX3232.EXE')
    directory=Path(tempfile.mkdtemp(prefix='realloc-boundary-',dir=tool.PREFIX.parent))
    plan=directory/'restore-plan.txt'
    plan.write_text(''.join('%x %x %x %s\n'%(r['record']['start'],original.offset(r['record']['start']),r['record']['end']-r['record']['start'],r['name']) for r in identity['records']))
    _,fixture,_,_=paths(config,args.case)
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC/'bin'),include=tool.windows(tool.TC/'include'))
    program=phase['command_line_template'].format(bin=tool.windows(tool.TC/'bin'))
    def prepare(name):
        work=directory/name;work.mkdir()
        for item in phase['inputs']:shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
        return work
    work=prepare('original-case')
    command=['wine',str(BINARY/'build/reference-realloc-observer.exe'),tool.windows(args.original_capture/'masked-original.exe'),'',
        tool.windows(directory/'original-stdout.bin'),tool.windows(directory/'original-stderr.bin'),tool.windows(directory/'original-events.txt'),
        tool.windows(work),'MSC_CMD_FLAGS='+flags,program]
    env=tool.environment();env.update(C1XX_REALLOC_PLAN=tool.windows(plan),C1XX_REALLOC_LOG=tool.windows(directory/'original-observation.txt'),C1XX_REALLOC_IMAGE=tool.windows(tool.TC/'bin/C1XX3232.EXE'))
    started=time.perf_counter();subprocess.run(command,env=env,check=True,timeout=90,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);original_seconds=time.perf_counter()-started
    raw=(directory/'original-observation.txt').read_text()
    observed={k:int(re.search(r'^'+k+r' (\d+)$',raw,re.M)[1]) for k in ('size','count','capacity','result_nonnull')}
    observed['return_va']=int(re.search(r'^return_va ([0-9a-f]+)$',raw,re.M)[1],16)
    for key in ('before','after','before_name','after_name'):observed[key]=re.search(r'^'+key+r' (.*)$',raw,re.M)[1]
    observed['runtime_order']=re.findall(r'^runtime (\S+)$',raw,re.M)
    work=prepare('native-case')
    commands=['break grow_virtual_entries','run > '+str(directory/'native-stdout.bin')+' 2> '+str(directory/'native-stderr.bin'),
        'disable 1','break realloc','continue','python','import gdb,json',
        'pointer=int(gdb.parse_and_eval("*(unsigned*)($esp+4)"))',
        'size=int(gdb.parse_and_eval("*(unsigned*)($esp+8)"))',
        'table=gdb.newest_frame().older().read_var("table")',
        'count=int(table["entry_count"]); capacity=int(table["entry_capacity"])',
        'memory=gdb.selected_inferior()',
        'before=bytes(memory.read_memory(pointer,count*16)).hex()',
        'def symbol_name(pointer): return gdb.Value(pointer).cast(gdb.lookup_type("CppSymbol").pointer())["name"].string()',
        'before_name=symbol_name(int.from_bytes(bytes.fromhex(before)[:4],"little"))','end','finish','python',
        'result=int(gdb.parse_and_eval("(unsigned)$eax"))',
        'after=bytes(memory.read_memory(result,count*16)).hex() if result else ""',
        'after_name=symbol_name(int.from_bytes(bytes.fromhex(after)[:4],"little")) if result else ""',
        'observed=dict(size=size,count=count,capacity=capacity,result_nonnull=int(bool(result)),before=before,after=after,before_name=before_name,after_name=after_name,caller=gdb.newest_frame().name())',
        'open('+repr(str(directory/'native-observation.json'))+',"w").write(json.dumps(observed,indent=2)+"\\n")','end']
    script=directory/'native.gdb';script.write_text('\n'.join(commands)+'\n')
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
    env.update(MSC_CMD_FLAGS=flags,RECON_PROGRAM_PATH=program,TZ='PST8PDT')
    started=time.perf_counter();result=subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/C1XX3232.EXE')],env=env,cwd=work,stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=60);native_seconds=time.perf_counter()-started
    (directory/'native-gdb.log').write_text(result.stdout+result.stderr)
    assert result.returncode==0,result.stdout+result.stderr
    native=json.loads((directory/'native-observation.json').read_text())
    print(directory)
    assert observed['return_va']==0x78be2 and native['caller']=='grow_virtual_entries'
    assert observed['runtime_order']==names
    assert observed['count']==native['count']==1 and observed['capacity']==native['capacity']==3
    assert observed['size']==native['size']==48 and observed['result_nonnull']==native['result_nonnull']==1
    for side in (observed,native):assert side['before']==side['after'] and side['before_name']==side['after_name'],'realloc did not preserve prefix'
    assert observed['before_name']==native['before_name'],'entry function differs'
    assert observed['before'][8:]==native['before'][8:],'nonpointer entry bytes differ'
    expected={p.name:p.read_bytes() for p in (directory/'original-case').iterdir() if p.is_file()}
    actual={p.name:p.read_bytes() for p in work.iterdir() if p.is_file()}
    assert expected==actual,'realloc boundary files differ'
    for stream in ('stdout.bin','stderr.bin'):assert (directory/('original-'+stream)).read_bytes()==(directory/('native-'+stream)).read_bytes()
    receipt=dict(kind='realloc',case=args.case,scope='explicit realloc boundary; pointer addresses represented by named symbol identity, all remaining input bytes compared, raw prefix preserved on each side',
        target_sha256=config['target']['sha256'],configuration_sha256=sha(BINARY/'functions.json'),accepted_before=len(config['accepted']),runtime_order=names,
        original=observed,native=native,original_capture=str(args.original_capture),original_capture_sha256=sha(args.original_capture/'receipt.json'),
        original_seconds=original_seconds,native_seconds=native_seconds,identity=identity)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':main()
