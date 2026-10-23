"""Whole CodeView module driver guard, global visit and db FILE ownership."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,initialized,opened):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def string(a):return bytes(u.mem_read(a,64)).split(b'\0')[0].decode()
    globals={n:names[n]if right else a for n,a in [('_debug_symbols_segment',0x7914c),('_codeview_output_started',0x84fd8),('_intermediate_prefix',0x83130),('_codeview_type_file',0x7b8ec)]}
    put(globals['_debug_symbols_segment'],0x204000);put(globals['_codeview_output_started'],initialized);put(globals['_intermediate_prefix'],0x202000);put(globals['_codeview_type_file'],0x205100);u.mem_write(0x202000,b'ir\0')
    functions=[('lookup','@lookup_global_symbol@4',0x4dce0),('signature','_write_codeview_signature',0x69cd0),('object','_write_codeview_object_name',0x69c60),('visit','@for_each_symbol@8',0x4e7f0),('compile','_write_codeview_compile_record',0x69bd0),('open','@open_intermediate@16',0x70120),('read','@read_codeview_initializers@4',0x69ba0),('close','_fclose',0x71b80),('error','@compiler_internal_error@8',0x6fb70)]
    targets={names[n]if right else a:label for label,n,a in functions};visitor=names['@write_debug_symbol@4']if right else 0x69930;put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);events=[];terminal=['return']
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in targets:return
        label=targets[a];esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0;pop=4
        if label=='lookup':assert ecx==7;result=0x203000
        else:assert word(globals['_debug_symbols_segment'])==0x203000
        if label=='visit':assert ecx==0 and edx==visitor
        elif label=='open':
            assert [string(ecx),string(edx),string(word(esp+4)),string(word(esp+8))]==['ir','db','r','db il'];result=0x205000 if opened else 0;pop=12
        elif label=='read':assert ecx==0x205000==word(globals['_codeview_type_file'])
        elif label=='close':assert word(esp+4)==0x205000==word(globals['_codeview_type_file'])
        elif label=='error':
            assert string(ecx)=='@(#)emit.c:1.20'and edx==0x361;events.append(label);terminal[0]='named diagnostic';m.emu_stop();return
        events.append(label);m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+pop)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['_finish_debug_module']if right else 0x69180,0,count=100000)
    if opened:assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,terminal=terminal[0],started=word(globals['_codeview_output_started']),segment=word(globals['_debug_symbols_segment']),type_file=word(globals['_codeview_type_file']))
rows=[]
for values in itertools.product([0,1,-1],[0,1]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(input=values,result=a))
Path('cases/codeview-module-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole138-byte original versus actual original-compiler C, with named child mocks. Initial signature guard, global callback identity, compiler record order, exactir/db/r/db-il argument shape and stored/forwarded/closed FILE pointer checked. Failed-open controls stop at named diagnostic with exact source/code; no actual CRT failure, file-read child, global-symbol traversal or unvisited diagnostic body acceptance. File global retains closed pointer as original; no invented null/reset.'),indent=2)+'\n');print('6 whole original/canonical module driver/ownership states match')
