#!/usr/bin/env python3
"""Narrow expression/lexer byte identity and noncoincident store proof."""
import argparse,copy,json,re,shutil,struct,time
from pathlib import Path
from protocol import HERE,ROOT,sha
from rich_protocol import raw_bytes
p=argparse.ArgumentParser();p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();started=time.perf_counter()
assert not a.output.exists();a.output.mkdir(parents=True)

def receipt(directory):
    value=json.loads((directory/'receipt.json').read_text())
    for name,item in value['files'].items():
        path=directory/name;assert path.stat().st_size==item['bytes']and sha(path)==item['sha256'],name
    assert value['expression_store_probe']is True
    return value
old=receipt(a.original);new=receipt(a.native)
assert old['status']==123 and not old['timed_out']and new['status']==0
for filename,canonical in [('loaded-target.bin','RCDLL.DLL'),('loaded-crt.bin','MSVCRT.DLL')]:assert sha(a.original/filename)==sha(ROOT/'toolchains/5.10/bin'/canonical)
text=(a.original/'events.txt').read_text()
assert re.findall(r'^expression-probe-storage counter=([0-9a-f]+) lookahead=([0-9a-f]+) lexer=([0-9a-f]+)$',text,re.M)==[('71c38e7c','71c349dc','71c344f4')]
assert re.findall(r'^expression-probe-child ([0-9a-f]+)$',text,re.M)==['00002d83']
state=json.loads((a.native/'snapshot.json').read_text());assert state['kind']=='expression-store-probe'and state['status']=='pass'
addresses=state['addresses'];assert len(set(addresses.values()))==3 and state['widths']==dict(rc_expression_parentheses=4,rc_expression_token=1,rc_token_kind=1)
expected_before=dict(counter=0x11223344,lookahead=0xa5,lexer=0x5a);expected_after=dict(counter=0,lookahead=1,lexer=0x5a)
for phase,expected in [('before',expected_before),('after',expected_after)]:
    observed={name:int.from_bytes(raw_bytes(text,'expression-probe-'+phase+'-'+name),'little')for name in expected}
    assert observed==state[phase]==expected
# Compiled native source identifies two genuine variables, never aliases.
source=json.loads((a.native/'compiled-source.json').read_text())['contents']['decomp/vc5.10/rcdll/src/directives.c']['text']
body=source.split('int __cdecl rc_evaluate_condition(void)',1)[1].split('int __cdecl rc_parse_conditional_expression(void)',1)[0]
assert 'rc_expression_token=1;'in body and not re.search(r'\brc_token_kind\b',body)
assert 'unsigned char rc_expression_token=1;'in source

def pe_slice(path,va,size):
    data=path.read_bytes();pe=struct.unpack_from('<I',data,60)[0];opt=pe+24;base=struct.unpack_from('<I',data,opt+28)[0];rva=va-base
    for i in range(struct.unpack_from('<H',data,pe+6)[0]):
        _,start,length,raw=struct.unpack_from('<IIII',data,opt+struct.unpack_from('<H',data,pe+20)[0]+i*40+8)
        if start<=rva and rva+size<=start+length:return data[raw+rva-start:raw+rva-start+size]
    raise AssertionError('unbacked PE span')
shutil.copyfile(HERE/'build/RCDLL.DLL',a.output/'rebuilt.dll');shutil.copyfile(HERE/'build/rcdll.map',a.output/'rebuilt.map')
mapping=(a.output/'rebuilt.map').read_text()
def public(name):
    rows=re.findall(r'\s'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\b',mapping);assert len(rows)==1
    return int(rows[0],16)
rebuilt={name:public('_'+name)for name in ['rc_evaluate_condition','rc_parse_conditional_expression','rc_expression_parentheses','rc_expression_token','rc_token_kind']}
assert len(set(rebuilt.values()))==5
new_body=pe_slice(a.output/'rebuilt.dll',rebuilt['rc_evaluate_condition'],rebuilt['rc_parse_conditional_expression']-rebuilt['rc_evaluate_condition'])
old_body=pe_slice(a.original/'loaded-target.bin',0x71c22cc0,195)
def byte_store(address):return b'\xc6\x05'+struct.pack('<I',address)+b'\x01'
assert byte_store(0x71c349dc)in old_body and byte_store(0x71c344f4)not in old_body
assert byte_store(rebuilt['rc_expression_token'])in new_body and byte_store(rebuilt['rc_token_kind'])not in new_body
# Bind actual post-child BYTE readers as well as the first reset writer.
def al_read(address):return b'\xa0'+struct.pack('<I',address)
assert al_read(0x71c349dc)in old_body and al_read(0x71c344f4)not in old_body
assert al_read(rebuilt['rc_expression_token'])in new_body and al_read(rebuilt['rc_token_kind'])not in new_body
assert b'\x8a\x0d'+struct.pack('<I',rebuilt['rc_expression_token'])in new_body
assert b'\x8a\x0d'+struct.pack('<I',rebuilt['rc_token_kind'])not in new_body
for image,lookahead,lexer in [(a.original/'loaded-target.bin',0x71c349dc,0x71c344f4),(a.output/'rebuilt.dll',rebuilt['rc_expression_token'],rebuilt['rc_token_kind'])]:
    assert pe_slice(image,lookahead,1)==pe_slice(image,lexer,1)==b'\x01'
# Controls reach the semantic predicate on coherent in-memory observations.
controls=[]
for name,changed in [('wrong-lookahead-write',dict(lookahead=0xa5)),('lexer-overwritten',dict(lexer=1)),('counter-not-reset',dict(counter=0x11223344))]:
    mutant=copy.deepcopy(state);mutant['after'].update(changed)
    try:assert mutant['after']==expected_after
    except AssertionError:controls.append(name)
    else:raise AssertionError('sentinel mutation passed')
wrong=byte_store(rebuilt['rc_token_kind'])
mutant=new_body.replace(byte_store(rebuilt['rc_expression_token']),wrong);assert mutant!=new_body
try:assert byte_store(rebuilt['rc_expression_token'])in mutant and wrong not in mutant
except AssertionError:controls.append('compiled-wrong-lexer-destination')
else:raise AssertionError('compiled wrong-byte store passed')
mutant=new_body.replace(al_read(rebuilt['rc_expression_token']),al_read(rebuilt['rc_token_kind']));assert mutant!=new_body
try:assert al_read(rebuilt['rc_expression_token'])in mutant and al_read(rebuilt['rc_token_kind'])not in mutant
except AssertionError:controls.append('compiled-wrong-lexer-read')
else:raise AssertionError('compiled wrong-byte read passed')
result=dict(status='pass',original=str(a.original),native=str(a.native),original_receipt_sha256=sha(a.original/'receipt.json'),native_receipt_sha256=sha(a.native/'receipt.json'),original_target_sha256=sha(a.original/'loaded-target.bin'),original_crt_sha256=sha(a.original/'loaded-crt.bin'),native_elf_sha256=sha(a.native/'captured-native.elf'),before=expected_before,after=expected_after,original_storage=dict(counter='0x71c38e7c',lookahead='0x71c349dc',lexer='0x71c344f4'),native_storage=addresses,widths=state['widths'],static_reader_identity=dict(original='MOV AL,[lookahead149dc], no lexer144f4 read',rebuilt='MOV AL and MOV CL,[MAP lookahead], no MAP lexer read',read_branch_dynamic_scope='Actual sentinel run stops before first child parsing; post-child read role separately bound by exact original/rebuilt opcodes and contextual C source, not claimed dynamically traversed.'),rebuilt_publics={name:hex(value)for name,value in rebuilt.items()},negative_controls=controls,scope='Opt-in debugger-written noncoincident inputs; actual production wrapper executes through both stores and stops at first conditional child. Original123 is controller-selected termination; native GDB0 is not target completion. No function/data credit added. Earlier value-only proofs remain historical and are not storage-identity proof.',files={f.name:sha(f)for f in a.output.iterdir()if f.is_file()},checker_sha256=sha(Path(__file__)),seconds=time.perf_counter()-started)
(a.output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
