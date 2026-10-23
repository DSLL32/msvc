#!/usr/bin/env python3
"""Finite LINK main IAT certificates; common Rust owns all CFG discovery."""
import json
import re
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM,X86_OP_MEM,X86_OP_REG,X86_REG_EBX,X86_REG_BX,X86_REG_BL,X86_REG_BH
from binary import PE,COFF,u32

START,END=0x40de90,0x40e252
REGION_ENDS={0x40e058:0x40e0dc,0x40e087:0x40e0dc,0x40e155:0x40e24d}
LOADS={0x40e058:('setvbuf',0x401234),0x40e087:('setvbuf',0x401234),0x40e155:('printf',0x401238)}
CALLS={0x40e05e:'setvbuf',0x40e08d:'setvbuf',0x40e0a2:'setvbuf',0x40e15b:'printf',0x40e181:'printf',0x40e1a1:'printf',0x40e225:'printf'}
EXPECTED={'kernel32.dll!SetConsoleCtrlHandler'} | {'msvcrt.dll!'+s for s in ['_splitpath','_strnicmp','_strupr','fflush','isdigit','memmove','printf','setvbuf']}
ALIASES={X86_REG_EBX,X86_REG_BX,X86_REG_BL,X86_REG_BH}


def slots(pe):
    result={};descriptor=pe.base+u32(pe.data,pe.optional+104)
    while True:
        lookup,_,_,dll,iat=struct.unpack('<5I',pe.read(descriptor,20))
        if not dll:return result
        index=0
        while True:
            entry=u32(pe.read(pe.base+lookup+4*index,4),0)
            if not entry:break
            assert not entry&0x80000000
            result[pe.base+iat+4*index]=pe.string(pe.base+dll).lower()+'!'+pe.string(pe.base+entry+2)
            index+=1
        descriptor+=20


def check(original,rebuilt,object_bytes,map_text):
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    instructions=list(cs.disasm(original.read(START,END-START),START))
    assert sum(i.size for i in instructions)==END-START,'fixed main certificate no longer covers exact bytes'
    by_pc={i.address:i for i in instructions};catalog=slots(original)
    register_calls={i.address:i.reg_name(i.operands[0].reg) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
    assert register_calls=={pc:'ebx' for pc in CALLS},'cached register callsite set changed'
    for pc,(name,slot) in LOADS.items():
        i=by_pc[pc];ops=i.operands
        assert i.mnemonic=='mov' and len(ops)==2 and ops[0].type==X86_OP_REG and ops[0].reg==X86_REG_EBX
        assert ops[1].type==X86_OP_MEM and not ops[1].mem.base and not ops[1].mem.index
        assert ops[1].mem.disp==slot and catalog[slot]=='msvcrt.dll!'+name,'cached IAT slot identity changed'
        end=REGION_ENDS[pc]
        for point in instructions:
            if pc<point.address<end:
                writes=ALIASES.intersection(point.regs_access()[1])
                assert not writes or point.address==0x40e087,'cached EBX alias overwritten before restore'
        restore=by_pc[end]
        assert restore.mnemonic=='pop' and restore.op_str=='ebx','cached register restore differs'
    # Two normal-mode alternatives share the final setvbuf call. Incoming
    # edges at 077 execute the second verified load before reading EBX; a
    # 0a7 entry skips all cached calls and reaches only the checked restore.
    # These are explicit finite cases, not an independent CFG walk.
    for i in instructions:
        if not i.group(CS_GRP_JUMP):continue
        assert i.operands[0].type==X86_OP_IMM,'unexpected indirect branch in fixed main body'
        target=i.operands[0].imm&0xffffffff
        if 0x40e058<target<0x40e0dc and not 0x40e058<=i.address<0x40e0dc:
            assert (i.address,target) in [(0x40e040,0x40e0a7),(0x40e043,0x40e077)],'branch bypasses setvbuf cache load'
        assert not (0x40e155<target<0x40e24d and not 0x40e155<=i.address<0x40e24d),'branch bypasses printf cache load'
    for i in instructions:
        if 0x40e077<=i.address<0x40e087:
            assert not ALIASES.intersection(i.regs_access()[0]),'alternate entry reads EBX before its load'
        if 0x40e0a7<=i.address<0x40e0dc:
            assert not ALIASES.intersection(i.regs_access()[0]),'post-cache entry consumes EBX'
    seen={'msvcrt.dll!'+name for name in CALLS.values()};direct=[]
    for i in instructions:
        if i.mnemonic=='call' and i.operands[0].type==X86_OP_MEM:
            m=i.operands[0].mem
            if m.base or m.index:
                assert i.address==0x40e0c3,'unknown indirect memory call'
                continue  # Separate typed tool-mode table proof.
            identity=catalog[m.disp&0xffffffff];seen.add(identity);direct.append(dict(site=i.address,identity=identity))
    assert seen==EXPECTED,'original complete main import set changed'
    publics={name:int(address,16) for name,address in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)}
    obj=COFF(object_bytes);symbol=next(s for s in obj.symbols.values() if s['name']=='_main');section=obj.sections[symbol['section']-1]
    rebuilt_catalog=slots(rebuilt);observed=set();rebuilt_calls=[];data_imports=[]
    for offset,symbol_ref,kind in section['relocs']:
        name=symbol_ref['name']
        if not name.startswith('__imp_'):continue
        assert kind==6
        site=publics['_main']+offset-symbol['value']
        slot=u32(rebuilt.read(site,4),0);identity=rebuilt_catalog[slot]
        if name=='__imp___iob':
            assert identity=='msvcrt.dll!_iob';data_imports.append(site);continue
        assert offset>=2 and section['data'][offset-2:offset]==b'\xff\x15','unresolved rebuilt import-pointer use'
        assert rebuilt.read(site-2,2)==b'\xff\x15'
        assert name[6:]=='_'+identity.split('!',1)[1]+('@8' if identity=='kernel32.dll!SetConsoleCtrlHandler' else ''),'COFF/PE import identity differs'
        observed.add(identity);rebuilt_calls.append(dict(site=site-2,identity=identity))
    assert observed==EXPECTED,'rebuilt complete main import set differs'
    dispatch=[]
    rows=[('Linker',4,1,0x448cb0,'_run_linker@8'),('Library Manager',3,2,0x43ce20,'_run_librarian@8'),
          ('Dumper',4,3,0x42e050,'_run_dumper@8'),('Editor',4,4,0x42f680,'_run_editor@8'),
          ('Helper',4,0,0x40e260,'_run_helper@8')]
    for index,(name,length,kind,function,symbol_name) in enumerate(rows):
        a,an,ak,af=struct.unpack('<4I',original.read(0x467020+16*index,16))
        b,bn,bk,bf=struct.unpack('<4I',rebuilt.read(publics['_tool_modes']+16*index,16))
        assert original.string(a)==rebuilt.string(b)==name,'dispatch name/order differs'
        assert (an,ak)==(bn,bk)==(length,kind),'dispatch prefix/kind differs'
        assert af==function and bf==publics[symbol_name],'dispatch function identity differs'
        dispatch.append(dict(name=name,prefix_length=length,kind=kind,function=symbol_name,original_address=af,rebuilt_address=bf))
    assert original.read(0x467070,16)==rebuilt.read(publics['_tool_modes']+80,16)==bytes(16),'dispatch sentinel differs'
    return dict(original_cached_loads=[dict(site=pc,slot=slot,identity='msvcrt.dll!'+name,restore=REGION_ENDS[pc]) for pc,(name,slot) in LOADS.items()],
        original_cached_calls=[dict(site=pc,identity='msvcrt.dll!'+name) for pc,name in CALLS.items()],original_direct=direct,
        dispatch_table=dispatch,rebuilt_direct=rebuilt_calls,rebuilt_iob_data_references=data_imports,qualified_imports=sorted(seen),
        preserved_register_abi='Win32 cdecl/stdcall callees preserve EBX; all EBX/BX/BL/BH writes are checked through the actual POP EBX restores.',
        scope='Finite main instruction intervals and exact COFF IAT relocations only; shared Rust retains full CFG/RET/reference/ownership/extent checks.')


def check_debug(original,rebuilt,object_bytes,map_text):
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    start,end=0x415230,0x4152ef
    instructions=list(cs.disasm(original.read(start,end-start),start))
    assert sum(i.size for i in instructions)==end-start
    by_pc={i.address:i for i in instructions};catalog=slots(original)
    from capstone.x86 import X86_REG_EDI,X86_REG_DI,X86_REG_EBP,X86_REG_BP
    contracts=[('edi',{X86_REG_EDI,X86_REG_DI},0x415256,0x4152e8,0x401220,'isdigit',[0x415264,0x4152a8,0x4152c3]),
               ('ebp',{X86_REG_EBP,X86_REG_BP},0x415275,0x4152ea,0x401244,'sscanf',[0x415286,0x4152df])]
    actual={(i.address,i.reg_name(i.operands[0].reg)) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
    assert actual=={(site,reg) for reg,_,_,_,_,_,calls in contracts for site in calls},'debug cached callsites differ'
    evidence=[]
    for reg,aliases,load,end_live,slot,name,calls in contracts:
        i=by_pc[load]
        assert i.mnemonic=='mov' and i.operands[0].type==X86_OP_REG and i.reg_name(i.operands[0].reg)==reg
        memory=i.operands[1].mem
        assert not memory.base and not memory.index and memory.disp==slot and catalog[slot]=='msvcrt.dll!'+name,'debug IAT slot differs'
        for point in instructions:
            if load<point.address<end_live:assert not aliases.intersection(point.regs_access()[1]),'debug cached register overwritten'
            if point.group(CS_GRP_JUMP):
                assert point.operands[0].type==X86_OP_IMM
                target=point.operands[0].imm&0xffffffff
                if load<target<end_live and not load<=point.address<end_live:
                    assert reg=='ebp' and (point.address,target)==(0x41524e,0x4152e8),'debug branch bypasses cached load'
        assert by_pc[end_live].mnemonic=='pop' and by_pc[end_live].op_str==reg
        evidence.append(dict(register=reg,load=load,restore=end_live,slot=slot,identity='msvcrt.dll!'+name,calls=calls))
    assert original.read(0x4152e8,3)==b'\x5f\x5e\x5d' # getenv-failure path consumes neither cache.
    direct=[i for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_MEM]
    assert len(direct)==1 and direct[0].address==0x415241 and catalog[direct[0].operands[0].mem.disp]=='msvcrt.dll!getenv'
    publics={n:int(a,16) for n,a in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)}
    obj=COFF(object_bytes);symbol=next(s for s in obj.symbols.values() if s['name']=='_initialize_debug@8')
    section=obj.sections[symbol['section']-1]
    stop=min([s['value'] for s in obj.symbols.values() if s['section']==symbol['section'] and s['type']==32 and s['value']>symbol['value']]+[len(section['data'])])
    rebuilt_catalog=slots(rebuilt);seen=set();calls=[]
    for offset,reference,kind in section['relocs']:
        if not symbol['value']<=offset<stop or not reference['name'].startswith('__imp_'):continue
        assert kind==6 and section['data'][offset-2:offset]==b'\xff\x15'
        site=publics['_initialize_debug@8']+offset-symbol['value']
        assert rebuilt.read(site-2,2)==b'\xff\x15'
        identity=rebuilt_catalog[u32(rebuilt.read(site,4),0)]
        assert reference['name'][6:]=='_'+identity.split('!',1)[1]
        seen.add(identity);calls.append(dict(site=site-2,identity=identity))
    assert seen=={'msvcrt.dll!'+n for n in ['getenv','isdigit','sscanf']},'debug rebuilt import set differs'
    return dict(original_cached=evidence,rebuilt_direct=calls,qualified_imports=sorted(seen),
        qualification='Finite EDI/EBP cache lifetimes through their restores, including getenv-NULL branch after the last cached call; ordinary cdecl/stdcall preserves these registers. Rust owns CFG discovery.')


def check_repro(original,rebuilt,object_bytes,map_text):
    """Finite two-register certificate for the complete reproduction setup."""
    from capstone.x86 import X86_REG_EDI,X86_REG_DI,X86_REG_ESI,X86_REG_SI
    start,end=0x444b10,0x444c33
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    instructions=list(cs.disasm(original.read(start,end-start),start));by_pc={i.address:i for i in instructions}
    assert sum(i.size for i in instructions)==end-start
    catalog=slots(original)
    contracts=[('edi',{X86_REG_EDI,X86_REG_DI},0x444b19,0x444bcf,0x401134,'getenv',[0x444b24,0x444bc1],0x444b9c,0x444c29),
               ('esi',{X86_REG_ESI,X86_REG_SI},0x444b36,0x444be5,0x401138,'_fullpath',[0x444b47,0x444b5e],0x444b9d,0x444c2a)]
    actual={(i.address,i.reg_name(i.operands[0].reg)) for i in instructions if i.mnemonic=='call' and i.operands[0].type==X86_OP_REG}
    assert actual=={(site,reg) for reg,_,_,_,_,_,calls,_,_ in contracts for site in calls},'repro cached callsites differ'
    evidence=[]
    for reg,aliases,load,dead,slot,name,calls,early_restore,final_restore in contracts:
        i=by_pc[load];ops=i.operands
        assert i.mnemonic=='mov' and ops[0].type==X86_OP_REG and i.reg_name(ops[0].reg)==reg
        assert ops[1].type==X86_OP_MEM and not ops[1].mem.base and not ops[1].mem.index
        assert ops[1].mem.disp==slot and catalog[slot]=='msvcrt.dll!'+name,'repro IAT slot identity differs'
        for point in instructions:
            if load<point.address<dead and aliases.intersection(point.regs_access()[1]):
                assert point.address==early_restore and point.mnemonic=='pop' and point.op_str==reg,'repro cache alias overwritten while live'
            if point.group(CS_GRP_JUMP):
                assert point.operands[0].type==X86_OP_IMM
                target=point.operands[0].imm&0xffffffff
                assert not (load<target<dead and not load<=point.address<dead),'repro branch bypasses cache load'
        assert by_pc[dead].mnemonic=='mov' and by_pc[dead].op_str==('edi, eax' if reg=='edi' else 'esi, edi')
        assert max(calls)<dead
        assert by_pc[early_restore].mnemonic==by_pc[final_restore].mnemonic=='pop'
        assert by_pc[early_restore].op_str==by_pc[final_restore].op_str==reg
        evidence.append(dict(register=reg,load=load,slot=slot,identity='msvcrt.dll!'+name,calls=calls,cache_dead_overwrite=dead,early_restore=early_restore,final_restore=final_restore))
    # Each post-cache alias write belongs to the identified inline strcat span.
    # These pointers never become import calls again, then the saved registers
    # are restored. The early-return branch terminates before this span.
    for reg,aliases,load,dead,slot,name,calls,early_restore,final_restore in contracts:
        writes=[i.address for i in instructions if dead<=i.address<final_restore and aliases.intersection(i.regs_access()[1])]
        assert writes==({'edi':[0x444bcf,0x444bda,0x444bde,0x444be9,0x444bee,0x444bf2,0x444bf6,0x444c02], 'esi':[0x444be5,0x444bf6,0x444c02]}[reg]),'post-cache inline string pointer writes differ'
    expected={'msvcrt.dll!'+name for name in ['getenv','_fullpath','_stricmp','fopen']}
    seen={'msvcrt.dll!getenv','msvcrt.dll!_fullpath'}
    for i in instructions:
        if i.mnemonic=='call' and i.operands[0].type==X86_OP_MEM:
            m=i.operands[0].mem;assert not m.base and not m.index
            seen.add(catalog[m.disp])
    assert seen==expected
    publics={n:int(a,16) for n,a in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)}
    obj=COFF(object_bytes);symbol=next(s for s in obj.symbols.values() if s['name']=='_initialize_repro');section=obj.sections[symbol['section']-1]
    stop=min([s['value'] for s in obj.symbols.values() if s['section']==symbol['section'] and s['type']==32 and s['value']>symbol['value']]+[len(section['data'])])
    rebuilt_slots=slots(rebuilt);seen=set();rebuilt_calls=[]
    for offset,reference,kind in section['relocs']:
        if not symbol['value']<=offset<stop or not reference['name'].startswith('__imp_'):continue
        assert kind==6 and section['data'][offset-2:offset]==b'\xff\x15'
        site=publics['_initialize_repro']+offset-symbol['value']
        assert rebuilt.read(site-2,2)==b'\xff\x15'
        identity=rebuilt_slots[u32(rebuilt.read(site,4),0)]
        assert reference['name'][6:]=='_'+identity.split('!',1)[1]
        seen.add(identity);rebuilt_calls.append(dict(site=site-2,identity=identity))
    assert seen==expected,'repro rebuilt qualified import set differs'
    return dict(original_cached=evidence,rebuilt_direct=rebuilt_calls,qualified_imports=sorted(seen),qualification='Bounded import cache lifetimes, explicit inline strcat pointer reuse and both restore paths; no independent CFG walk. All actual bodies retain common Rust CFG/RET/ref/ownership checks.')


def check_commandline(original,rebuilt,object_bytes,map_text):
    from capstone.x86 import X86_REG_EBX,X86_REG_BX,X86_REG_BL,X86_REG_BH
    start,end=0x45cb20,0x45cbb6
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    instructions=list(cs.disasm(original.read(start,end-start),start));by_pc={i.address:i for i in instructions}
    assert sum(i.size for i in instructions)==end-start
    catalog=slots(original);load=by_pc[0x45cb25]
    assert load.mnemonic=='mov' and load.operands[0].reg==X86_REG_EBX
    m=load.operands[1].mem;assert not m.base and not m.index and m.disp==0x401134 and catalog[m.disp]=='msvcrt.dll!getenv'
    aliases={X86_REG_EBX,X86_REG_BX,X86_REG_BL,X86_REG_BH}
    for i in instructions:
        if 0x45cb25<i.address<0x45cbb2:assert not aliases.intersection(i.regs_access()[1]),'argument parser cache overwritten'
        if i.group(CS_GRP_JUMP):
            assert i.operands[0].type==X86_OP_IMM
            target=i.operands[0].imm&0xffffffff
            assert not (0x45cb25<target<0x45cbb2 and not 0x45cb25<=i.address<0x45cbb2),'argument parser branch bypasses cache load'
    assert by_pc[0x45cbb2].mnemonic=='pop' and by_pc[0x45cbb2].op_str=='ebx'
    calls=[(i.address,i.op_str) for i in instructions if i.mnemonic=='call' and i.operands[0].type in (X86_OP_REG,X86_OP_MEM)]
    assert calls==[(0x45cb32,'ebx'),(0x45cb93,'ebx')],'argument parser cached calls differ'
    publics={n:int(a,16) for n,a in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)}
    obj=COFF(object_bytes);symbol=next(s for s in obj.symbols.values() if s['name']=='_parse_command_line@16');section=obj.sections[symbol['section']-1]
    stop=min([s['value'] for s in obj.symbols.values() if s['section']==symbol['section'] and s['type']==32 and s['value']>symbol['value']]+[len(section['data'])])
    rebuilt_slots=slots(rebuilt);seen=[]
    for offset,reference,kind in section['relocs']:
        if not symbol['value']<=offset<stop or not reference['name'].startswith('__imp_'):continue
        assert kind==6 and reference['name']=='__imp__getenv' and section['data'][offset-2:offset]==b'\xff\x15'
        site=publics['_parse_command_line@16']+offset-symbol['value']
        assert rebuilt.read(site-2,2)==b'\xff\x15' and rebuilt_slots[u32(rebuilt.read(site,4),0)]=='msvcrt.dll!getenv'
        seen.append(site-2)
    assert len(seen)==2,'rebuilt argument parser imports differ'
    return dict(original_load=0x45cb25,slot=0x401134,identity='msvcrt.dll!getenv',original_calls=[x[0] for x in calls],restore=0x45cbb2,rebuilt_calls=seen,qualification='Exact whole-body register-write and incoming-branch certificate; cdecl/stdcall callees preserve EBX. Common Rust retains CFG, RET, references and ownership.')


def check_switch(original,rebuilt,object_bytes,map_text):
    from capstone.x86 import X86_REG_EBP,X86_REG_BP
    start,end=0x444f20,0x444f7c
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    instructions=list(cs.disasm(original.read(start,end-start),start));by_pc={i.address:i for i in instructions}
    assert sum(i.size for i in instructions)==end-start
    catalog=slots(original);load=by_pc[0x444f39]
    assert load.mnemonic=='mov' and load.operands[0].reg==X86_REG_EBP
    m=load.operands[1].mem;assert not m.base and not m.index and m.disp==0x40113c and catalog[m.disp]=='msvcrt.dll!_stricmp'
    aliases={X86_REG_EBP,X86_REG_BP};restores={0x444f65,0x444f6e,0x444f75}
    for i in instructions:
        if i.address>0x444f39 and aliases.intersection(i.regs_access()[1]):
            assert i.address in restores and i.mnemonic=='pop' and i.op_str=='ebp','switch cache overwritten'
        if i.group(CS_GRP_JUMP):
            assert i.operands[0].type==X86_OP_IMM
            target=i.operands[0].imm&0xffffffff
            assert not (0x444f39<target<0x444f65 and not 0x444f39<=i.address<0x444f65),'switch branch bypasses cached load'
    assert {i.address for i in instructions if i.mnemonic=='pop' and i.op_str=='ebp'}==restores
    calls=[(i.address,i.op_str) for i in instructions if i.mnemonic=='call']
    assert calls==[(0x444f43,'ebp')]
    publics={n:int(a,16) for n,a in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)}
    obj=COFF(object_bytes);symbol=next(s for s in obj.symbols.values() if s['name']=='_has_switch@4');section=obj.sections[symbol['section']-1]
    stop=min([s['value'] for s in obj.symbols.values() if s['section']==symbol['section'] and s['type']==32 and s['value']>symbol['value']]+[len(section['data'])])
    rebuilt_slots=slots(rebuilt);seen=[]
    for offset,reference,kind in section['relocs']:
        if not symbol['value']<=offset<stop or not reference['name'].startswith('__imp_'):continue
        assert kind==6 and reference['name']=='__imp___stricmp' and section['data'][offset-2:offset]==b'\xff\x15'
        site=publics['_has_switch@4']+offset-symbol['value']
        assert rebuilt.read(site-2,2)==b'\xff\x15' and rebuilt_slots[u32(rebuilt.read(site,4),0)]=='msvcrt.dll!_stricmp'
        seen.append(site-2)
    assert len(seen)==1
    return dict(original_load=0x444f39,slot=0x40113c,identity='msvcrt.dll!_stricmp',original_calls=[0x444f43],restores=sorted(restores),rebuilt_calls=seen,qualification='Exact whole-body EBP/BP writes and incoming branches; count-zero branch skips the cache and all calls. Every original return restores EBP. Common Rust owns CFG/RET/ref/ownership.')
