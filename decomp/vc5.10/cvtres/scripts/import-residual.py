#!/usr/bin/env python3
"""Finite CVTRES cached-IAT certificates, not a CFG discovery engine.

The shared Rust engine owns CFG traversal/graph/RET/extent checks. This checker
verifies six explicit callee-saved register live regions and object relocations
where the common import check intentionally cannot resolve cached registers.
"""
import argparse
import json
import re
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM,X86_OP_MEM,X86_OP_REG,X86_REG_EDI,X86_REG_DI,X86_REG_EBP,X86_REG_BP,X86_REG_ESI,X86_REG_SI
from binary import PE,COFF,u32
from layout import import_addresses
from mask import BINARY,ROOT,sha

CONTRACTS=[
 dict(name='_stricmp',register='edi',load=0x10014e5,end=0x1001811,calls=[0x1001530,0x1001542,0x1001554,0x1001579,0x100159e,0x10015b0,0x10015d6,0x10015e8,0x1001601,0x1001628,0x1001658,0x100166f,0x10016b9,0x10016cb,0x10016dd,0x10016ef,0x1001701,0x100171a,0x1001733,0x100174c,0x100175e,0x100176c,0x100178e,0x10017a0,0x10017c5]),
 dict(name='_fsopen',register='edi',load=0x1001959,end=0x10019f0,calls=[0x100196c,0x10019ee]),
 dict(name='exit',register='ebp',load=0x100196e,end=0x1001a3e,calls=[0x100198e,0x1001a0b,0x1001a3c]),
 dict(name='fclose',register='esi',load=0x1001a27,end=0x1001a34,calls=[0x1001a2f,0x1001a32])]
ALIASES={'edi':{X86_REG_EDI,X86_REG_DI},'ebp':{X86_REG_EBP,X86_REG_BP},'esi':{X86_REG_ESI,X86_REG_SI}}
EXPECTED={'_stricmp','_strnicmp','_fsopen','exit','fclose','printf','puts','_splitpath','_makepath','_tzset'}

def qualified(catalog,slot):
 dll,name=catalog[slot]
 assert dll.lower()=='msvcrt.dll','unexpected imported module'
 return 'msvcrt.dll!'+name

def check(original,rebuilt,objbytes,maptext):
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
 start,end=0x10014a5,0x1001a4c
 body=original.read(start,end-start);instructions=list(cs.disasm(body,start))
 assert sum(i.size for i in instructions)==len(body),'original main linear code certificate no longer covers its bytes'
 by_pc={i.address:i for i in instructions};catalog=import_addresses(original)
 actual_register_calls={(i.address,i.reg_name(i.operands[0].reg)) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
 expected_register_calls={(pc,c['register']) for c in CONTRACTS for pc in c['calls']}
 assert actual_register_calls==expected_register_calls,'cached import callsite set changed'
 result=[];imports=set()
 for contract in CONTRACTS:
  load=by_pc[contract['load']];ops=load.operands;register=contract['register']
  assert load.mnemonic=='mov' and len(ops)==2 and ops[0].type==X86_OP_REG and load.reg_name(ops[0].reg)==register
  assert ops[1].type==X86_OP_MEM and not ops[1].mem.base and not ops[1].mem.index
  slot=ops[1].mem.disp&0xffffffff;identity=qualified(catalog,slot)
  assert identity=='msvcrt.dll!'+contract['name'],'cached IAT slot swapped: '+identity
  writes=[]
  for instruction in instructions:
   if contract['load']<instruction.address<contract['end']:
    _,changed=instruction.regs_access()
    if ALIASES[register].intersection(changed):writes.append(instruction.address)
   if instruction.group(CS_GRP_JUMP):
    assert instruction.operands[0].type==X86_OP_IMM,'unexpected indirect branch in fixed main certificate'
    target=instruction.operands[0].imm&0xffffffff
    assert not(contract['load']<target<contract['end'] and not contract['load']<=instruction.address<contract['end']),'external branch bypasses cached import load'
  assert not writes,('callee-saved cached register overwritten',contract['name'],writes)
  imports.add(identity)
  result.append(dict(import_identity=identity,slot=slot,load=contract['load'],live_end=contract['end'],calls=contract['calls'],intervening_register_writes=writes))
 direct=[]
 for instruction in instructions:
  if instruction.mnemonic!='call' or instruction.operands[0].type!=X86_OP_MEM:continue
  memory=instruction.operands[0].mem
  assert not memory.base and not memory.index and (memory.disp&0xffffffff) in catalog,'unresolved original memory call'
  identity=qualified(catalog,memory.disp&0xffffffff);imports.add(identity)
  direct.append(dict(pc=instruction.address,import_identity=identity))
 assert imports=={'msvcrt.dll!'+name for name in EXPECTED}
 seen,calls=rebuilt_import_calls(rebuilt,objbytes,maptext,'_main')
 assert seen==imports,('complete main import identity set differs',imports,seen)
 return dict(original_cached=result,original_direct=direct,rebuilt_direct=calls,qualified_imports=sorted(imports),
   scope='Four fixed callee-saved-register intervals plus exact compiled COFF IAT relocations. Rust owns CFG reachability and graph comparison; this residual does not traverse a CFG.',
   abi_assumption='All calls inside these intervals use Win32 cdecl/stdcall ABIs preserving EDI/EBP/ESI; no indirect main jump or entry bypassing the specified load is permitted.')

def rebuilt_import_calls(rebuilt,objbytes,maptext,function_symbol):
 publics={name:int(address,16) for name,address in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',maptext,re.M)}
 obj=COFF(objbytes);main=next(v for v in obj.symbols.values() if v['name']==function_symbol)
 section=obj.sections[main['section']-1];rebuilt_imports=import_addresses(rebuilt);seen=set();calls=[]
 end=min([s['value'] for s in obj.symbols.values() if s['section']==main['section'] and s['type']==32 and s['value']>main['value']]+[len(section['data'])])
 for offset,symbol,kind in section['relocs']:
  if not main['value']<=offset<end:continue
  if not symbol['name'].startswith('__imp_'):continue
  assert kind==6 and offset>=2 and section['data'][offset-2:offset]==b'\xff\x15','unresolved rebuilt imported pointer use'
  pc=publics[function_symbol]+offset-main['value']-2
  actual=rebuilt.read(pc,6);assert actual[:2]==b'\xff\x15'
  slot=u32(actual,2);identity=qualified(rebuilt_imports,slot)
  assert symbol['name'][6:]=='_'+identity.split('!',1)[1],'COFF relocation/import slot identity differs'
  seen.add(identity);calls.append(dict(pc=pc,import_identity=identity))
 return seen,calls

def check_usage(original,rebuilt,objbytes,maptext):
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
 start,end=0x1001460,0x10014a5
 instructions=list(cs.disasm(original.read(start,end-start),start));assert sum(i.size for i in instructions)==end-start
 catalog=import_addresses(original);by_pc={i.address:i for i in instructions};load=by_pc[0x1001483]
 assert load.mnemonic=='mov' and load.op_str.startswith('esi, dword ptr [')
 slot=load.operands[1].mem.disp&0xffffffff
 assert qualified(catalog,slot)=='msvcrt.dll!puts','usage cached IAT slot swapped'
 calls={(i.address,i.reg_name(i.operands[0].reg)) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
 assert calls=={(0x100148a,'esi'),(0x1001493,'esi')},'usage cached callsite set changed'
 for i in instructions:
  if 0x1001483<i.address<0x1001495:assert not ALIASES['esi'].intersection(i.regs_access()[1]),'usage cached register overwritten'
  assert not i.group(CS_GRP_JUMP),'usage finite straight-line certificate contains a branch'
 imports={'msvcrt.dll!puts'};direct=[]
 for i in instructions:
  if i.mnemonic=='call' and i.operands[0].type==X86_OP_MEM:
   memory=i.operands[0].mem;assert not memory.base and not memory.index
   identity=qualified(catalog,memory.disp&0xffffffff);imports.add(identity);direct.append(dict(pc=i.address,import_identity=identity))
 seen,rebuilt_calls=rebuilt_import_calls(rebuilt,objbytes,maptext,'_usage@4')
 assert imports==seen=={'msvcrt.dll!puts','msvcrt.dll!printf','msvcrt.dll!exit'}
 return dict(original_cached=dict(import_identity='msvcrt.dll!puts',slot=slot,load=0x1001483,live_end=0x1001495,calls=[0x100148a,0x1001493]),original_direct=direct,rebuilt_direct=rebuilt_calls,qualified_imports=sorted(imports),scope='Fixed straight-line ESI live interval; no independent CFG traversal.')

def check_insert(original,rebuilt,objbytes,maptext):
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
 start,end=0x1001d49,0x1001fdf
 instructions=list(cs.disasm(original.read(start,end-start),start));assert sum(i.size for i in instructions)==end-start
 by_pc={i.address:i for i in instructions};catalog=import_addresses(original)
 load_pc,live_end=0x1001ecc,0x1001f52
 callsites=[0x1001ed3,0x1001f1e,0x1001f50]
 load=by_pc[load_pc];ops=load.operands
 assert load.mnemonic=='mov' and len(ops)==2 and ops[0].type==X86_OP_REG and load.reg_name(ops[0].reg)=='esi'
 assert ops[1].type==X86_OP_MEM and not ops[1].mem.base and not ops[1].mem.index
 slot=ops[1].mem.disp&0xffffffff
 assert qualified(catalog,slot)=='msvcrt.dll!printf','insert cached IAT slot swapped'
 actual={(i.address,i.reg_name(i.operands[0].reg)) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
 assert actual=={(pc,'esi') for pc in callsites},'insert cached callsite set changed'
 for instruction in instructions:
  if load_pc<instruction.address<live_end:
   assert not ALIASES['esi'].intersection(instruction.regs_access()[1]),'insert cached register overwritten'
  if instruction.group(CS_GRP_JUMP):
   assert instruction.operands[0].type==X86_OP_IMM,'unexpected indirect branch in fixed insert certificate'
   target=instruction.operands[0].imm&0xffffffff
   assert not(load_pc<target<live_end and not load_pc<=instruction.address<live_end),'insert branch bypasses cached load'
 imports={'msvcrt.dll!printf'};direct=[]
 for instruction in instructions:
  if instruction.mnemonic!='call' or instruction.operands[0].type!=X86_OP_MEM:continue
  memory=instruction.operands[0].mem
  assert not memory.base and not memory.index
  identity=qualified(catalog,memory.disp&0xffffffff);imports.add(identity)
  direct.append(dict(pc=instruction.address,import_identity=identity))
 seen,rebuilt_calls=rebuilt_import_calls(rebuilt,objbytes,maptext,'_insert_resource@12')
 assert imports==seen=={'msvcrt.dll!printf','msvcrt.dll!wcscmp','msvcrt.dll!exit'},'insert qualified import set differs'
 return dict(original_cached=dict(import_identity='msvcrt.dll!printf',slot=slot,load=load_pc,live_end=live_end,calls=callsites),original_direct=direct,rebuilt_direct=rebuilt_calls,qualified_imports=sorted(imports),scope='One fixed callee-saved ESI interval; complete linear register-call set/write/bypass checks, no independent CFG traversal.')

def check_bitmap(original,rebuilt,objbytes,maptext):
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
 start,end=0x1002225,0x100266b
 instructions=list(cs.disasm(original.read(start,end-start),start));assert sum(i.size for i in instructions)==end-start
 by_pc={i.address:i for i in instructions};catalog=import_addresses(original)
 contracts=[dict(register='esi',load=0x10023bb,end=0x10023ec,calls=[0x10023c6,0x10023e2]),dict(register='edi',load=0x100257e,end=0x10025a9,calls=[0x1002589,0x100259f])]
 actual={(i.address,i.reg_name(i.operands[0].reg)) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
 assert actual=={(pc,c['register']) for c in contracts for pc in c['calls']},'bitmap cached callsite set changed'
 # The reverse-conversion failure jumps into this error-only push/jmp pair.
 # It leaves the ESI interval without reaching either cached fwrite call.
 assert by_pc[0x10023d0].mnemonic=='push' and by_pc[0x10023d0].op_str=='0x1001410','bitmap error-only push changed'
 assert by_pc[0x10023d5].mnemonic=='jmp' and by_pc[0x10023d5].operands[0].imm==0x10025ae,'bitmap error-only jump changed'
 certified=[]
 for contract in contracts:
  load=by_pc[contract['load']];ops=load.operands;register=contract['register']
  assert load.mnemonic=='mov' and ops[0].type==X86_OP_REG and load.reg_name(ops[0].reg)==register
  assert ops[1].type==X86_OP_MEM and not ops[1].mem.base and not ops[1].mem.index
  slot=ops[1].mem.disp&0xffffffff;assert qualified(catalog,slot)=='msvcrt.dll!fwrite','bitmap cached IAT slot swapped'
  bypasses=[]
  for instruction in instructions:
   if contract['load']<instruction.address<contract['end']:
    assert not ALIASES[register].intersection(instruction.regs_access()[1]),'bitmap cached register overwritten'
   if instruction.group(CS_GRP_JUMP):
    assert instruction.operands[0].type==X86_OP_IMM
    target=instruction.operands[0].imm&0xffffffff
    if contract['load']<target<contract['end'] and not contract['load']<=instruction.address<contract['end']:
     assert register=='esi' and instruction.address==0x1002591 and target==0x10023d0,'bitmap branch bypasses cached load'
     assert instruction.mnemonic=='jne';bypasses.append(dict(source=instruction.address,target=target,error_only_exit=0x10025ae))
  certified.append(dict(import_identity='msvcrt.dll!fwrite',slot=slot,**contract,certified_error_bypasses=bypasses))
 imports={'msvcrt.dll!fwrite'};direct=[]
 for instruction in instructions:
  if instruction.mnemonic!='call' or instruction.operands[0].type!=X86_OP_MEM:continue
  memory=instruction.operands[0].mem;assert not memory.base and not memory.index
  identity=qualified(catalog,memory.disp&0xffffffff);imports.add(identity);direct.append(dict(pc=instruction.address,import_identity=identity))
 seen,rebuilt_calls=rebuilt_import_calls(rebuilt,objbytes,maptext,'_convert_ce_image@20')
 assert imports==seen=={'msvcrt.dll!fwrite','msvcrt.dll!printf','msvcrt.dll!fprintf','msvcrt.dll!__p__iob'},'bitmap qualified import set differs'
 return dict(original_cached=certified,original_direct=direct,rebuilt_direct=rebuilt_calls,qualified_imports=sorted(imports),scope='Two fixed fwrite intervals and one exact error-only push/jmp bypass, no independent CFG traversal.')

def check_diagnostic(original,rebuilt,objbytes,maptext,name):
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
 if name=='warning':start,end,load_pc,live_end,calls=0x1001bf7,0x1001c39,0x1001c0a,0x1001c34,[0x1001c11,0x1001c32]
 else:
  assert name=='error'
  start,end,load_pc,live_end,calls=0x1001bb5,0x1001bf7,0x1001bc8,0x1001bf2,[0x1001bcf,0x1001bf0]
 instructions=list(cs.disasm(original.read(start,end-start),start));assert sum(i.size for i in instructions)==end-start
 catalog=import_addresses(original);by_pc={i.address:i for i in instructions};load=by_pc[load_pc];ops=load.operands
 assert load.mnemonic=='mov' and ops[0].type==X86_OP_REG and load.reg_name(ops[0].reg)=='esi'
 assert ops[1].type==X86_OP_MEM and not ops[1].mem.base and not ops[1].mem.index
 slot=ops[1].mem.disp&0xffffffff;assert qualified(catalog,slot)=='msvcrt.dll!printf','diagnostic cached IAT slot swapped'
 actual={(i.address,i.reg_name(i.operands[0].reg)) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
 assert actual=={(pc,'esi') for pc in calls},'diagnostic cached callsite set changed'
 for instruction in instructions:
  assert not instruction.group(CS_GRP_JUMP),'diagnostic straight-line certificate has a branch'
  if load_pc<instruction.address<live_end:
   assert not ALIASES['esi'].intersection(instruction.regs_access()[1]),'diagnostic cached register overwritten'
 imports={'msvcrt.dll!printf'};direct=[]
 for instruction in instructions:
  if instruction.mnemonic!='call' or instruction.operands[0].type!=X86_OP_MEM:continue
  memory=instruction.operands[0].mem;assert not memory.base and not memory.index
  identity=qualified(catalog,memory.disp&0xffffffff);imports.add(identity);direct.append(dict(pc=instruction.address,import_identity=identity))
 seen,rebuilt_calls=rebuilt_import_calls(rebuilt,objbytes,maptext,'_'+name)
 assert imports==seen=={'msvcrt.dll!printf','msvcrt.dll!vprintf'},'diagnostic qualified import set differs'
 return dict(original_cached=dict(import_identity='msvcrt.dll!printf',slot=slot,load=load_pc,live_end=live_end,calls=calls),original_direct=direct,rebuilt_direct=rebuilt_calls,qualified_imports=sorted(imports),scope='One exact straight-line callee-saved ESI interval, no branch/CFG inference.')

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 original=ROOT/'toolchains/5.10/bin/CVTRES.EXE';rebuilt=BINARY/'build/CVTRES.EXE';obj=BINARY/'build/main.obj';mapfile=BINARY/'build/cvtres.map'
 result=check(PE(original),PE(rebuilt),obj.read_bytes(),mapfile.read_text())
 result.update(original_sha256=sha(original.read_bytes()),rebuilt_sha256=sha(rebuilt.read_bytes()),object_sha256=sha(obj.read_bytes()),map_sha256=sha(mapfile.read_bytes()))
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(imports=len(result['qualified_imports']),cached_intervals=len(result['original_cached']),output=str(a.output))))
