"""Whole filename callback and explicit original/canonical state identities."""
from pathlib import Path
import copy,importlib.util,itertools,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM
from pe_indexes import indexes,publics
started=time.perf_counter();pe,canonical,config,left,right=indexes();symbols=publics();flag=symbols['_coff_emitted_function'];pending=symbols['_coff_has_function_lines']
assert struct.unpack('<I',pe.read(0x850c8,4))[0]==struct.unpack('<I',canonical.read(flag,4))[0]==1
native=subprocess.check_output(['build/native/check-coff-filename'],text=True).splitlines();assert native.pop(0)=='initial 1';rows=[]
for emitted,codeview,line,marker in itertools.product([-1,0,1],[-1,0,1],[-1,0,1],[0,1,65535]):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,value):u.mem_write(a,struct.pack('<I',value&0xffffffff))
    for a,value in [(0x850c8,emitted),(0x8313c,codeview),(0x83140,line),(0x7e8b4,0xa5a50000|marker),(0x300800,0x400000)]:put(a,value)
    u.mem_write(0x200000,b'dbglist.c\0');u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [0x6eda0,0x6dc00]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX)
        if a==0x6eda0:events.append('L%u;'%(ecx&65535));m.reg_write(UC_X86_REG_ECX,0xabcdef12);m.reg_write(UC_X86_REG_EDX,0xabcdef34)
        else:assert ecx==0x200000;events.append('F;')
        m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x6d3b0,0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804 and word(0x7e8b4)>>16==0xa5a5
    expected='%d %d %d %u %u %s'%(emitted,codeview,line,marker,word(0x7e8b4)&65535,''.join(events));actual=native[len(rows)];assert actual==expected,(actual,expected)
    rows.append(dict(emitted=emitted,codeview=codeview,line=line,pending_before=marker,result=expected))
assert len(rows)==len(native)==81
spec=importlib.util.spec_from_file_location('coff_filename_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;observed=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};observed.append(self)
    def instruction(self,a):
        item=super().instruction(a)
        if item is not None:self.visited[a]=item
        return item
g.FunctionDecoder=Decoder
def references(image,records,row,address):
    g.edges(image,records,row);d=observed[-1];refs=[]
    for a,size,mn,ops in d.visited.values():
        for operand in d.detail(a,size).operands:
            value=operand.mem.disp&0xffffffff if operand.type==X86_OP_MEM else operand.imm&0xffffffff if operand.type==X86_OP_IMM else None
            if value==address:refs.append(dict(site=hex(a),mnemonic=mn,type='memory'if operand.type==X86_OP_MEM else'address',width=operand.size))
    return refs
bindings=[]
for name in ['@coff_output_function@4','@coff_output_debug_pair@4','@coff_output_debug_record@8']:
    for label,a,b in [('emitted',0x850c8,flag),('pending',0x7e8b4,pending)]:
        original=references(pe,left,next(r for r in left if r['name']==name),a);rebuilt=references(canonical,right,next(r for r in right if r['name']==name),b)
        bindings.append(dict(function=name,storage=label,original_address=hex(a),canonical_address=hex(b),original=original,canonical=rebuilt))
def validate_bindings(values):
    assert len(values)==6
    for b in values:
        assert b['original']and b['canonical']
        assert b['canonical_address']==hex(flag if b['storage']=='emitted'else pending)
        assert all(r['width']==(4 if b['storage']=='emitted'else 2)for r in b['original']+b['canonical']if r['type']=='memory')
validate_bindings(bindings);rejected=[]
for label,change in [('same-valued separate emitted storage',lambda r:r[2].update(canonical_address=hex(flag+4))),('missing accepted def-use binding',lambda r:r[2].update(canonical=[])),('widened pending-word store',lambda r:r[3]['canonical'][0].update(width=4))]:
    bad=copy.deepcopy(bindings);change(bad)
    try:validate_bindings(bad)
    except AssertionError:rejected.append(label)
    else:raise AssertionError('Accepted bad COFF storage identity: '+label)
Path('cases/coff-filename-validation.json').write_text(json.dumps(dict(cases=rows,initial_emitted=1,bindings=bindings,rejected=rejected,seconds=time.perf_counter()-started,scope='Whole original6d3b0..6d3ec versus actual C for81 guard/WORD shapes with named line/filename child mocks. Original+canonical PE initializer1 and six accepted function/storage identities, including2-byte pending reads/writes, inspected through memory/address-construction operands. No whole-image arbitrary indirect-alias proof. Real first callback guard is false; child bodies/ABIs and cold execution are not accepted by these mocks.'),indent=2)+'\n');print('81 callback shapes, PE initializer1 and6 storage identities/3 negatives pass')
