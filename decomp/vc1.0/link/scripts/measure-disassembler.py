#!/usr/bin/env python3
"""Read-only original CFG inventory for accepted contextual bodies; never execute a target."""
from pathlib import Path
import json,argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path)
parser.add_argument('--family',choices=('disassembler','macintosh','definition'),default='disassembler')
options=parser.parse_args()
from indexes import indexes
from binary import u32
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_GRP_JUMP,CS_GRP_RET
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
original,_,cfg,records,_=indexes();cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
accepted=set(cfg['accepted']);bodies={};total=set();unresolved=[];extent=set();data=set()
sources={'disassembler':('src/disassembler.c','src/disassembly-output.c'),
         'definition':('src/definition-file.c',),
         'macintosh':('src/mac-object.c','src/mac-sections.c','src/mac-fixups.c','src/m68k-relocations.c')}
for r in cfg['driver']:
 if r['name'] not in accepted or r.get('source') not in sources[options.family]:continue
 lo,hi=r['start'],r['end'];pending=[lo];seen=set();body=set();jumps=[]
 extent.update(range(lo,hi))
 for a,b in r.get('shared',[]):data.update(range(a,b));extent.update(range(a,b))
 while pending:
  pc=pending.pop()
  if pc in seen or not lo<=pc<hi:continue
  seen.add(pc);ins=next(cs.disasm(original.read(pc,15),pc,count=1),None);assert ins and pc+ins.size<=hi,(r['name'],hex(pc));body.update(range(pc,pc+ins.size))
  if ins.group(CS_GRP_RET):continue
  if ins.group(CS_GRP_JUMP):
   op=ins.operands[0]
   if op.type==X86_OP_IMM:
    if lo<=op.imm<hi:pending.append(op.imm)
   elif ins.mnemonic=='jmp' and op.type==X86_OP_MEM:
    pointer=op.mem.disp;targets=[]
    try:
     for i in range(4096):
      target=u32(original.read(pointer+4*i,4),0)
      if not lo<=target<hi:break
      targets.append(target)
    except (ValueError,IndexError):pass
    if len(targets)>=2:
     pending.extend(targets);jumps.append({'address':pointer,'targets':targets})
     data.update(range(pointer,pointer+len(targets)*4))
    else:unresolved.append({'function':r['name'],'instruction':pc,'operand':ins.op_str})
   else:unresolved.append({'function':r['name'],'instruction':pc,'operand':ins.op_str})
   if ins.mnemonic=='jmp':continue
  pending.append(pc+ins.size)
 total.update(body);bodies[r['name']]={'bytes':len(body),'start':lo,'end':hi,'internal_tables':jumps}
old=set()
for line in Path('analysis/native-body-inventory.tsv').read_text().splitlines():
 addr,name,ranges,calls=line.split('\t');ranges=[tuple(int(v,16) for v in item.split(':')) for item in ranges.strip(',').split(',') if item]
 if len(ranges)==1 and ranges[0][0]==ranges[0][1]:continue
 for a,b in ranges:old.update(range(a,b+1))
missing=sorted(extent-total-data);holes=[]
for address in missing:
 if holes and holes[-1][1]==address:holes[-1][1]=address+1
 else:holes.append([address,address+1])
padding=[]
for a,b in holes:
 items=list(cs.disasm(original.read(a,b-a),a));assert sum(i.size for i in items)==b-a,(hex(a),hex(b))
 padding.extend((i.mnemonic,i.op_str) for i in items)
allowed={('add','eax, 0'),('lea','ebx, [ebx]'),('lea','ecx, [ecx]'),('lea','esp, [esp]'),('mov','edi, edi'),('nop','')}
assert set(padding)<=allowed, 'unclassified non-padding extent gaps'
report={'target_sha256':cfg['target']['sha256'],'family':options.family,'alignment_bytes':len(missing),'extent_gap_ranges':holes,'extent_gap_instructions':sorted(set(padding)),'method':'Static original CFG instruction-byte union of all accepted contextual '+options.family+' functions, follows both conditional edges, internal JMP tables and call fallthrough; excludes padding, inline table bytes and callee bodies. Not dynamic branch coverage. Old Ghidra one-byte seeds excluded from overlap.','functions':bodies,'function_count':len(bodies),'instruction_bytes':len(total),'accepted_extent_union_bytes':len(extent),'internal_data_bytes':len(data),'extent_outside_cfg_and_data':len(extent-total-data),'unresolved_jumps':unresolved,'instruction_overlap_old_body_inventory':len(total&old),'additional_proven_instruction_bytes_outside_old_inventory':len(total-old)}
assert not unresolved,unresolved
if options.output:options.output.write_text(json.dumps(report,indent=2)+'\n')
print('%d accepted %s bodies: %d instruction bytes, %d inline data, %d alignment; %d instruction bytes outside old inventory'% (report['function_count'],options.family,report['instruction_bytes'],report['internal_data_bytes'],report['alignment_bytes'],report['additional_proven_instruction_bytes_outside_old_inventory']))
