#!/usr/bin/env python3
"""Check actual masked response/native effect proof and affected negatives.

Mutations are applied to this worker's capture files and immediately restored.
No reconstruction, reference executable, or subprocess is run by these checks.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import time

from dependency_provider import verify, encoded
from dependency_response import decode as original_decode
from dependency_native import decode as request_decode
from check_dependency_payload import payload
from frontier_state import decode as frontier_decode, snapshot
from dependency_wire import string


def run(original,native):
    started=time.perf_counter()
    response=original_decode(original)
    actual=verify(native)
    expected=frontier_decode(original,'original')['fields']
    assert frontier_decode(native,'native')['fields']==expected
    assert expected['frontier']==response['next_frontier'] and len(actual['outputs'])==4
    request_path=native/'dependency-native/request-2-2.bin'
    attempted=native/'dependency-native/attempt-2-2.bin'
    receipt_path=native/'dependency-native.json'
    passed=[]
    def negative(label,changes):
        originals={path:(path.read_bytes() if path.exists() else None)for path in changes}
        try:
            for path,change in changes.items():
                value=change(originals[path])
                if value is None:path.unlink()
                else:path.write_bytes(value)
            try:verify(native)
            except (AssertionError,ValueError,KeyError,FileNotFoundError,struct.error):passed.append(label)
            else:raise AssertionError('native effect proof accepted mutation: '+label)
        finally:
            for path,value in originals.items():
                if value is None:
                    if path.exists():path.unlink()
                else:path.write_bytes(value)
    def request_negative(label,change):
        fields=request_decode(request_path.read_bytes());change(fields);value=payload(fields)
        def receipt_change(data):
            result=json.loads(data)
            item={'kind':2,'pid':result['pid'],'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()}
            result['requests'][2]=item;result['attempts'][2]=dict(item)
            return encoded(result)
        negative(label,{request_path:lambda unused:value,attempted:lambda unused:value,receipt_path:receipt_change})
    def original_negative(label,changes,receipt_change=None):
        receipt=original/'capture.json';before=receipt.read_bytes()
        originals={name:(original/name).read_bytes()for name in changes}
        try:
            capture=json.loads(before)
            for name,change in changes.items():
                value=change(originals[name]);(original/name).write_bytes(value)
                capture['files'][name]={'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()}
            if receipt_change:receipt_change(capture)
            receipt.write_bytes(encoded(capture))
            try:original_decode(original)
            except (AssertionError,ValueError,KeyError,FileNotFoundError,struct.error):passed.append(label)
            else:raise AssertionError('masked response accepted mutation: '+label)
        finally:
            for name,value in originals.items():(original/name).write_bytes(value)
            receipt.write_bytes(before)
    def integer(value,offset,number):
        changed=bytearray(value);struct.pack_into('<I',changed,offset,number);return bytes(changed)
    original_negative('analysis cannot authorize native response',{},lambda c:c.update(baseline=True,dependency_analysis=True))
    original_negative('response controller must use masked launch',{},lambda c:c['command'].__setitem__(-2,'0'))
    original_negative('different actual loaded C1',{'observer/loaded-c1.bin':lambda b:b[:-1]+bytes([b[-1]^1])})
    original_negative('actual original BOOL differs',{'observer/invoke-before-stack.bin':lambda b:integer(b,8,0)})
    original_negative('actual original execution globals differ',{'observer/invoke-before-execution-words.bin':lambda b:integer(b,0,1)})
    original_negative('actual original argument bytes differ',{'observer/invoke-before-caller-argv.bin':lambda b:b.replace(b'test.c',b'jest.c')})
    frontier_address={'unload_pass':0x4054ec,'cleanup_files':0x404ed1,'delete_temporary':0x404f6f,'next_jobs':0x403709}[response['next_frontier']]
    original_negative('actual original next frontier differs',{'events.txt':lambda b:b.replace(('address=%08x'%frontier_address).encode(),('address=%08x'%(frontier_address+1)).encode())})
    original_negative('actual original IR copies disagree',{'observer/c1-output/'+next(iter(response['outputs'])):lambda b:b+b'x'})
    original_negative('unobserved extra Invoke',{'observer/events.txt':lambda b:b+b'invoke-call argc=26 argv=00000001 last=1 function=10612229 esp=0031f574\n'})
    original_negative('wrong controlled stop status',{'events.txt':lambda b:b.replace(b'status=28673',b'status=0')})
    if 'child' in response:
        original_negative('actual backend wait mode differs',{'observer/spawn-before-stack.bin':lambda b:integer(b,0,1)})
        original_negative('actual backend argv pointer differs',{'observer/spawn-before-stack.bin':lambda b:integer(b,8,1)})
        original_negative('actual backend executable differs',{'observer/spawn-executable.bin':lambda b:b[:-1]+bytes([b[-1]^1])})
        original_negative('actual backend output copy differs',{'observer/c2-output/test.obj':lambda b:b+b'x'})
        original_negative('actual backend DOS error effect omitted',{'mktemp-spawn-after-ptd.bin':lambda b:integer(b,12,2)})
        original_negative('backend assignment trailing space omitted',{'observer/putenv-2-before-assignment.bin':lambda b:b[:-2]+b'\0'})
        original_negative('backend insertion loses old string identity',{'observer/putenv-2-after-crt-environment.bin':lambda b:integer(b,8,1)})
    if response['next_frontier']=='delete_temporary':
        original_negative('actual helper suffix index differs',{'observer/frontier-stack.bin':lambda b:integer(b,4,1)})
    request_negative('actual BOOL argument changed',lambda f:f.update(last=0))
    request_negative('actual frontend input changed',lambda f:f['argv']['strings'].__setitem__(4,'other.c'))
    request_negative('actual argv pointer alias changed',lambda f:f['argv']['pointers'].__setitem__(1,1))
    request_negative('actual error input changed',lambda f:f.update(last_error=0))
    request_negative('actual FP control changed',lambda f:f.update(controlfp=0))
    request_negative('actual target current-pass alias changed',lambda f:f.update(pass_is_current=0))
    request_negative('actual cached export changed',lambda f:f.update(invoke_is_c1=0))
    request_negative('actual loader lease changed',lambda f:f.update(provider_state=5))
    request_negative('actual environment entries reordered',lambda f:f['environment']['strings'].reverse())
    request_negative('actual initial string was not copied',lambda f:f['environment']['pointers'].__setitem__(0,f['initial_environment']['pointers'][0]))
    request_negative('actual OS namespace changed',lambda f:f['os_environment'].update(bytes='00000000'))
    request_negative('actual FILE storage alias changed',lambda f:f['file_words'].__setitem__(0,1))
    request_negative('actual extra open descriptor',lambda f:f['open_descriptors'].append(3))
    artifact=native/next(iter(response['outputs']))
    if 'deletions'in response:
        negative('temporary IR recreated after cleanup',{artifact:lambda b:b'leftover'})
    else:
        negative('replayed IR byte differs',{artifact:lambda b:b+ b'x'})
        negative('replayed IR missing',{artifact:lambda b:None})
    prefix=response['invoke_argv'][2][2:]
    negative('extra intermediate artifact',{native/(prefix+'unknown'):lambda b:b'extra'})
    negative('C2 output differs'if'child'in response else'C2 ran before frontier',{native/'test.obj':lambda b:b'unexpected'})
    negative('source changed after callback',{native/'test.c':lambda b:b+'x'.encode()})
    negative('actual configuration changed',{native/'dependency-config.bin':lambda b:b[:-1]+bytes([b[-1]^1])})
    def missing(data):
        result=json.loads(data);result['requests'].pop();result['attempts'].pop();return encoded(result)
    negative('missing actual Invoke inventory',{receipt_path:missing})
    def changed_pid(data):
        result=json.loads(data);result['pid']+=1
        for name in ('requests','attempts'):
            for item in result[name]:item['pid']=result['pid']
        return encoded(result)
    process=native/'native-process.json'
    negative('mutually changed PID metadata',{receipt_path:changed_pid,
        process:lambda b:encoded({'pid':json.loads(b)['pid']+1})})
    if 'child' in response:
        spawn_receipt=native/'spawn-native.json'
        spawn_payload=native/'dependency-native/spawn-0.bin'
        for name in ('MSVCRT.DLL','MSPDB50.DLL'):
            negative('native backend provider bytes differ: '+name,
                     {native/name:lambda b:b[:-1]+bytes([b[-1]^1])})
        def spawn_negative(label,change):
            old=json.loads(spawn_receipt.read_bytes());request=copy.deepcopy(old['observations'][0]);change(request)
            parent=request['parent_state'];environment=request['environment']
            value=struct.pack('<iIII',request['mode'],request['pid'],len(request['argv']),len(environment))
            value+=struct.pack('<4I',*(parent[name]for name in ('errno','doserrno','last_error')),
                               2 if request['environment_source']=='target-os'else 0)
            value+=string(request['file'])+string(request['cwd'])
            value+=b''.join(string(text)for text in [*request['argv'],*environment])
            old['payloads'][0]={'profile':True,'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()}
            old['observations'][0]=request
            old['requests'][0]={**{key:data for key,data in request.items()if key!='environment'},
                'action':'replay','status':0,'parent_state_after':response['child']['after']}
            negative(label,{spawn_payload:lambda unused:value,spawn_receipt:lambda unused:encoded(old)})
        spawn_negative('native child executable differs',lambda r:r.update(file=r['file']+'x'))
        spawn_negative('native child command differs',lambda r:r['argv'].append('-unexpected'))
        spawn_negative('native child namespace order differs',lambda r:r['environment'].reverse())
        spawn_negative('native child command flags changed',lambda r:r['environment'].__setitem__(-1,r['environment'][-1][:-1]))
        spawn_negative('native child used host namespace',lambda r:r.update(environment_source='host'))
        spawn_negative('native child parent error differs',lambda r:r['parent_state'].update(doserrno=0))
        negative('native backend object missing',{native/'test.obj':lambda b:None})
        def post_error(data):
            value=json.loads(data);value['requests'][0]['parent_state_after']['doserrno']=2;return encoded(value)
        negative('native child DOS error transition omitted',{spawn_receipt:post_error})
    if 'deletions'in response:
        removed=native/'native-unlink-gdb.json'
        def removal_negative(label,change):
            def mutate(value):
                fields=json.loads(value);change(fields);return encoded(fields)
            negative(label,{removed:mutate})
        removal_negative('actual removal filename differs',lambda r:r['calls'][0].update(filename='other'))
        removal_negative('actual removal input presence differs',lambda r:r['calls'][0].update(present_before=False))
        removal_negative('actual removal local-buffer alias differs',lambda r:r['calls'][0].update(argument_is_local_buffer=False))
        removal_negative('actual removal status differs',lambda r:r['calls'][0].update(result=-1))
        removal_negative('actual missing-file DOS transition omitted',lambda r:r['calls'][4]['after'].update(doserrno=0))
        removal_negative('actual removal output remains',lambda r:r['calls'][0].update(present_after=True))
        removal_negative('actual removal inventory missing',lambda r:r['calls'].pop())
        removal_negative('actual removal PID differs',lambda r:r.update(pid=r['pid']+1))
        original_negative('original removal filename differs',{'observer/unlink-1-before-filename.bin':lambda b:b.replace(b'ex',b'ey')})
        original_negative('original removal formal alias differs',{'observer/unlink-1-before-stack.bin':lambda b:integer(b,0,1)})
        original_negative('original missing-file DOS effect omitted',{'mktemp-unlink-5-after-ptd.bin':lambda b:integer(b,12,0)})
        original_negative('original next_jobs Driver formal differs',{'context.bin':lambda b:integer(b,208,1)})
        original_negative('original next_jobs pending caller alias differs',{'context.bin':lambda b:integer(b,212,1)})
    # Replay the independent GDB memory transcript against altered target bytes.
    raw=json.loads((native/'frontier-memory.json').read_text());backing={}
    for item in raw['ranges']:
        for index,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+index
            assert address not in backing or backing[address]==byte
            backing[address]=byte
    def memory_negative(label,address,value):
        changed=backing.copy()
        for index,byte in enumerate(value):changed[address+index]=byte
        try:fields=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),raw['context'])
        except (AssertionError,KeyError):passed.append(label);return
        assert fields!=expected,'independent GDB decoder accepted changed state: '+label
        passed.append(label)
    context=raw['context'];crt=context['crt_addresses'];module=context['compiler_passes_address']+36
    memory_negative('post-Invoke cached module differs',module+20,struct.pack('<I',context['compiler_module']['module'] if 'child' in response else 0))
    memory_negative('post-Invoke active pass differs',context['execution_addresses']['active_pass'],struct.pack('<I',0))
    memory_negative('post-Invoke codepage reset',crt['mb_codepage'],struct.pack('<I',0))
    memory_negative('post-Invoke ctype byte differs',struct.unpack('<I',bytes(backing[crt['pctype_slot']+i]for i in range(4)))[0],b'\xff')
    if response['next_frontier']=='delete_temporary':
        table=context['temporary_suffixes_address']
        memory_negative('actual suffix role differs',table+4,struct.pack('<I',0))
        pointer=struct.unpack('<I',bytes(backing[table+i]for i in range(4)))[0]
        memory_negative('actual suffix literal differs',pointer,b'x')
        memory_negative('actual suffix sentinel differs',table+56,struct.pack('<I',1))
        changed=copy.deepcopy(context);changed['arguments']=[1]
        try:snapshot(lambda a,n:bytes(backing[a+i]for i in range(n)),changed)
        except AssertionError:passed.append('actual helper formal differs')
        else:raise AssertionError('helper formal mutation was accepted')
    if response['next_frontier']=='next_jobs':
        memory_negative('next_jobs minimal-rebuild guard differs',context['globals']['minimal_rebuild'],struct.pack('<I',1))
        changed=copy.deepcopy(context);changed['arguments'][1]+=4
        try:snapshot(lambda a,n:bytes(backing[a+i]for i in range(n)),changed)
        except AssertionError:passed.append('actual next_jobs caller-local pending alias differs')
        else:raise AssertionError('next_jobs caller-local alias mutation was accepted')
    assert verify(native)==actual,'mutations were not fully restored'
    print(json.dumps({'result':'PASS','actual_next_frontier':response['next_frontier'],'IR_files':4,
                      'negatives':len(passed),'seconds':time.perf_counter()-started}))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
    args=parser.parse_args();run(args.original,args.native)
