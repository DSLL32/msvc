#!/usr/bin/env python3
"""Compiler lifetime normal/action/handler ABI residuals; Rust owns CFGs."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path


def contextual_bytes(image, address, count):
    output = subprocess.check_output([
        'objdump', '-s', '--start-address='+hex(address),
        '--stop-address='+hex(address+count), str(image)], text=True)
    result = bytearray()
    for line in output.splitlines():
        match = re.match(r'^\s*[0-9a-f]+\s+((?:[0-9a-f]{2,8}\s+){1,4})', line)
        if match:
            result.extend(bytes.fromhex(match[1]))
    assert len(result) == count, 'bounded contextual read must be exact'
    return bytes(result)


def verify(report, images):
    facts = report['decoded_instructions']
    for side, image in images.items():
        verify_owner(facts, side, image, '_process_module_symbols@24',
                     'ScopedBuffer::~ScopedBuffer', 24, 0x650, -0x630, 0)
        verify_owner(facts, side, image, '_process_definition_file@8',
                     'DefinitionBuffer::~DefinitionBuffer', 8,
                     0x79c if side == 'original' else 0x7ac,
                     -0x7a0 if side == 'original' else -0x7b0, 1)

        verify_owner(facts, side, image, 'LinkContext::PrintFoldedComdats',
                     'ScopedBuffer::~ScopedBuffer', 0,
                     0x238 if side == 'original' else 0x23c, -0x220, 0)
        verify_import_conversion(facts, side, image)
        verify_converted_member(facts, side, image)
        verify_dumper_file(facts, side, image)
        verify_dumper_file(facts, side, image, members=True)


def verify_owner(facts, side, image, name, cleanup_name, argument_bytes,
                 frame_bytes, local_offset, normal_cleanup_calls):
    identity = facts['inputs']['images'][side]
    assert identity['original_file_sha256'] == hashlib.sha256(image.read_bytes()).hexdigest()
    owner = facts['functions'][name][side]
    destructor = facts['functions'][cleanup_name][side]
    start, end = owner['record']['start'], owner['record']['end']
    chunks = owner['record']['code_chunks']
    assert len(chunks) == 2
    (action, action_end), (handler, handler_end) = chunks
    assert action_end == handler and action_end-action == 11 and handler_end-handler == 10
    assert set(owner['effective_roots']) == {start, action, handler}
    assert owner['checked_ret_cleanup_set'] is None
    assert owner['local_ret_cleanup_set'] == [argument_bytes]
    assert destructor['checked_ret_cleanup_set'] == [0]
    instructions = {row['address']: bytes.fromhex(row['bytes_hex']) for row in owner['instructions']}
    memory = {}
    for address, data in instructions.items():
        for offset, byte in enumerate(data):
            assert address+offset not in memory
            memory[address+offset] = byte
    def code(address, count):
        return bytes(memory[address+i] for i in range(count))
    # The only normal return restores FS registration, all saved registers
    # and the local frame, then consumes the physical DWORD arguments.
    normal_returns = [(at, data) for at,data in instructions.items()
                      if start <= at < end and data[:1] in (b'\xc2', b'\xc3')]
    ret_bytes = b'\xc3' if argument_bytes == 0 else b'\xc2'+struct.pack('<H',argument_bytes)
    assert len(normal_returns) == 1 and normal_returns[0][1] == ret_bytes
    epilogue = (bytes.fromhex('8b8c24')+struct.pack('<I',frame_bytes+16)+
                bytes.fromhex('5f5e5d64890d000000005b81c4')+
                struct.pack('<I',frame_bytes+12)+ret_bytes)
    ret = normal_returns[0][0]
    assert code(ret+len(ret_bytes)-len(epilogue), len(epilogue)) == epilogue
    prologue = code(start, 27)
    assert prologue[:3] == bytes.fromhex('6aff68')
    assert struct.unpack_from('<I', prologue, 3)[0] == handler
    assert prologue[7:] == bytes.fromhex('64a100000000506489250000000081ec')+struct.pack('<I',frame_bytes)
    # Action has no stack mutation: it establishes this at the actual 12-byte
    # or 20-byte local and tail-enters the independently checked thiscall RET0 body.
    action_bytes = code(action, 11)
    assert action_bytes[:7] == bytes.fromhex('8d8d')+struct.pack('<i',local_offset)+b'\xe9'
    assert action+11+struct.unpack_from('<i', action_bytes, 7)[0] == destructor['record']['start']
    cleanup_calls = [at for at,data in instructions.items()
                     if start <= at < end and data[:1] == b'\xe8'
                     and at+5+struct.unpack_from('<i',data,1)[0] == destructor['record']['start']]
    assert len(cleanup_calls) == normal_cleanup_calls
    if argument_bytes == 0:
        # This owner's normal cleanup inlines the same conditional scalar delete;
        # only its exceptional action enters the canonical 12-byte destructor.
        delete_calls=[]
        for at,data in instructions.items():
            if start <= at < end and data[:1] == b'\xe8':
                target=at+5+struct.unpack_from('<i',data,1)[0]
                if contextual_bytes(image,target,2)==b'\xff\x25':
                    if qualified_import(image,identity,target)==[('msvcrt.dll','??3@YAXPAX@Z')]:
                        delete_calls.append(at)
        assert len(delete_calls)==1
    if argument_bytes == 8:
        # Exactly five DWORD stores construct the actual 20-byte local before
        # Grow and EH lifetime registration. This is the same stack owner
        # addressed by the exceptional action, not a surrogate guard.
        stores = b''.join(bytes.fromhex('895c24')+bytes([offset])
                          for offset in (0x18,0x1c,0x20,0x24,0x28))
        assert contextual_bytes(image,start,min(end-start,128)).count(stores) == 1
        # Success calls the selected outline; the early path retains the
        # actual inline delete followed by Set(NULL,NULL), also in Rust CFG.
        setter = facts['functions']['DefinitionBuffer::Set'][side]['record']['start']
        early_sets = [at for at,data in instructions.items()
                      if start <= at < start+0x120 and data[:1] == b'\xe8'
                      and at+5+struct.unpack_from('<i',data,1)[0] == setter]
        assert early_sets and max(early_sets) < min(cleanup_calls)
    handler_bytes = code(handler, 10)
    assert handler_bytes[0] == 0xb8 and handler_bytes[5] == 0xe9
    info = struct.unpack_from('<I', handler_bytes, 1)[0]
    thunk = handler+10+struct.unpack_from('<i', handler_bytes, 6)[0]
    # Genuine compiler FuncInfo has exactly one unwind state and no catch.
    words = struct.unpack('<8I', contextual_bytes(image, info, 32))
    assert words[:2] == (0x19930520, 1) and words[3:] == (0,0,0,0,0)
    assert struct.unpack('<2I', contextual_bytes(image, words[2], 8)) == (0xffffffff, action)
    if side == 'original':
        assert owner['record']['shared'][-1] == [info, words[2]+8]
    assert qualified_import(image,identity,thunk) == [('msvcrt.dll','__CxxFrameHandler')]


def qualified_import(image, identity, thunk):
    raw = contextual_bytes(image, thunk, 6)
    assert raw[:2] == bytes.fromhex('ff25')
    slot = struct.unpack_from('<I', raw, 2)[0]
    imports = subprocess.check_output(['llvm-readobj','--coff-imports',str(image)], text=True)
    found = []
    for block in re.findall(r'Import \{(.*?)\n\}', imports, re.S):
        module = re.search(r'Name: (\S+)', block).group(1)
        iat = int(re.search(r'ImportAddressTableRVA: (0x[0-9A-Fa-f]+)', block).group(1),16)
        names = re.findall(r'^  Symbol: (\S+) \(', block,re.M)
        for index,name in enumerate(names):
            if identity['preferred_base']+iat+4*index == slot:
                found.append((module.lower(),name))
    return found


def verify_import_conversion(facts, side, image):
    identity=facts['inputs']['images'][side]
    assert identity['original_file_sha256']==hashlib.sha256(image.read_bytes()).hexdigest()
    owner=facts['functions']['_get_import_conversion@28'][side]
    start,end=owner['record']['start'],owner['record']['end']
    (action,action_end),(handler,handler_end)=owner['record']['code_chunks']
    assert action_end==handler and handler-action==14 and handler_end-handler==10
    assert set(owner['effective_roots'])=={start,action,handler}
    assert owner['checked_ret_cleanup_set'] is None
    instructions={row['address']:bytes.fromhex(row['bytes_hex']) for row in owner['instructions']}
    memory={at+i:byte for at,data in instructions.items() for i,byte in enumerate(data)}
    def code(at,count):
        return bytes(memory[at+i] for i in range(count))
    returns=[(at,data) for at,data in instructions.items()
             if start<=at<end and data[:1] in (b'\xc2',b'\xc3')]
    assert len(returns)==1 and returns[0][1]==bytes.fromhex('c21c00')
    frame=0x424 if side=='original' else 0x428
    local=-0x420 if side=='original' else -0x418
    result_offset=0x14 if side=='original' else 0x10
    prologue=code(start,32)
    assert prologue[:9]==bytes.fromhex('64a1000000006aff68')
    assert struct.unpack_from('<I',prologue,9)[0]==handler
    assert prologue[13:15]==bytes.fromhex('50a0')
    assert prologue[19:28]==bytes.fromhex('6489250000000081ec')
    assert struct.unpack_from('<I',prologue,28)[0]==frame
    epilogue=(bytes.fromhex('5f5e8b8c24')+struct.pack('<I',frame+8)+
              bytes.fromhex('8b4424')+bytes([result_offset])+bytes.fromhex('5d64890d000000005b81c4')+
              struct.pack('<I',frame+12)+bytes.fromhex('c21c00'))
    assert code(returns[0][0]+3-len(epilogue),len(epilogue))==epilogue
    # The constructor-failure action deletes the exact saved allocation,
    # rather than invoking a destructor on a fully constructed object.
    a=code(action,14)
    assert a[:8]==bytes.fromhex('8b85')+struct.pack('<i',local)+bytes.fromhex('50e8')
    assert a[-2:]==bytes.fromhex('59c3')
    delete=action+12+struct.unpack_from('<i',a,8)[0]
    assert qualified_import(image,identity,delete)==[('msvcrt.dll','??3@YAXPAX@Z')]
    saved=frame+28+local
    assert any(start<=at<end and data==bytes.fromhex('897424')+bytes([saved])
               for at,data in instructions.items())
    h=code(handler,10)
    assert h[0]==0xb8 and h[5]==0xe9
    info=struct.unpack_from('<I',h,1)[0]
    words=struct.unpack('<8I',contextual_bytes(image,info,32))
    assert words[:2]==(0x19930520,1) and words[3:]==(0,0,0,0,0)
    assert struct.unpack('<2I',contextual_bytes(image,words[2],8))==(0xffffffff,action)
    if side=='original':assert owner['record']['shared']==[[info,words[2]+8]]
    thunk=handler+10+struct.unpack_from('<i',h,6)[0]
    assert qualified_import(image,identity,thunk)==[('msvcrt.dll','__CxxFrameHandler')]


def verify_converted_member(facts, side, image):
    identity=facts['inputs']['images'][side]
    assert identity['original_file_sha256']==hashlib.sha256(image.read_bytes()).hexdigest()
    owner=facts['functions']['_write_converted_archive_member@32'][side]
    destructor=facts['functions']['DefinitionBuffer::~DefinitionBuffer'][side]
    setter=facts['functions']['DefinitionBuffer::Set'][side]
    start,end=owner['record']['start'],owner['record']['end']
    (action,action_end),(handler,handler_end)=owner['record']['code_chunks']
    assert action_end==handler and handler-action==8 and handler_end-handler==10
    assert set(owner['effective_roots'])=={start,action,handler}
    assert owner['checked_ret_cleanup_set'] is None
    assert owner['local_ret_cleanup_set']==[32]
    assert destructor['checked_ret_cleanup_set']==[0]
    assert setter['checked_ret_cleanup_set']==[8]
    ins={row['address']:bytes.fromhex(row['bytes_hex']) for row in owner['instructions']}
    memory={at+i:byte for at,data in ins.items() for i,byte in enumerate(data)}
    def code(at,count):
        return bytes(memory[at+i] for i in range(count))
    rets=[at for at,data in ins.items() if start<=at<end and data[:1] in (b'\xc2',b'\xc3')]
    assert len(rets)==1 and ins[rets[0]]==bytes.fromhex('c22000')
    if side=='original':
        prologue=bytes.fromhex('6aff68')+struct.pack('<I',handler)+bytes.fromhex('64a100000000506489250000000083ec38')
        epilogue=bytes.fromhex('8b4c24485f5e5d64890d000000005b83c444c22000')
        stores=b''.join(bytes.fromhex('897c24')+bytes([x]) for x in (0x20,0x24,0x28,0x2c,0x30))
        local=0xcc
        delete_site=rets[0]-26
    else:
        prologue=bytes.fromhex('558bec6aff68')+struct.pack('<I',handler)+bytes.fromhex('64a100000000506489250000000083ec40')
        epilogue=bytes.fromhex('8b4df45f5e64890d000000005b8be55dc22000')
        stores=b''.join(bytes.fromhex('8945')+bytes([x]) for x in (0xc8,0xcc,0xd0,0xd4,0xd8))
        local=0xc8
        delete_site=rets[0]-24
    assert code(start,len(prologue))==prologue
    assert code(rets[0]+3-len(epilogue),len(epilogue))==epilogue
    body=code(start,end-start) if side=='original' else contextual_bytes(image,start,end-start)
    assert body.count(stores)==1
    a=code(action,8)
    assert a[:4]==bytes.fromhex('8d4d')+bytes([local])+b'\xe9'
    assert action+8+struct.unpack_from('<i',a,4)[0]==destructor['record']['start']
    calls=[at+5+struct.unpack_from('<i',data,1)[0] for at,data in ins.items()
           if start<=at<end and data[:1]==b'\xe8']
    assert destructor['record']['start'] not in calls
    assert calls.count(setter['record']['start'])==1
    # The normal path inlines this same owner's destruction to scalar delete;
    # the exceptional path alone enters the canonical destructor outline.
    delete=code(delete_site,5)
    assert delete[0]==0xe8 and code(delete_site-1,1)==b'\x50'
    assert qualified_import(image,identity,delete_site+5+struct.unpack_from('<i',delete,1)[0])==[('msvcrt.dll','??3@YAXPAX@Z')]
    h=code(handler,10)
    assert h[0]==0xb8 and h[5]==0xe9
    info=struct.unpack_from('<I',h,1)[0]
    words=struct.unpack('<8I',contextual_bytes(image,info,32))
    assert words[:2]==(0x19930520,1) and words[3:]==(0,0,0,0,0)
    assert struct.unpack('<2I',contextual_bytes(image,words[2],8))==(0xffffffff,action)
    if side=='original':assert [info,words[2]+8] in owner['record']['shared']
    thunk=handler+10+struct.unpack_from('<i',h,6)[0]
    assert qualified_import(image,identity,thunk)==[('msvcrt.dll','__CxxFrameHandler')]


def verify_dumper_file(facts, side, image, members=False):
    """Normal caller cleanup and failed-new delete action are distinct ABI roles."""
    identity = facts['inputs']['images'][side]
    assert identity['original_file_sha256'] == hashlib.sha256(image.read_bytes()).hexdigest()
    owner_name = '_dump_archive_members@8' if members else '_dump_file@4'
    owner = facts['functions'][owner_name][side]
    start, end = owner['record']['start'], owner['record']['end']
    (action, action_end), (handler, handler_end) = owner['record']['code_chunks']
    assert action_end == handler and handler-action == 11 and handler_end-handler == 10
    assert set(owner['effective_roots']) == {start, action, handler}
    assert owner['checked_ret_cleanup_set'] is None
    instructions = {row['address']: bytes.fromhex(row['bytes_hex'])
                    for row in owner['instructions']}
    memory = {at+i: byte for at, data in instructions.items() for i, byte in enumerate(data)}
    def code(at, count):
        return bytes(memory[at+i] for i in range(count))
    frame = 0x2c if members else 0x48 if side == 'original' else 0x50
    argument_bytes = 8 if members else 4
    ret_bytes = b'\xc2'+struct.pack('<H',argument_bytes)
    prologue = code(start, 24 if members else 29)
    assert prologue[:9] == bytes.fromhex('64a1000000006aff68')
    assert struct.unpack_from('<I', prologue, 9)[0] == handler
    if members:
        assert prologue[13:] == bytes.fromhex('506489250000000083ec')+bytes([frame])
    else:
        assert prologue[13:15] == bytes.fromhex('50a1')
        assert prologue[19:] == bytes.fromhex('6489250000000083ec')+bytes([frame])
    returns = [(at, data) for at, data in instructions.items()
               if start <= at < end and data[:1] in (b'\xc2', b'\xc3')]
    assert len(returns) == 1 and returns[0][1] == ret_bytes
    if members:
        epilogue = (bytes.fromhex('8b4c24')+bytes([frame+16])+
                    bytes.fromhex('5f5e5d64890d000000005b83c4')+bytes([frame+12])+ret_bytes)
    else:
        epilogue = (bytes.fromhex('5f5e8b4c24')+bytes([frame+4])+
                    bytes.fromhex('5b64890d0000000083c4')+bytes([frame+12])+ret_bytes)
    assert code(returns[0][0]+3-len(epilogue), len(epilogue)) == epilogue
    # The action loads the exact saved allocation at EBP-50h, calls scalar
    # delete with cdecl cleanup, and returns without consuming owner arguments.
    a = code(action, 11)
    local_offset = -0x24 if members else -0x50
    assert a[:5] == bytes.fromhex('8b45')+bytes([local_offset&255])+bytes.fromhex('50e8') and a[-2:] == bytes.fromhex('59c3')
    delete = action+9+struct.unpack_from('<i', a, 5)[0]
    assert qualified_import(image, identity, delete) == [('msvcrt.dll','??3@YAXPAX@Z')]
    allocations = []
    normal_deletes = []
    for at, data in instructions.items():
        if start <= at < end and data[:1] == b'\xe8':
            target = at+5+struct.unpack_from('<i', data, 1)[0]
            if contextual_bytes(image, target, 2) == bytes.fromhex('ff25'):
                imported = qualified_import(image, identity, target)
                if imported == [('msvcrt.dll','??2@YAPAXI@Z')]: allocations.append(at)
                if imported == [('msvcrt.dll','??3@YAXPAX@Z')]: normal_deletes.append(at)
    assert len(allocations) == len(normal_deletes) == 1
    allocation = allocations[0]
    assert code(allocation-2, 2) == bytes.fromhex('6a0c')
    saved = frame+(28 if members else 24)+local_offset
    assert code(allocation+5, 9) == bytes.fromhex('8bf083c404897424')+bytes([saved])
    assert code(normal_deletes[0]-1, 1) == b'\x56'
    assert code(normal_deletes[0]+5, 3) == bytes.fromhex('83c404')
    h = code(handler, 10)
    assert h[0] == 0xb8 and h[5] == 0xe9
    info = struct.unpack_from('<I', h, 1)[0]
    words = struct.unpack('<8I', contextual_bytes(image, info, 32))
    assert words[:2] == (0x19930520, 1) and words[3:] == (0,0,0,0,0)
    assert struct.unpack('<2I', contextual_bytes(image, words[2], 8)) == (0xffffffff, action)
    if side == 'original': assert [info, words[2]+8] in owner['record']['shared']
    thunk = handler+10+struct.unpack_from('<i', h, 6)[0]
    assert qualified_import(image, identity, thunk) == [('msvcrt.dll','__CxxFrameHandler')]


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--rebuilt',type=Path,required=True)
    parser.add_argument('--owner', choices=['dumper-file','dumper-members'])
    options = parser.parse_args()
    report = json.loads(options.report.read_text())
    images = {'original': options.original, 'rebuilt': options.rebuilt}
    if options.owner in ('dumper-file','dumper-members'):
        for side, image in images.items():
            verify_dumper_file(report['decoded_instructions'], side, image, members=options.owner=='dumper-members')
        print('PASS '+options.owner+': normal caller cleanup, allocation-bound failed-new RET0 action and qualified handler')
    else:
        verify(report, images)
        print('PASS seven C++ owners: symbol RET24, definition RET8, import conversion RET28, converted member RET32, COMDAT report RET0 dumper file RET4 and archive members RET8; distinct cleanup roles and qualified handlers')
