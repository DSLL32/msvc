#!/usr/bin/env python3
"""Host-only negative controls over frozen IDE state and protocol evidence."""
import copy,hashlib,json,struct,time
from pathlib import Path
from ide_protocol_abi import compare_history
from importlib.util import spec_from_file_location,module_from_spec
ROOT=Path(__file__).resolve().parents[1];CASE=ROOT/'cases/ide-callback-protocol'
spec=spec_from_file_location('ide_fixture',ROOT/'scripts/check-ide-protocol.py');fixture=module_from_spec(spec);spec.loader.exec_module(fixture)
def load():
    native=json.loads((CASE/'acceptance/native-final/state.json').read_text())
    original=[json.loads(x) for x in (CASE/'acceptance/read_ide_flags/original-complete/events.jsonl').read_text().splitlines()]
    exports=json.loads((CASE/'analysis/compiled-dependency.json').read_text())['callbacks']
    return native,original,exports

def main():
    start=time.perf_counter();native,original,exports=load();checks=compare_history(native,original,exports)
    (CASE/'acceptance/native-final/typed-validation.json').write_text(json.dumps(dict(status='pass',checks=checks,scope='Actual formals/ownership, mandatory callback inventory, defined state, caller/return, cdecl stack/nonvolatiles and returned CRT environment alias; external dependencies not app vertices'),indent=2)+'\n')
    passed=[]
    def reject(name,action):
        try:action()
        except (AssertionError,KeyError,IndexError,struct.error):passed.append(name);return
        raise AssertionError('negative accepted: '+name)
    def mutate(name,selector,field,value):
        n=copy.deepcopy(native);event=next(x for x in n['events'] if selector(x));event[field]=value
        reject(name,lambda:compare_history(n,original,exports))
    entry=lambda e:e['event']=='target_entry' and e['phase']==2
    write=lambda e:e['event']=='write_entry' and e['phase']==4
    returned=lambda e:e['event']=='target_return' and e['phase']==10
    for name,field,value in [('missing-formals','arguments',[]),('wrong-API-formal','arguments',[native['events'][0]['arguments'][0],1]),('wrong-errors','errors',[1,0,0]),('wrong-locale','ctype','00'),('missing-output-guards','out_slot','')]:mutate(name,entry,field,value)
    for off,label in [(0,'wrong-getter-symbol'),(4,'wrong-writer-symbol'),(8,'changed-reserved-word'),(12,'wrong-assertion-symbol'),(16,'wrong-owned-buffer'),(20,'wrong-capacity'),(56,'changed-buffer-byte')]:
        n=copy.deepcopy(native);data=bytearray.fromhex(n['events'][0]['probe']);data[off]^=1;n['events'][0]['probe']=data.hex();reject(label,lambda:compare_history(n,original,exports))
    mutate('wrong-string-owner',lambda e:e['event']=='target_entry' and e['phase']==4,'arguments',[1,1,1])
    mutate('wrong-callback-caller',write,'actual_target_caller','read_ide_flags')
    mutate('wrong-callback-channel',write,'arguments',[1,8,1,8])
    mutate('wrong-return-association',returned,'paired_entry_phase',9)
    mutate('wrong-defined-return',returned,'eax',0)
    mutate('wrong-stack-cleanup',returned,'esp',0)
    mutate('nonvolatile-clobber',returned,'ebx',0)
    mutate('wrong-environment-owner',returned,'environment',dict(pointer=1,owned=False,value=''))
    mutate('wrong-initializer-channel',lambda e:e['event']=='channel_entry','arguments',[1,3])
    n=copy.deepcopy(native);n['events']=[];reject('empty-native-inventory',lambda:compare_history(n,original,exports))
    reject('empty-original-inventory',lambda:compare_history(native,[],exports))
    n=copy.deepcopy(native);n['events'].pop(5);reject('omitted-callback',lambda:compare_history(n,original,exports))
    n=copy.deepcopy(native);n['terminal']=[];reject('missing-terminal',lambda:compare_history(n,original,exports))
    for off,label in [(0,'protocol-header'),(32,'sink-channel'),(36,'sink-count'),(52,'sink-order'),(172,'sink-bytes'),(249,'buffer-guard'),(281,'defined-buffer'),(1337,'getter-return'),(1373,'environment-value')]:
        data=bytearray((CASE/'baseline/1/protocol.bin').read_bytes());data[off]^=1;reject(label,lambda:fixture.validate_protocol(data))
    reject('empty-trace',lambda:fixture.validate_trace([],['initialize_ide_api','emit_ide_message','read_ide_flags']))
    reject('both-empty-trace',lambda:fixture.validate_trace([],[]))
    receipt=dict(status='pass',seconds=time.perf_counter()-start,positive_packets=len(checks),negative_count=len(passed),rejected=passed,execution='host-only; no target, Wine, original or debugger execution')
    (CASE/'negative-controls.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
