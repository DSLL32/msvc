#!/usr/bin/env python3
"""Modern type-compactor exit and the next symbol processor's real ABI."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha


start=time.perf_counter()
pair=BINARY/'cases/hello-debug-dll/evidence/0056-compact-type-index'
config=json.loads((pair/'original/configuration.json').read_text())
data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text()
native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,configuration=config):return decoder.compare(configuration,data,events,state,artifacts,views)
baseline=compare(native)
assert baseline['parameters']['size']==baseline['parameters']['lines']['size']==8093
assert baseline['parameters']['symbols']['global_name']=='module_symbol_count'
assert baseline['parameters']['references']['global_name']=='module_reference_count'
assert native['observed_argument_types'][2]['pointee_width']==2
assert native['observed_argument_types'][3]['pointee_width']==2
rejected=[]
def reject(name,mutation,reason):
    state=copy.deepcopy(native);mutation(state)
    try:compare(state)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('changed-direct-count',lambda s:s['parameters'].__setitem__('size',8092),'typed ABI arguments differ')
reject('changed-buffer-defined-size',lambda s:s['parameters']['lines'].update(size=8092),'typed ABI arguments differ')
reject('changed-buffer-alias',lambda s:s['parameters']['lines'].update(global_name='symbol_data'),'typed ABI arguments differ')
reject('swapped-output-global-alias',lambda s:s['parameters']['symbols'].update(global_name='module_reference_count'),'typed ABI arguments differ')
reject('omitted-actual-formal',lambda s:s.__setitem__('observed_argument_names',[]),'actual frontier formal-argument inventory differs')
reject('count-pointer-confusion',lambda s:s['observed_argument_types'][1].update(kind='pointer'),'actual frontier formal type differs')
reject('incorrect-output-word-width',lambda s:s['observed_argument_types'][2].update(pointee_width=4),'actual frontier pointee extent differs')
reject('incorrect-reference-word-width',lambda s:s['observed_argument_types'][3].update(pointee_width=4),'actual frontier pointee extent differs')
reject('primitive-left-recursion-pending',lambda s:s['globals'].__setitem__('recursive_type_pending','0100'),'application global differs')
reject('primitive-changed-recursion-depth',lambda s:s['globals'].__setitem__('type_recursion_depth','0100'),'application global differs')
def change_buffer(state):
    address=int.from_bytes(bytes.fromhex(state['globals']['type_data']),'little')
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[8000]^=1
    state['memory'][str(address)]=raw.hex()
reject('changed-defined-symbol-bytes',change_buffer,'initialized subsection buffer bytes differ')

# Exercise the actual DWARF observer rather than merely mutating its JSON output.
actual=[]
for name,mutation,reason in [
    ('omitted-symbol-formals',lambda row:row.update(parameters=[]),'incomplete frontier ABI contract'),
    ('wrong-actual-output-pointee',lambda row:row['parameters'][2].update(native_pointee_width=4),'actual pointer formal pointee extent differs')]:
    with tempfile.TemporaryDirectory(prefix='cvpack-modern-type-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);configuration=copy.deepcopy(config)
        mutation(next(row for row in configuration['functions'] if row['name']=='decode_new_lines'))
        bad=directory/'configuration.json';bad.write_text(json.dumps(configuration))
        output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
            '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],
            cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
        assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,returncode=result.returncode,diagnostic=diagnostic,
            bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py')))

report=dict(schema=1,native_only=True,rejected=rejected,actual_gdb_rejections=actual,
    count=len(rejected)+len(actual),actual_formals=native['observed_argument_types'],
    direct_symbol_input_bytes=8093,typed_records=baseline['typed_record_entries_checked'],
    checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),
    observer_sha256=sha(BINARY/'scripts/gdb-state.py'),
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    seconds=time.perf_counter()-start)
(pair/'modern-type-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
