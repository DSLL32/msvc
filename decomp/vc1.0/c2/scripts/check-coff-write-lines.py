"""COFF line writer: actual two-input ABI and whole-body circular-list shapes."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument("--abi-only",action="store_true");args=ap.parse_args()
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();name='@coff_write_lines@8';row=next(r for r in config['driver']if r['name']==name);path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text());sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();assert receipt['binding']==binding and all(sha(path.parent/n)==v for n,v in binding['files'].items())
def validate(r):
    assert r['matched']and r['function']==name and r['original_va']=='0x6f040'and r['no_hidden_inputs_proof']
    assert r['hidden_inputs']==row['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
    assert [{k:p[k]for k in ['name','location','type']}for p in r['formals']]==row['abi']['parameters']
    assert r['formals'][1]['value']==0
    shapes=[]
    for side,formal in [(r['original'],r['formals'][0]['original']),(r['native'],r['formals'][0]['native'])]:
        assert formal==side['pointer']==side['state']['owner']and formal and side['base']==0
        raw=bytes.fromhex(side['contribution_hex']);assert len(raw)==56
        assert struct.unpack_from('<I',raw,4)[0]==side['state']['section']and struct.unpack_from('<I',raw,20)[0]==side['state']['last']
        blocks=side['blocks'];assert len(blocks)==1 and blocks[0]['count']==26 and len(bytes.fromhex(blocks[0]['hex']))==156 and blocks[0]['next']==blocks[0]['address']==side['state']['last']
        assert [a['index']for a in side['aux']]==[12,23]and all(a['entry']and a['auxiliary']and len(bytes.fromhex(a['hex']))==18 for a in side['aux'])
        shapes.append(dict(debug=side['state']['debug'],free_null=not side['state']['free_head'],position=struct.unpack_from('<I',raw,44)[0],records=blocks[0]['hex']))
    assert shapes[0]==shapes[1]and shapes[0]['debug']&1
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate(receipt);rejected=[]
for label,change in [('missing base input',lambda r:r['formals'].pop()),('wrong base register',lambda r:r['formals'][1].update(location='entryESP+4')),('wrong base value',lambda r:r['native'].update(base=1)),('wrong contribution owner',lambda r:r['native']['state'].update(owner=0)),('broken circular list',lambda r:r['native']['blocks'][0].update(next=0)),('missing defined line byte',lambda r:r['native']['blocks'][0].update(hex='')),('wrong auxiliary vector index',lambda r:r['native']['aux'][0].update(index=11)),('unobserved hidden parameter',lambda r:r.update(hidden_inputs=['unknown']))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted bad COFF line ABI '+label)
assert row['start']==0x6f040 and row['end']==0x6f108 and original.read(0x6f107,1)==b'\xc3'
if args.abi_only:print('COFF line actual2-input ABI,8 rejects,extent and defined contribution/ring/aux storage pass');raise SystemExit
fields={'coff_debug':0x7905c,'coff_symbol_entries':0x851a8,'coff_file':0x79064,'coff_error_handler':0x790f4,'coff_free_lines':0x851cc}
def run(image,is_canonical,counts,debug,base,short,mutate):
    globals={n:names['_'+n]if is_canonical else a for n,a in fields.items()};u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x20000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    for n,v in [('coff_debug',debug),('coff_symbol_entries',0x202000),('coff_file',0x202100),('coff_error_handler',0x400100),('coff_free_lines',0x209000)]:put(globals[n],v)
    blocks=[0x201000+0x200*i for i in range(len(counts))];u.mem_write(0x200000,b'\x5a'*56);put(0x200014,blocks[-1]);put(0x20002c,0xfffffffc)
    for k,(block,count)in enumerate(zip(blocks,counts)):
        u.mem_write(block,b'\xa5'*308);put(block,count,blocks[(k+1)%len(blocks)])
        for j in range(count):u.mem_write(block+8+6*j,struct.pack('<IH',10+100*k+j,0 if j%2==0 else 0xffff))
    for i in range(500):put(0x203000+8*i,4,0x206000+20*i);u.mem_write(0x206000+20*i,b'\x3c'*18)
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);u.reg_write(UC_X86_REG_EDX,base)
    get=names['@coff_vector_get@8']if is_canonical else 0x6e780;write=names['_fwrite']if is_canonical else 0x72c30;events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [get,write,0x400100]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0;pop=4
        if a==get:
            assert ecx==0x202000 and 0<edx<500;events.append(['aux',edx]);result=0x203000+edx*8
            if mutate:put(globals['coff_debug'],word(globals['coff_debug'])^1)
        elif a==write:
            pointer,size,count,file=[word(esp+i)for i in [4,8,12,16]];assert size==6 and file==0x202100;events.append(['write',count,bytes(m.mem_read(pointer,6*count)).hex()]);result=count-1 if count and short else count
            if mutate:put(globals['coff_debug'],word(globals['coff_debug'])^1);put(globals['coff_free_lines'],0x209100)
        else:
            assert ecx==14 and word(esp+4)==0x9d3 and bytes(m.mem_read(edx,64)).split(b'\0')[0]==b'd:\\usr\\cuda\\src\\P2\\coffemit.c';events.append(['error',14,0x9d3]);pop=8
            if mutate:put(globals['coff_debug'],word(globals['coff_debug'])^1);put(globals['coff_free_lines'],0x209200)
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+pop)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if is_canonical else row['start'],0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,blocks=[bytes(u.mem_read(b,308)).hex()for b in blocks],contribution=bytes(u.mem_read(0x200000,56)).hex(),aux=bytes(u.mem_read(0x206000,20*500)).hex(),free_head=word(globals['coff_free_lines']),debug=word(globals['coff_debug']))
rows=[]
for counts,debug,base,short,mutate in itertools.product([(0,),(1,),(3,),(50,0),(2,3)],[0,1,0x100,0x101],[0,0xfffffffe],[0,1],[0,1]):
    a=run(original,False,counts,debug,base,short,mutate);b=run(canonical,True,counts,debug,base,short,mutate);assert a==b,(counts,debug,base,short,mutate,a['events'],b['events']);rows.append(dict(counts=counts,debug=debug,base=base,short=short,mutate=mutate,events=a['events']))
Path('cases/coff-write-lines-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,cases=rows,seconds=time.perf_counter()-started,scope='Actual two formals and26 defined records/circular list/auxiliary output ownership.160 whole original/canonical controls: null count, partial/full/multiple rings, debug low-byte bit0, linezero handles versus rebased addresses, wraparound position/base, exact six-byte writes, short-write diagnostic request and current free-list splice. Vector/fwrite/error children explicitly mocked, including global changes across call boundaries; mock short-write control does not accept actual CRT failure behavior. All308 block bytes/auxiliary tails checked unchanged except original writes. Real fresh frontier and complete artifact oracle separate.'),indent=2)+'\n');print(len(rows),'whole original/canonical COFF line writes,8 ABI rejects match')
