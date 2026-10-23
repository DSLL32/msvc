"""Original/native whole file-cache control and typed row20 alias proof.

fopen and diagnostics are explicitly mocked; this is not real failure-path CRT
or diagnostic ABI acceptance. Real case frontiers are independently captured.
"""
from pathlib import Path
import importlib.util,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM
started=time.perf_counter();pe,canonical,config,originals,canonicals=indexes()
def word(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def put(u,a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
def string(u,a):return bytes(u.mem_read(a,100)).split(b'\0')[0]
rows=[]
for count in [0,1,19,20]:
    for existing in sorted(set([-1]+([0,count-1]if count else []))):
        for fail in [0,1]:
            u=Uc(UC_ARCH_X86,UC_MODE_32)
            for a,n in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
            for s in pe.sections:
                if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
            u.mem_write(0x7c954,bytes(336));requested=0x201000;u.mem_write(requested,b'requested.c\0')
            for i in range(count):
                address=0x200000+i*32;u.mem_write(address,('existing%d.c\0'%i).encode());put(u,0x7c954+i*16,address,0x1000+i,0x2000+i,10+i)
            put(u,0x7e1c8,count);put(u,0x7e1cc,0);put(u,0x7ca94,0xabc,0xdef,1,0x444)
            put(u,0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000+existing*32 if existing>=0 else requested)
            diagnostics=[];reports=[]
            def hook(m,a,size,unused):
                if a==0x400000:m.emu_stop();return
                if a not in [0x71fd0,0x70840,0x6fb70]:return
                stack=m.reg_read(UC_X86_REG_ESP);pop=4
                if a==0x71fd0:
                    assert string(m,word(m,stack+4))==b'requested.c'and string(m,word(m,stack+8))==b'rb'
                    m.reg_write(UC_X86_REG_EAX,0 if fail else 0x12345678)
                elif a==0x70840:
                    assert m.reg_read(UC_X86_REG_ECX)==0x53 and string(m,m.reg_read(UC_X86_REG_EDX))==b'source'and string(m,word(m,stack+4))==b'requested.c';reports.append(0x53);pop=8
                else:
                    assert m.reg_read(UC_X86_REG_EDX)==0x5ed and string(m,m.reg_read(UC_X86_REG_ECX))==b'@(#)listing.c:1.76'
                    diagnostics.append([int(word(m,0x7ca94)==requested),word(m,0x7ca98),word(m,0x7ca9c),word(m,0x7caa0)])
                m.reg_write(UC_X86_REG_EIP,word(m,stack));m.reg_write(UC_X86_REG_ESP,stack+pop)
            u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x27fc0,0,count=100000)
            head=[word(u,0x7e1c8),word(u,0x7e1cc),len(diagnostics),len(reports),*[word(u,a)for a in [0x7ca94,0x7ca98,0x7ca9c,0x7caa0]],*(diagnostics[0]if diagnostics else [0]*4)]
            lines=[' '.join(map(str,head))]
            for i in range(21):
                values=list(struct.unpack('<IIi',u.mem_read(0x7c958+16*i,12)));lines.append(' '.join(map(str,values)))
            native=subprocess.check_output(['build/native/check-listing-storage',str(count),str(existing),str(fail)],text=True).splitlines()
            assert native==lines,(count,existing,fail,native,lines)
            rows.append(dict(count=count,existing=existing,failed_mock_open=fail,output=lines))
# Actual original/canonical instruction operands bind all four live aliases to
# one storage object. Include address constructions and base/index accesses;
# this is not a raw absolute-address byte search.
spec=importlib.util.spec_from_file_location('listing_storage_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
base=g.FunctionDecoder;observed=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};observed.append(self)
    def instruction(self,a):
        item=super().instruction(a)
        if item is not None:self.visited[a]=item
        return item
g.FunctionDecoder=Decoder
storage=publics()['_listing_storage'];slots={0x7ca94:storage+320,0x7ca98:storage+324,0x7ca9c:storage+328,0x7caa0:storage+332}
def references(image,records,row,addresses):
    g.edges(image,records,row);d=observed[-1];found=set()
    for a,size,mn,ops in d.visited.values():
        if not any(hex(x)in ops for x in addresses):continue
        for op in d.detail(a,size).operands:
            value=op.mem.disp&0xffffffff if op.type==X86_OP_MEM else(op.imm&0xffffffff if op.type==X86_OP_IMM else None)
            if value in addresses:found.add(value)
    return found
right={r['name']:r for r in canonicals};bindings=[]
for row in config['driver']:
    if row['name']not in config['accepted']:continue
    used=references(pe,originals,row,slots)
    if not used:continue
    actual=references(canonical,canonicals,right[row['name']],set(slots.values()))
    assert {slots[x]for x in used}<=actual,(row['name'],used,actual)
    bindings.append(dict(function=row['name'],original=[hex(x)for x in sorted(used)],canonical=[hex(slots[x])for x in sorted(used)]))
assert {a for r in bindings for a in r['original']}=={hex(x)for x in slots}
prepass=right['@prepare_source_listing@4'];wrong={storage+336+320,storage+336+324}
assert not wrong<=references(canonical,canonicals,prepass,wrong), 'Split-storage negative unexpectedly matches prepass'
Path('cases/listing-storage-validation.json').write_text(json.dumps(dict(cases=rows,storage_symbol='_listing_storage',canonical_base=hex(storage),alias_offsets=[320,324,328,332],operand_bindings=bindings,negative='Rebinding cursor/head to separate zero storage outside the typed union fails actual prepass operands',seconds=time.perf_counter()-started,scope='Whole original/cache C control and typed storage identity with mocked successful/failed fopen and returning diagnostic capture. Includes row20 writes before overflow diagnostic; no real file/error-runtime behavior or general post-fatal continuation claim.'),indent=2)+'\n')
print(len(rows),'whole original/native cache states and',len(bindings),'function/storage identity bindings pass')
