#!/usr/bin/env python3
"""Mutate fresh native clock/child/state records, restoring every raw byte."""
import argparse,copy,json,time
from pathlib import Path
from clock_provider import verify

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('--stop-slice',action='store_true');args=parser.parse_args()
directory=args.directory.resolve();started=time.perf_counter();passed=[];verify(directory)
def denied(label,name,change):
    path=directory/name;raw=path.read_bytes()
    try:
        value=json.loads(raw);change(value);path.write_text(json.dumps(value,indent=2)+'\n')
        try:verify(directory)
        except(AssertionError,KeyError,ValueError,IndexError,StopIteration):passed.append(label)
        else:raise AssertionError('accepted changed native evidence: '+label)
    finally:path.write_bytes(raw)
def addressed(value,address):
    changed=0
    for row in value['ranges']:
        raw=bytearray.fromhex(row['data']);offset=address-row['address']
        if 0<=offset<len(raw):raw[offset]^=1;row['data']=raw.hex();changed+=1
    assert changed
clock=json.loads((directory/'native-clock-gdb.json').read_bytes());count=len(clock['calls'])
if args.stop_slice:assert count==2
for index in ([1]if args.stop_slice else range(count)):
    for when in('before','after'):
        for name in('errno','doserrno','last_error','counter','timezone','daylight','started'):
            def corrupt(value,index=index,when=when,name=name):
                point=value['calls'][index][when];addressed(point,point['symbols'][name])
            denied('clock%d %s %s'%(index+1,when,name),'native-clock-gdb.json',corrupt)
        for field,offset in [('time',0),('millitm',4),('zone',6),('DST',8),('padding',10)]:
            def corrupt(value,index=index,when=when,offset=offset):
                point=value['calls'][index][when];addressed(point,point['pointer']+offset)
            denied('clock%d %s %s'%(index+1,when,field),'native-clock-gdb.json',corrupt)
    denied('clock%d formal alias'%(index+1),'native-clock-gdb.json',lambda v,index=index:v['calls'][index].update(argument_slot_hex='00000000'))
    denied('clock%d caller'%(index+1),'native-clock-gdb.json',lambda v,index=index:v['calls'][index].update(caller='main'))
    for register in('fctrl','fstat','ftag','st0'):
        def corrupt(value,index=index,register=register):
            raw=bytearray.fromhex(value['calls'][index]['after']['x87_registers'][register]);raw[0]^=1
            value['calls'][index]['after']['x87_registers'][register]=raw.hex()
        denied('clock%d x87 %s'%(index+1,register),'native-clock-gdb.json',corrupt)
denied('missing clock call','native-clock-gdb.json',lambda v:v['calls'].clear())
denied('extra clock call','native-clock-gdb.json',lambda v:v['calls'].append(copy.deepcopy(v['calls'][0])))
for phase in(()if args.stop_slice else('link-before','link-after')):
    filename='native-'+phase+'-gdb.json'
    for symbol in('errno','doserrno','last_error'):
        denied(phase+' '+symbol,filename,lambda v,symbol=symbol:addressed(v,v['raw_context']['symbols'][symbol]))
for symbol in(()if args.stop_slice else('mb_codepage','updates','environment','initenv','os_environment','active_pass','pass_running','mre_calls')):
    denied('child state '+symbol,'native-link-after-gdb.json',lambda v,symbol=symbol:addressed(v,v['raw_context']['symbols'][symbol]))
for field in(()if args.stop_slice else('mode','file','argv')):
    denied('spawn formal '+field,'native-link-before-gdb.json',lambda v,field=field:addressed(v,v['raw_context']['spawn_formals'][field]))
for field in(()if args.stop_slice else('mode','pass','name','environment','args')):
    denied('caller formal '+field,'native-link-before-gdb.json',lambda v,field=field:addressed(v,v['raw_context']['caller_formals'][field]))
for label,change in[
    ('missing child',lambda v:v['requests'].clear()),
    ('extra child',lambda v:v['requests'].append(copy.deepcopy(v['requests'][0]))),
    ('wire profile',lambda v:v['wire'][0].update(state_profile=False)),
    ('child status',lambda v:v['requests'][0].update(status=2)),
    ('parent effect',lambda v:v['requests'][0]['parent_state_after'].update(last_error=2)),
    ('output effect',lambda v:v['effects'][0]['output'].update(sha256='0'*64)),
    ('response identity',lambda v:v.update(response_sha256='0'*64)),
    ('cached as fresh',lambda v:v.update(cached_fixture=True))]:denied(label,'clock-native.json',change)
denied('different trial','config.json',lambda v:v['accepted'].remove('_start_timer'))
for key,value in [('TZ','UTC0'),('clock_input','2020-01-01T00:00:00.000/0'),('expected_calls',99)]:
    denied('host input '+key,'clock-host-inputs.json',lambda v,key=key,value=value:v.__setitem__(key,value))
denied('host zonefile','clock-host-inputs.json',lambda v:v['zonefile'].update(sha256='0'*64))
if count==1:
    for index in(1,2):
        denied('frontier descriptor alias '+str(index),'streams-frontier.json',lambda v,index=index:v['win32'][index].update(handle=v['win32'][index]['handle']+1))
    denied('frontier errno','streams-frontier.json',lambda v:v['errors'].update(errno=2))
    path=directory/'streams-frontier-iob.bin';raw=path.read_bytes()
    try:
        changed=bytearray(raw);changed[16]^=1;path.write_bytes(changed)
        try:verify(directory)
        except AssertionError:passed.append('frontier FILE word')
        else:raise AssertionError('accepted changed frontier FILE word')
    finally:path.write_bytes(raw)
verify(directory)
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed}))
