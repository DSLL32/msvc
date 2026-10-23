#!/usr/bin/env python3
"""Host-only mutation checks; uses frozen actual packets, launches no processes."""
from pathlib import Path
import copy,importlib.util,json,subprocess,tempfile,time
from legacy_diagnostic_abi import validate
HERE=Path(__file__).resolve().parents[1];CASE=HERE/'cases/legacy-resource-diagnostics'
def main():
    start=time.perf_counter();tests=[]
    original=[json.loads(x) for x in (CASE/'baseline/strings/1/events.jsonl').read_text().splitlines()]
    actual=json.loads((CASE/'acceptance/strings/native-final/state.json').read_text())
    payload=(CASE/'inputs/strings.bin').read_bytes();validate(actual,original,payload,'strings',True)
    def reject(name,change):
        packet=copy.deepcopy(actual);source=copy.deepcopy(original);change(packet,source)
        try:validate(packet,source,payload,'strings',True)
        except (AssertionError,KeyError,StopIteration,ValueError):tests.append(name)
        else:raise AssertionError('invalid state accepted: '+name)
    reject('both formal inventories omitted',lambda p,o:(p.update(entry={}),o.clear()))
    reject('wrong payload formal',lambda p,o:p['entry']['arguments'].__setitem__(1,0))
    reject('wrong size formal',lambda p,o:p['entry']['arguments'].__setitem__(2,48))
    reject('wrong input bytes',lambda p,o:p['entry'].update(payload='00'*49))
    reject('guard damage',lambda p,o:p['return'].update(guards='00'*64))
    reject('C table TAB discrepancy',lambda p,o:p['entry'].update(ctype=p['entry']['ctype'][:40]+'6800'+p['entry']['ctype'][44:]))
    reject('missing error triple',lambda p,o:p['entry'].update(errors=[]))
    reject('wrong live errno',lambda p,o:p['entry']['errors'].__setitem__(0,2))
    reject('wrong FILE flags',lambda p,o:p['return']['stdout_file'].__setitem__(3,2))
    reject('wrong pending stdout',lambda p,o:p['return'].update(buffered='00'))
    reject('wrong stack cleanup',lambda p,o:p['return'].update(esp=p['return']['esp']+8))
    reject('callee-saved EBP changed',lambda p,o:p['return']['registers'].update(ebp=0))
    reject('wrong original payload owner',lambda p,o:o[0].update(input=0))
    spec=importlib.util.spec_from_file_location('callback_checker',HERE/'scripts/check-legacy-diagnostics.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    item=json.loads((CASE/'manifest.json').read_text())['cases'][0]
    for name,entries,record in [('missing callback entry',[],item),('wrong callback entry',['display_legacy_menu'],item),('both trace inventories empty',[],dict(item,expected_application_entries=[]))]:
        try:module.validate_trace(entries,record)
        except AssertionError:tests.append(name)
        else:raise AssertionError(name)
    with tempfile.TemporaryDirectory() as directory:
        work=Path(directory);good=subprocess.CompletedProcess([],0,(CASE/item['stdout']['path']).read_bytes(),b'')
        module.validate_case(work,good,item,good.stdout,b'',payload)
        for name,result in [('wrong driver exit',subprocess.CompletedProcess([],1,good.stdout,b'')),('wrong stdout',subprocess.CompletedProcess([],0,b'',b'')),('unexpected stderr',subprocess.CompletedProcess([],0,good.stdout,b'bad'))]:
            try:module.validate_case(work,result,item,good.stdout,b'',payload)
            except AssertionError:tests.append(name)
            else:raise AssertionError(name)
        (work/'unexpected').write_bytes(b'')
        try:module.validate_case(work,good,item,good.stdout,b'',payload)
        except AssertionError:tests.append('unexpected file effect')
        else:raise AssertionError('file inventory')
    receipt=dict(status='pass',tests=tests,count=len(tests),seconds=time.perf_counter()-start,scope='Host-only actual-packet mutations. No fixture/original/debugger execution.')
    (CASE/'negative-validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(receipt['count'],'controls pass')
if __name__=='__main__':main()
