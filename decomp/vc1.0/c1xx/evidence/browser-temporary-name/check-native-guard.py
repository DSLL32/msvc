"""Inject only an empty browser option at the genuine native formatter entry."""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
BINARY=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(BINARY/'scripts'))
from case_paths import CAPTURE_ROOT,ROOT,windows,native_work
started=time.perf_counter()
spec=importlib.util.spec_from_file_location('native_guard_capture',BINARY/'scripts/capture-native.py')
capture=importlib.util.module_from_spec(spec);spec.loader.exec_module(capture)
scratch=Path(tempfile.mkdtemp(prefix='browser-empty-name-guard-',dir=CAPTURE_ROOT))
entry=scratch/'entry';observed=capture.capture(entry,breakpoint='format_browser_temporary_name',case_name='member_registry_browser')
assert observed['function']=='format_browser_temporary_name'
fixture=BINARY.parent/'fixtures/c1xx-member-registry-browser'
phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
guard=scratch/'empty-option';guard.mkdir();work,drive_environment=native_work(guard,phase['canonical_working_directory'])
for item in phase['inputs']:(work/Path(item['path']).name).write_bytes((fixture/item['path']).read_bytes())
commands=(entry/'commands.gdb').read_text().replace(str(entry),str(guard))
line=next(line for line in commands.splitlines() if line.startswith('run > '))
commands=commands.replace(line,line+'\nset {char}options.option_Fr = 0\ncontinue\nx/i $pc-1',1)
(guard/'commands.gdb').write_text(commands)
env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'):env.pop(key,None)
windows_bin=windows(ROOT/'toolchains/1.0-nt/bin')
env['MSC_CMD_FLAGS']=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows_bin,include=windows(ROOT/'toolchains/1.0-nt/include'))
env.update(drive_environment);env.update(RECON_PROGRAM_PATH=phase['command_line_template'].format(bin=windows_bin),TZ='PST8PDT')
result=subprocess.run(['gdb','-q','-batch','-x',str(guard/'commands.gdb'),'--args',str(BINARY/'build/native/C1XX3232.EXE')],cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
(guard/'gdb.log').write_bytes(result.stdout+result.stderr)
assert result.returncode==0
checkpoint=json.loads((guard/'checkpoint.json').read_text())
assert checkpoint['function']=='format_browser_temporary_name' and checkpoint['signal']==5
log=(guard/'gdb.log').read_text();assert 'int3' in log and '<format_browser_temporary_name+' in log, 'missing actual guard INT3 instruction'
assert {p.name for p in work.iterdir()}=={'registry.cpp','registry.h'}, 'guard crossed file-creation boundary'
assert checkpoint['buffers']==[None]*6
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
report=dict(schema=1,scope=__doc__,status='held exact unsupported native empty-name branch',scratch=str(scratch),native_executable_sha256=sha(BINARY/'build/native/C1XX3232.EXE'),configuration_sha256=sha(BINARY/'functions.json'),checker_sha256=sha(Path(__file__)),evidence={str(p):sha(p) for d in [entry,guard] for p in sorted(d.iterdir()) if p.is_file()},original_invalid_case_invocations=0,unsupported_original_stack_equivalence=False,elapsed_seconds=time.perf_counter()-started)
for src,name in [(guard/'gdb.log','guard-native-gdb.log'),(guard/'checkpoint.json','guard-native-checkpoint.json'),(guard/'commands.gdb','guard-native-commands.gdb')]:shutil.copyfile(src,Path(__file__).with_name(name))
Path(__file__).with_name('guard-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
print(Path(__file__).with_name('guard-receipt.json'))
