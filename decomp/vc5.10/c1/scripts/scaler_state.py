"""Actual x87/QWORD conversion evidence; initial state and allocation are separate."""
import copy,json,re,struct
from fractions import Fraction
from pathlib import Path
from provenance import sha,captured_native_binary
from allocation_state import symbols,call
from allocation_imports import export_rva,pe_read

def value80(raw):
    data=bytes.fromhex(raw);assert len(data)==10
    significand,sign_exponent=struct.unpack('<QH',data);exponent=sign_exponent&0x7fff
    assert exponent not in (0,0x7fff) or significand==0
    if not significand:return Fraction(0)
    assert significand>>63==1
    power=exponent-16383-63
    value=Fraction(significand<<power)if power>=0 else Fraction(significand,1<<(-power))
    return -value if sign_exponent&0x8000 else value

def original_fp(directory,sequence):
    raw=(directory/('fpu-%03d.bin'%sequence)).read_bytes();assert len(raw)==112
    cw,sw,tag=(x&0xffff for x in struct.unpack_from('<3I',raw));top=(sw>>11)&7
    registers=[raw[28+i*10:38+i*10].hex()for i in range(8)]
    return dict(cw=cw,sw=sw,tag=tag,top=top,registers=registers,
        live=[dict(logical=i,tag=(tag>>(2*((top+i)&7)))&3,raw80=registers[i])for i in range(8)if ((tag>>(2*((top+i)&7)))&3)!=3])

def semantic(fp):
    return {key:fp[key]for key in ('cw','sw','tag','top','live')}

def compare(original,native):
    events=(original/'events.txt').read_text();candidate=json.loads((original/'candidate.json').read_text())
    binary=captured_native_binary(native);names=symbols(binary)
    metadata=json.loads((native/'scaler-probe-metadata.json').read_text())
    from scaler_native_probe import describe
    assert metadata==describe(binary)
    assert (metadata['start'],metadata['size'])==names['arena_reservation_bytes']
    crt=original/'loaded-msvcrt.bin'
    assert sha(crt)=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
    assert export_rva(crt,'_ftol')==0x4dc1
    assert pe_read(original/'loaded-target.bin',0x5b4d0,8)==struct.pack('<d',-0.01)
    assert (original/'ftol-provider-code-000.bin').read_bytes()==pe_read(crt,0x4dc1,39)
    assert events.count('ftol-provider iat=0005d0cc resolved=78004dc1\n')==1
    records=json.loads((native/'scaler-fpu.json').read_text());assert [r['label']for r in records]==['instruction-%d'%i for i in range(9)]+['converted']
    assert [i['mnemonic']for i in metadata['instructions']]==['fild','fild','fmulp','fld','fmulp','fnstcw','fldcw','fistp','fldcw']
    for instruction,record in zip(metadata['instructions'],records):assert instruction['pc']==record['pc']
    assert records[-1]['pc']==metadata['converted_pc']
    def at(rva):
        rows=re.findall(r'^fpu seq=(\d+) rva=%08x eax=([0-9a-f]+) edx=([0-9a-f]+) esp=([0-9a-f]+) cw=([0-9a-f]+) sw=([0-9a-f]+) tag=([0-9a-f]+)$'%rva,events,re.M);assert len(rows)==1
        row=rows[0];sequence=int(row[0]);fp=original_fp(original,sequence)
        assert [int(x,16)&0xffff for x in row[4:]]==[fp['cw'],fp['sw'],fp['tag']]
        return sequence,fp,int(row[1],16),int(row[2],16)
    entry_seq,entry_fp,_,_=at(0x12a7a)
    source_data=(original/('compiler-data-%03d.bin'%entry_seq)).read_bytes();percent=struct.unpack_from('<i',source_data,0x1978)[0]
    native_entries=json.loads((native/'scaler-entries.json').read_text());native_returns=json.loads((native/'scaler-returns.json').read_text());assert len(native_entries)==len(native_returns)==1
    entry=native_entries[0];returned=native_returns[0]
    assert entry['pc']==names['arena_reservation_bytes'][0] and entry['descriptor']==names['c1_arena_descriptors'][0]
    assert entry['formal_inventory']==[dict(name='descriptor',type='CompilerArenaDescriptor *')]
    assert entry['compiler_state']['initialized']['memory_percent']==percent
    descriptor=entry['compiler_state']['arena_descriptors'][0];assert descriptor==dict(kind=0,reserve_bytes=0x3200000,commit_unit=0x10000,flags=0x21,alignment=1)
    assert entry_fp['cw']==entry['fp']['cw']==0x027f and entry_fp['tag']==entry['fp']['tag']==0xffff
    assert entry_fp['live']==entry['fp']['live']==[] and entry_fp['top']==entry['fp']['top']==0
    comparisons=[]
    for rva,index,label in [(0x12a7f,0,'before signed load'),(0x12a85,1,'after signed reserve load'),(0x12a8b,3,'after signed percentage multiplication'),(0x12aac,5,'before QWORD conversion'),(0x12a96,9,'after QWORD conversion')]:
        _,old,_,_=at(rva);new=records[index]['fp']
        without_sw=lambda fp:{k:v for k,v in semantic(fp).items()if k!='sw'}
        assert without_sw(old)==without_sw(new)
        comparisons.append(dict(label=label,original_rva=rva,native_pc=records[index]['pc'],architectural_state_match=semantic(old)==semantic(new),original_status=old['sw'],native_status=new['sw']))
    _,loaded,_,_=at(0x12a85);_,product,_,_=at(0x12a8b);_,negative,_,_=at(0x12aac)
    assert len(loaded['live'])==len(product['live'])==len(negative['live'])==1
    assert value80(loaded['registers'][0])==descriptor['reserve_bytes']
    assert value80(product['registers'][0])==descriptor['reserve_bytes']*percent
    # Extra native temporaries occupy slots that the original empty input owns
    # as empty. Their exact operands and stack depths are observed, not assumed.
    assert [len(records[i]['fp']['live'])for i in (0,1,2,3,4,5)]==[0,1,2,1,2,1]
    assert value80(records[2]['fp']['registers'][0])==percent
    assert records[2]['fp']['registers'][1]==loaded['registers'][0]
    assert value80(records[4]['fp']['registers'][0])==Fraction.from_float(-0.01)
    assert records[4]['fp']['registers'][1]==product['registers'][0]
    for index in (2,4):
        temporary=records[index]['fp'];previous=records[index-1]['fp']
        assert temporary['cw']==previous['cw']==0x027f
        assert temporary['sw']&~0x3800==previous['sw']&~0x3800
        assert temporary['top']==6 and temporary['tag']==0x0fff
        assert [row['tag']for row in temporary['live']]==[0,0]
    assert records[6]['fp']['cw']==0x027f and records[7]['fp']['cw']==records[8]['fp']['cw']==0x0e7f
    converted=records[-1];assert converted['qword_address']==converted['ebp']+metadata['result_ebp_displacement']
    raw=bytes.fromhex(converted['qword_bytes']);assert len(raw)==8;qword=struct.unpack('<Q',raw)[0]
    _,after_fp,eax,edx=at(0x12a96);assert qword==(edx<<32)|eax
    signed=struct.unpack('<q',raw)[0];assert signed==int(value80(negative['registers'][0]))
    unit=descriptor['commit_unit'];rounded=(unit-(qword&0xffffffff)-1)&(~(unit-1)&0xffffffff)
    if not rounded:rounded=unit
    return_seq,return_fp,old_return,_=at(0x12a29)
    assert returned['eax']==old_return==rounded
    caller_start,caller_size=names['_ZN13CompilerArena10InitializeEP23CompilerArenaDescriptorPv']
    assert caller_start<=returned['pc']<caller_start+caller_size
    call(binary,returned['pc'],names['arena_reservation_bytes'][0])
    assert semantic(after_fp)==semantic(converted['fp'])==semantic(return_fp)==semantic(returned['fp'])
    assert after_fp['live']==[]
    for record in records+[returned]:assert record['compiler_state']==entry['compiler_state']
    for rva in (0x12a7f,0x12a85,0x12a8b,0x12aac,0x12a96,0x12a29):
        at_sequence=at(rva)[0]
        for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
            assert(original/('%s-%03d.bin'%(region,at_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,entry_seq))).read_bytes()
    initial=json.loads((native/'initial-x87.json').read_text());assert initial and initial[0]['pc']==names['_DllMainCRTStartup'][0]
    assert initial[0]['args'][1:]==[1,0]
    original_initial=original_fp(original,0)
    return dict(status='conversion values/final state match; whole case held on host-input/allocation support',acceptance=False,
        original_receipt_sha256=sha(original/'receipt.json'),native_receipt_sha256=sha(native/'receipt.json'),
        input=dict(reserve_bytes=descriptor['reserve_bytes'],memory_percent=percent,commit_unit=unit),
        signed_qword=signed,qword_hex='%016x'%qword,rounded_return=rounded,
        initial_state_match=semantic(original_initial)==semantic(initial[0]['fp']),
        original_initial=semantic(original_initial),native_initial=semantic(initial[0]['fp']),
        comparisons=comparisons,final_state_match=True,temporary_depths=[0,1,2,1,2,1],
        original_return_sequence=return_seq,captured_native_elf_sha256=sha(binary))

def boundary(original,native,state,sequence,module):
    events=(original/'events.txt').read_text()
    rows=re.findall(r'^trap seq=(\d+) rva=00012a7a .*stack=([0-9a-f,]+)$',events,re.M);assert len(rows)==1
    at=int(rows[0][0]);assert at<sequence
    entries=json.loads((native/'scaler-entries.json').read_text());assert len(entries)==1
    entry=copy.deepcopy(entries[0]);entry.update(entry['crt_state'])
    assert entry['invocation']==state['invocation'] and entry['args']=={'descriptor':entry['descriptor']}
    return entry,at

def validate(original,native,entry,state,entry_sequence,sequence,module,candidate):
    assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])==('arena-initialized',0x12ab2)
    result=compare(original,native)
    assert result['initial_state_match'] and result['final_state_match']
    assert all(item['architectural_state_match'] for item in result['comparisons'])
    binary=captured_native_binary(native);names=symbols(binary)
    initial=json.loads((native/'initial-x87.json').read_text());assert len(initial)==1
    reference=sha(original/'receipt.json');binding=json.loads((native/'x87-entry-binding.json').read_text())
    assert initial[0]['profile']==dict(reference=reference,applications=1)
    assert binding['original_receipt_sha256']==reference and binding['original_directory']==str(original.resolve())
    assert binding['original_float_save_sha256']==sha(original/'fpu-000.bin')
    assert binding['original_environment_sha256']==sha(original/'invoke-environment.bin')
    assert (binding['guard_rva'],binding['cw'],binding['sw'],binding['tag'],binding['top'],binding['live_registers'])==(0x1202a,0x027f,0x0020,0xffff,0,[])
    assert initial[0]['args']==[state['module_token'],1,0]
    assert initial[0]['return'] and initial[0]['esp']
    events=(original/'events.txt').read_text()
    before=re.findall(r'^arena-allocation-before seq=(\d+) esp=([0-9a-f]+) address=([0-9a-f]+) bytes=([0-9a-f]+) kind=([0-9a-f]+) protection=([0-9a-f]+) provider=([0-9a-f]+) error=(\d+)$',events,re.M)
    after=re.findall(r'^arena-allocation-after seq=(\d+) esp=([0-9a-f]+) before-esp=([0-9a-f]+) result=([0-9a-f]+) error=(\d+) before-error=(\d+)$',events,re.M)
    assert len(before)==len(after)==1;b,a=before[0],after[0];bs,ats=int(b[0]),int(a[0])
    assert entry_sequence<bs<ats<sequence
    size=result['rounded_return'];assert [int(x,16)for x in b[2:6]]==[0,size,0x102000,4]
    assert int(a[1],16)==int(b[1],16)+16 and int(a[2],16)==int(b[1],16)
    assert int(b[7])==int(a[4])==int(a[5])==0
    kernel=int(re.findall(r'^module name=KERNEL32.dll actual=([0-9a-f]+) ',events,re.M)[0],16)
    assert int(b[6],16)==kernel+export_rva(original/'loaded-kernel32.bin','VirtualAlloc')
    original_base=int(a[3],16);assert original_base and original_base%0x10000==0
    query=re.findall(r'^arena-region base=([0-9a-f]+) allocation=([0-9a-f]+) allocation-protect=([0-9a-f]+) bytes=([0-9a-f]+) state=([0-9a-f]+) protect=([0-9a-f]+) type=([0-9a-f]+)$',events,re.M)
    assert [tuple(int(x,16)for x in row)for row in query]==[(original_base,original_base,4,size,0x2000,0,0x20000)]
    entries=json.loads((native/'arena-api-entries.json').read_text());returns=json.loads((native/'arena-api-returns.json').read_text());assert len(entries)==len(returns)==1;nb,na=entries[0],returns[0]
    assert nb['pc']==names['recon_VirtualAlloc'][0] and nb['args']==[0,size,0x102000,4]
    assert nb['formal_inventory']==[dict(name=name,type=kind)for name,kind in [('address','LPVOID'),('bytes','DWORD'),('kind','DWORD'),('protection','DWORD')]]
    assert na['pc']==nb['return'] and na['esp']==nb['esp']+20
    call(binary,na['pc'],nb['pc']);assert nb['preserved']==na['preserved']
    assert nb['last_error']==na['last_error']==0 and semantic(nb['fp'])==semantic(na['fp'])
    native_base=na['eax'];assert native_base and native_base%0x10000==0 and native_base+size<=0x100000000
    regions=[]
    for line in na['maps'].splitlines():
        row=re.match(r'^([0-9a-f]+)-([0-9a-f]+) ([-rwxps]+)\s',line);assert row
        start,end=int(row[1],16),int(row[2],16)
        if start<native_base+size and end>native_base:regions.append((max(start,native_base),min(end,native_base+size),row[3]))
    assert regions and regions[0][0]==native_base and regions[-1][1]==native_base+size
    assert all(x[1]==y[0]for x,y in zip(regions,regions[1:])) and all(row[2]=='---p'for row in regions)
    expected=copy.deepcopy(entry['compiler_state']);expected['bss']['arenas'][0]['reserve_bytes']=size
    assert nb['compiler_state']==na['compiler_state']==expected
    expected['bss']['arenas'][0]['base']=native_base
    assert state['compiler_state']==expected
    for region in ('compiler-data','compiler-snapshot','state'):
        assert all((original/('%s-%03d.bin'%(region,index))).read_bytes()==(original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()for index in (bs,ats,sequence))
    bss=(original/('compiler-bss-%03d.bin'%entry_sequence)).read_bytes();wanted=bytearray(bss);struct.pack_into('<I',wanted,0x8fc4,size)
    assert all((original/('compiler-bss-%03d.bin'%index)).read_bytes()==bytes(wanted)for index in (bs,ats))
    struct.pack_into('<I',wanted,0x8fb8,original_base)
    assert (original/('compiler-bss-%03d.bin'%sequence)).read_bytes()==bytes(wanted)
    assert state['symbol']=='CompilerArena::Reset' and state['args']=={'this':names['c1_bss'][0]+804}
    assert state['formal_inventory']==[dict(name='this',type='CompilerArena *')]
    assert state['caller']['symbol']=='CompilerArena::Initialize' and state['caller']['args']==entry['caller']['args']
    reset=json.loads((native/'arena-reset-entries.json').read_text());assert len(reset)==1;r=reset[0]
    assert r['pc']==names['_ZN13CompilerArena5ResetEv'][0] and r['ecx']==state['args']['this']
    assert r['formal_inventory']==state['formal_inventory'] and r['compiler_state']==state['compiler_state']
    assert r['return']==state['caller']['pc'];call(binary,r['return'],r['pc'])
    rows=re.findall(r'^trap seq=%d rva=00012ab2 esp=([0-9a-f]+) eax=[0-9a-f]+ ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(rows)==1
    assert int(rows[0][1],16)==module+0x58fb0 and int(rows[0][2].split(',')[0],16)==module+0x12a70
    snapshot=(original/'relocated-text.bin').read_bytes()
    assert snapshot[0x12a6b-0x1000:0x12a70-0x1000]==b'\xe8\x42\0\0\0'
    # Reset is still a named trap. Its real RET0 is ABI evidence, not code credit.
    assert pe_read(original/'loaded-target.bin',0x12ac2,1)==b'\xc3'
    assert semantic(r['fp'])==semantic(na['fp'])
    for key,value in entry['crt_state'].items():assert state[key]==value
    return result
