"""Explicit original/native constructor marker arguments and returns.

Reconstruction diagnostic only; never called by cached fixtures. The fixed
valid lifetime input stops at its pending constructor-access probe.
"""
import fcntl
import hashlib
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
from case_paths import BINARY, ROOT, SCRATCH, CAPTURE_ROOT, paths, canonical_parts, native_work, windows
from win32_indexes import imports, mask


def main():
    config_data=(BINARY/'functions.json').read_bytes();config=json.loads(config_data)
    assert config['cases']['stack_lifetime']['terminal']=='_is_void_parameter_constructor@4'
    candidate='_is_void_parameter_constructor@4'
    spec=importlib.util.spec_from_file_location('constructor_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    _,fixture,_,_=paths(config,'stack_lifetime')
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(tool.TC/'bin'),include=windows(tool.TC/'include'))
    program=phase['command_line_template'].format(bin=windows(tool.TC/'bin'))
    directory=Path(tempfile.mkdtemp(prefix='constructor-marker-',dir=CAPTURE_ROOT))
    image=PE(tool.TC/'bin/C1XX3232.EXE')
    assert hashlib.sha256(image.data).hexdigest()==config['target']['sha256']
    drivers={r['start'] for r in config['driver']}
    records=[r for r in json.loads((BINARY/'runtime.json').read_text()) if r['start'] not in drivers]
    records+=imports(image)+config['driver']
    target=directory/'observed-original.exe'
    mask(image,records,config['accepted']+[candidate],target)
    data=bytearray(target.read_bytes())
    for address in (0x37f50,0x37f90):
        assert image.read(address,1)==b'\x8b'
        data[image.offset(address)]=0xcc
    target.write_bytes(data)
    with (CAPTURE_ROOT/'capture.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        work=SCRATCH.joinpath(*canonical_parts(phase['canonical_working_directory']))
        work.mkdir(exist_ok=True)
        for existing in work.iterdir():
            assert existing.is_file() and not existing.is_symlink();existing.unlink()
        for item in phase['inputs']:
            source=fixture/item['path'];raw=source.read_bytes()
            assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256']
            (work/Path(item['path']).name).write_bytes(raw)
        environment=tool.environment();environment['C1XX_CONSTRUCTOR_LOG']=windows(directory/'original-markers.txt')
        command=['wine',windows(BINARY/'build/reference-constructor-observer.exe'),windows(target),'',
            windows(directory/'original-stdout.bin'),windows(directory/'original-stderr.bin'),
            windows(directory/'original-events.txt'),phase['canonical_working_directory'],
            'MSC_CMD_FLAGS='+flags,program]
        start=time.perf_counter()
        with (directory/'original-launcher.log').open('wb') as log:
            subprocess.run(command,env=environment,cwd=BINARY,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
        original_seconds=time.perf_counter()-start
        shutil.copytree(work,directory/'original-files')
    original_log=(directory/'original-markers.txt').read_text()
    assert original_log.endswith('terminal make_constructor_access_probe\n')
    expected=[dict(kind=k,low=int(a,16),member=int(b,16),value=int(v)) for k,a,b,v in
        re.findall(r'^result (\S+) low ([0-9a-f]+) member ([0-9a-f]+) value (\d+) caller [0-9a-f]+$',original_log,re.M)]
    assert expected and any(r['member']&0x10 and not(r['low']&0x10) for r in expected)
    native=directory/'native';native.mkdir()
    work,drives=native_work(native,phase['canonical_working_directory'])
    for item in phase['inputs']:(work/Path(item['path']).name).write_bytes((fixture/item['path']).read_bytes())
    result_path=native/'markers.json'
    commands=['python','import gdb,json','marker_rows=[]',
        'class MarkerReturn(gdb.FinishBreakpoint):',
        '    def __init__(self,frame,row):',
        '        super().__init__(frame,internal=True)',
        '        self.row=row',
        '    def stop(self):',
        '        self.row["value"]=int(self.return_value)',
        '        marker_rows.append(self.row)',
        '        open('+repr(str(result_path))+',"w").write(json.dumps(marker_rows,indent=2)+"\\n")',
        '        return False',
        'class MarkerEntry(gdb.Breakpoint):',
        '    def __init__(self,name,argument,kind):',
        '        super().__init__(name,internal=True)',
        '        self.argument=argument;self.kind=kind',
        '    def stop(self):',
        '        frame=gdb.newest_frame()',
        '        symbol=frame.read_var(self.argument)',
        '        raw=bytes(gdb.selected_inferior().read_memory(int(symbol)+48,4))',
        '        row={"kind":self.kind,"low":raw[0],"member":raw[2]}',
        '        assert int(symbol["cpp"]["parts"]["low_flags"])==raw[0]',
        '        assert int(symbol["cpp"]["parts"]["member_flags"])==raw[2]',
        '        MarkerReturn(frame,row)',
        '        return False',
        'MarkerEntry("default_constructor_candidate","candidate","callback")',
        'MarkerEntry("is_void_parameter_constructor","function","checker")','end',
        'run > '+str(native/'stdout.bin')+' 2> '+str(native/'stderr.bin'),'bt','python',
        'assert gdb.newest_frame().name()=="make_constructor_access_probe"',
        'assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5','end']
    (native/'commands.gdb').write_text('\n'.join(commands)+'\n')
    environment={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'):environment.pop(name,None)
    environment.update(drives,MSC_CMD_FLAGS=flags,RECON_PROGRAM_PATH=program,TZ='PST8PDT')
    start=time.perf_counter()
    result=subprocess.run(['gdb','-q','-batch','-x',str(native/'commands.gdb'),'--args',str(BINARY/'build/native/C1XX3232.EXE')],cwd=work,env=environment,capture_output=True,text=True,timeout=60)
    native_seconds=time.perf_counter()-start
    (native/'gdb.log').write_text(result.stdout+result.stderr)
    assert result.returncode==0,result.stdout+result.stderr
    actual=json.loads(result_path.read_text());assert actual==expected,(actual,expected)
    assert all(row['value']==((row['member']&0x10)>>4) for row in expected if row['kind']=='callback')
    report=dict(status='exact original/native marker arguments and returns',rows=expected,
        original_seconds=original_seconds,native_debugger_seconds=native_seconds,
        config_sha256=hashlib.sha256(config_data).hexdigest(),original_sha256=config['target']['sha256'],
        native_sha256=hashlib.sha256((BINARY/'build/native/C1XX3232.EXE').read_bytes()).hexdigest(),
        observer_sha256=hashlib.sha256((BINARY/'build/reference-constructor-observer.exe').read_bytes()).hexdigest(),
        operand={'offset':50,'mask':16,'typed_field':'CppSymbol.cpp.parts.member_flags','wrong_old_field_offset':48},
        scope='same unchanged lifetime inputs, accepted prefix plus pending checker; stops at constructor access probe; no fixture refresh')
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(directory)


if __name__=='__main__':main()
