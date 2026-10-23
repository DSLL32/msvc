#!/usr/bin/env python3
"""Live local/public symbol bytes, buffer aliases and actual GDB ABI controls."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pair',type=Path,default=BINARY/'cases/hello-debug-dll/evidence/0053-read-module-local-symbols-complete')
    parser.add_argument('--output',type=Path,default=BINARY/'cases/hello-debug-dll/symbol-subsection-checks.json')
    args=parser.parse_args();pair=args.pair
    config=json.loads((pair/'original/configuration.json').read_text())
    data=json.loads((pair/'original/data.json').read_text())
    events=(pair/'original/events').read_text();native=json.loads((pair/'native-formal-types/state.json').read_text())
    artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
    views={side:{p.name:p.read_bytes() for p in (pair/folder).glob(pattern)} for side,folder,pattern in
        [('original','original','events-view-*.bin'),('native','native-formal-types','native-view-*.bin')]}
    spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
    decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
    def compare(state,configuration=config):return decoder.compare(configuration,data,events,state,artifacts,views)
    start=time.perf_counter();baseline=compare(native);rejected=[]
    def root(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
    def change(state,name,offset):
        address=root(state,name);raw=bytearray.fromhex(state['memory'][str(address)])
        raw[offset]^=1;state['memory'][str(address)]=raw.hex()
    def reject(name,mutation,reason,config_change=None):
        state=copy.deepcopy(native);mutation(state)
        changed=copy.deepcopy(config)
        if config_change:config_change(changed)
        try:compare(state,changed)
        except (AssertionError,KeyError) as error:
            assert reason in str(error), ('wrong rejection guard',name,str(error))
            rejected.append(dict(name=name,diagnostic=str(error)))
        else:raise AssertionError('mutation accepted: '+name)
    reject('changed-local-symbol-byte',lambda s:change(s,'type_data',8000),'initialized subsection buffer bytes differ')
    reject('changed-retained-public-prefix',lambda s:change(s,'symbol_data',110),'initialized subsection buffer bytes differ')
    def truncate(state):
        address=root(state,'type_data');state['memory'][str(address)]=state['memory'][str(address)][:-2]
    reject('incomplete-local-symbol-capacity',truncate,'incomplete read-buffer capacity observation')
    reject('retained-subsection-alias',lambda s:s['globals'].__setitem__('module_symbol_records',(root(s,'type_data')+4).to_bytes(4,'little').hex()),'retained subsection buffer alias differs')
    reject('omitted-actual-formals',lambda s:s.__setitem__('observed_argument_names',[]),'actual frontier formal-argument inventory differs')
    reject('omitted-actual-formal-types',lambda s:s.__setitem__('observed_argument_types',[]),'actual frontier formal-type inventory differs')
    reject('integer-pointer-formal-confusion',lambda s:s['observed_argument_types'][0].update(kind='pointer'),'actual frontier formal type differs')
    reject('wrong-formal-width',lambda s:s['observed_argument_types'][0].update(width=2),'actual frontier formal width differs')
    reject('wrong-directory-pointee-extent',lambda s:s['observed_argument_types'][1].update(pointee_width=8),'actual frontier pointee extent differs')
    def empty_contract(configuration):
        row=next(row for row in configuration['functions'] if row['name']=='read_module_global_types')
        row['parameters']=[]
    reject('empty-contract-cannot-hide-observed-formals',lambda s:None,'actual frontier formal-argument inventory differs',empty_contract)
    # Run the real GDB observer with a deliberately incomplete contract. Merely
    # comparing two copied metadata lists cannot satisfy this control.
    actual_gdb_negatives=[]
    def frontier(configuration):return next(row for row in configuration['functions'] if row['name']=='read_module_global_types')
    actual_cases=[
        ('omitted-formals',empty_contract,'incomplete frontier ABI contract'),
        ('integer-as-pointer',lambda configuration:frontier(configuration)['parameters'][0].update(kind='global-pointer'),'pointer ABI formal is not an actual pointer'),
        ('integer-width',lambda configuration:frontier(configuration)['parameters'][0].update(native_width=2),'actual frontier formal width differs'),
        ('directory-pointee',lambda configuration:frontier(configuration)['parameters'][1].update(element_size=8),'actual pointer formal pointee extent differs'),
    ]
    for name,mutation,expected_guard in actual_cases:
        with tempfile.TemporaryDirectory(prefix='cvpack-formal-observer-',dir=Path.home()/'.ai-tmp') as temporary:
            directory=Path(temporary);configuration=copy.deepcopy(config);mutation(configuration)
            bad=directory/'bad-config.json';bad.write_text(json.dumps(configuration))
            output=directory/'native'
            command=[sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
                '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)]
            result=subprocess.run(command,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
            diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
            assert result.returncode and expected_guard in diagnostic, diagnostic
            actual_gdb_negatives.append(dict(name=name,returncode=result.returncode,bad_config_sha256=sha(bad),
                diagnostic=diagnostic,script_sha256=sha(output/'gdb-state.py')))
    # The public allocation has an initialized 116-byte prefix from its first
    # read and a newer 92-byte read; bytes 92..115 remain defined, not malloc tail.
    public=root(native,'symbol_data');state=copy.deepcopy(native)
    raw=bytearray.fromhex(state['memory'][str(public)])
    assert len(raw)==1126
    raw[116:]=bytes(value^0xff for value in raw[116:]);state['memory'][str(public)]=raw.hex()
    compare(state)
    report=dict(schema=1,native_only=True,rejected=rejected,count=len(rejected)+len(actual_gdb_negatives),
        actual_gdb_contract_rejections=actual_gdb_negatives,
        native_owned_public_tail_change_allowed=True,formal_types=native['observed_argument_types'],
        reads_checked=baseline['reads_checked'],checker_sha256=sha(Path(__file__)),
        decoder_sha256=sha(BINARY/'scripts/raw-state.py'),gdb_observer_sha256=sha(BINARY/'scripts/gdb-state.py'),
        original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native-formal-types/receipt.json'),
        seconds=time.perf_counter()-start)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(rejected=report['count'],actual_gdb_guard=True,native_owned_public_tail_change_allowed=True,seconds=report['seconds']),indent=2))


if __name__=='__main__':main()
