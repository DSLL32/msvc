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

        verify_import_conversion(facts, side, image)


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
    assert len(normal_returns) == 1 and normal_returns[0][1] == b'\xc2'+struct.pack('<H', argument_bytes)
    epilogue = (bytes.fromhex('8b8c24')+struct.pack('<I',frame_bytes+16)+
                bytes.fromhex('5f5e5d64890d000000005b81c4')+
                struct.pack('<I',frame_bytes+12)+b'\xc2'+
                struct.pack('<H',argument_bytes))
    ret = normal_returns[0][0]
    assert code(ret+3-len(epilogue), len(epilogue)) == epilogue
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


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--rebuilt',type=Path,required=True)
    options = parser.parse_args()
    verify(json.loads(options.report.read_text()),
           {'original':options.original,'rebuilt':options.rebuilt})
    print('PASS symbol/definition RET24/8 and import conversion RET28, distinct object/failure cleanup roles, qualified C++ handlers')
