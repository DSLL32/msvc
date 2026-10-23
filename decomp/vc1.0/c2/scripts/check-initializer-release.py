"""Initializer release body, actual ABI evidence and single free-list storage."""
from pathlib import Path
import copy,hashlib,json,struct,time
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86_const import X86_OP_MEM
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();name='@release_initializer@4';row=next(r for r in config['driver']if r['name']==name);path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text());sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();assert receipt['binding']==binding and all(sha(path.parent/n)==v for n,v in binding['files'].items())
def validate(r):
    assert r['matched']and r['function']==name and r['original_va']=='0x4e8e0'and r['no_hidden_inputs_proof']
    assert r['hidden_inputs']==row['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
    assert [{k:p[k]for k in ['name','location','type']}for p in r['formals']]==row['abi']['parameters']==[dict(name='initializer',location='ECX',type='Initializer pointer')]
    for side,formal in [(r['original_list'],r['formals'][0]['original']),(r['native'],r['formals'][0]['native'])]:
        assert formal==side['head']==side['nodes'][0]['address']and formal and side['free_head']==0 and side['free_hex']==''
        nodes=side['nodes'];assert len(nodes)==3 and len({n['address']for n in nodes})==3
        for i,n in enumerate(nodes):
            w=struct.unpack('<7I',bytes.fromhex(n['hex']));assert w[1]==n['next']==(nodes[i+1]['address']if i+1<len(nodes)else 0)and w[2]==n['link']
            assert w[0]==[10,1,1][i]
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate(receipt);rejected=[]
for label,change in [('missing pointer formal',lambda r:r.update(formals=[])),('wrong register',lambda r:r['formals'][0].update(location='EDX')),('broken head identity',lambda r:r['native'].update(head=0)),('broken next chain',lambda r:r['native']['nodes'][0].update(next=0)),('different free-head identity',lambda r:r['native'].update(free_head=1)),('unobserved hidden argument',lambda r:r.update(hidden_inputs=['unknown']))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad initializer ABI '+label)
# The existing static object is deliberately not exported merely for a test.
# Its sole direct absolute operand in new_initializer binds allocation and
# release to one object; no second same-zero global can satisfy this identity.
def free_storage(image,start,end):
    d=Cs(CS_ARCH_X86,CS_MODE_32);d.detail=True;values=set()
    for ins in d.disasm(image.read(start,end-start),start):
        for op in ins.operands:
            if op.type==X86_OP_MEM and not op.mem.base and not op.mem.index:values.add(op.mem.disp&0xffffffff)
    assert len(values)==1,values
    return values.pop()
a=next(r for r in left if r['name']=='@new_initializer@4');b=next(r for r in right if r['name']=='@new_initializer@4');storage=[free_storage(original,a['start'],a['end']),free_storage(canonical,b['start'],b['end'])];assert storage[0]==0x7c144
for image,records,address in [(original,left,storage[0]),(canonical,right,storage[1])]:
    f=next(r for r in records if r['name']==name);assert free_storage(image,f['start'],f['end'])==address
    section=next(s for s in image.sections if image.base+s['rva']<=address<image.base+s['rva']+s['virtual_size']);assert section['flags']&0x80 or image.read(address,4)==bytes(4)
def run(image,canonical_side,count,free_kind,poison):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    put=lambda a,v:u.mem_write(a,struct.pack('<I',v&0xffffffff));word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
    nodes=[0x200000+i*32 for i in range(count)];before=[]
    for i,p in enumerate(nodes):
        words=[0x1010101*(j+1)+poison+i for j in range(7)];words[1]=nodes[i+1]if i+1<count else 0;raw=struct.pack('<7I',*words);u.mem_write(p,raw);before.append(raw)
    free=0 if free_kind==0 else 0x208000 if free_kind==1 or not nodes else nodes[-1];u.mem_write(0x208000,b'Z'*64);free_before=bytes(u.mem_read(0x208000,64));put(storage[int(canonical_side)],free);put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,nodes[0]if nodes else 0)
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop()
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if canonical_side else row['start'],0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    after=[bytes(u.mem_read(p,28))for p in nodes]
    for i,(a,b)in enumerate(zip(before,after)):
        expected=bytearray(a);struct.pack_into('<I',expected,8,nodes[i-1]if i else free);assert b==expected
    assert word(storage[int(canonical_side)])==(nodes[-1]if nodes else free)and bytes(u.mem_read(0x208000,64))==free_before
    return dict(nodes=[n.hex()for n in after],free_head=word(storage[int(canonical_side)]))
rows=[]
for count in [0,1,3,8,64]:
 for free_kind in [0,1,2]:
  for poison in [0,0x111000]:
   a=run(original,False,count,free_kind,poison);b=run(canonical,True,count,free_kind,poison);assert a==b;rows.append(dict(count=count,free_kind=free_kind,poison=poison,free_head=a['free_head']))
Path('cases/initializer-release-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,static_storage=dict(original=hex(storage[0]),canonical=hex(storage[1])),controls=rows,seconds=time.perf_counter()-started,scope='Actual ECX input chain3 kinds10/1/1 and empty free-head, raw28-byte nodes retained; only consumed topology/identity equated, unused payload pointer bytes not normalized.30 original/canonical whole-body controls include null/single/long chains, empty/external/aliased prior heads and poisoned fields. Exact+8 writes, reversed free chain, preserved+4/other node bytes/external head, RET0 and single allocator/release static storage identity checked. No children mocked; host observation bound is not a function guard. No cyclic-next input equivalence claimed.'),indent=2)+'\n');print('Actual initializer ABI,6 rejects, single storage identity and30 whole-body controls pass')
