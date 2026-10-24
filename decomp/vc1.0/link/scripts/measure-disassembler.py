#!/usr/bin/env python3
"""Read-only original CFG inventory for accepted contextual bodies; never execute a target."""
from pathlib import Path
import json,argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path)
parser.add_argument('--family',choices=('debug-log','omf-continuation','editor-errors','resource-identity','mac-loader-map','cross-import','section-edit','large-image','editor','disassembler','macintosh','definition','demangler','dump','fpo-absolute','omf-records','hash-wildcards','legacy-resources','omf-lidata','rom-repro'),default='disassembler')
options=parser.parse_args()
from indexes import indexes
from binary import u32
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_GRP_JUMP,CS_GRP_RET
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
original,_,cfg,records,_=indexes();cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
accepted=set(cfg['accepted']);bodies={};total=set();unresolved=[];extent=set();data=set()
sources={'debug-log':('src/driver.c','src/comdat-diagnostics.c'),'omf-continuation':('src/omf.c',),'editor-errors':('src/editor-rebase.c','src/editor-bind.c','src/diagnostics.c'),'resource-identity':('src/resources.c','src/diagnostics.c'),'mac-loader-map':('src/mac-object.c','src/symbols.c'),'cross-import':('src/definition-file.c','src/import-members.c'),'section-edit':('src/editor.c',),'large-image':('src/options.c','src/link-pass.c','src/ordering.c','src/files.c','src/disassembler.c'),'editor':('src/editor-rebase.c','src/editor-bind.c','src/checksum.c'),'rom-repro':('src/repro.c','src/object.c','src/pass-one.c','src/section-layout.c','src/base-relocations.c'),'omf-lidata':('src/omf.c',),'legacy-resources':('src/resource-legacy.c',),'hash-wildcards':('src/arguments.c','src/symbols.c'),'omf-records':('src/omf.c',),'fpo-absolute':('src/dump.c','src/link-pass.c','src/object.c','src/symbols.c'),
         'dump':('src/dump.c',),
         'demangler':('src/demangle.c',),
         'disassembler':('src/disassembler.c','src/disassembly-output.c'),
         'definition':('src/definition-file.c',),
         'macintosh':('src/mac-object.c','src/mac-sections.c','src/mac-fixups.c','src/m68k-relocations.c','src/mac-debug.c')}
case_groups={'debug-log':('direct-debug-log',),'omf-continuation':('omf-continuation-continued','omf-continuation-use16'),'editor-errors':('editor-errors-rebase','editor-errors-bind','editor-errors-missing'),'resource-identity':('resource-identity-duplicate','resource-identity-duplicate-controlled'),'mac-loader-map':('mac-loader-map',),'cross-import':('import-archive-i860',),'section-edit':('edit-section-rename',),'large-image':('large-image-mapped','large-image-extra','large-image-dump486','large-image-standard486'),'editor':('editor-rebase','editor-bind','editor-release'),'rom-repro':('rom-local-imports',),'omf-lidata':('omf-lidata-exestr-library',),'legacy-resources':('legacy-win16-resources',),'hash-wildcards':('large-symbols-wildcards',),'fpo-absolute':('dump-fpo-map','absolute-weak'),'omf-records':('omf-records-library',)}
family_entries={name for name,case in zip(cfg['accepted'],cfg['frontier_cases']) if case in case_groups.get(options.family,())}
for r in cfg['driver']:
 if options.family in case_groups and r['name'] not in family_entries:continue
 if r['name'] not in accepted or r.get('source') not in sources[options.family]:continue
 lo,hi=r['start'],r['end'];pending=[lo]+r.get('cfg_entries',[]);seen=set();body=set();jumps=[]
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
