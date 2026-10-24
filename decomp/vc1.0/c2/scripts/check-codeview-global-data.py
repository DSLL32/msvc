"""Observed global-data descriptor ABI and full packed global/TLS record controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();name='@write_codeview_global_data@4';row=next(r for r in config['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
expected=dict(identifier=116,bucket=52,index=0,kind=1,flags=2304,type=117,coff=0,byte_first=0,debug_offset=559,name_hex=b'_omf_unknown\0'.hex(),payload={})
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x69f30'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]and e['shape']==expected
 for side in ['original','native']:
  d=e[side];s=d['state'];assert s['symbol']==s['member']==e['formals'][0][side]and s['symbol']and s['debug']and all(s[k]==v for k,v in expected.items()if k!='payload')and d['payload']=={}
 assert e['original']['state']['return_address']==0x69a37 and e['native']['state']['caller']=='write_debug_symbol'and e['native']['state']['caller_symbol']==e['native']['state']['symbol']
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong hash member',lambda e:e['original']['state'].update(member=0)),('wrong typeWORD',lambda e:e['native']['state'].update(type=0)),('missing name',lambda e:e['original']['state'].update(name_hex='')),('wrong flags',lambda e:e['native']['state'].update(flags=0)),('wrong debug offset',lambda e:e['original']['state'].update(debug_offset=0)),('invented unused nonCOFF offset',lambda e:e['native'].update(payload=dict(offset=0))),('invented output record',lambda e:e.update(hidden_inputs=['record']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x69f30 and row['end']==0x69fef and original.read(0x69fee,1)==b'\xc3'and original.read(0x69f3c,4)==bytes.fromhex('66bb0202')and original.read(0x69f74,4)==bytes.fromhex('66bb0e02')
assert 'sizeof(CodeViewDataRecord) == 308'in Path('src/codeview-output.c').read_text()
if args.abi_only:print('Global CodeView actual sole/closed symbol/active fields/10 rejects and191-byte RET0/packed layout pass');raise SystemExit
def run(image,right,coff,byte_first,kind,flags,length,offset,type,mutate):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    put(names['_coff_object']if right else 0x806f4,coff);put(names['_byte_block_first']if right else 0x806f0,byte_first);put(names['_debug_symbols_segment']if right else 0x7914c,0x201000);put(0x201004,100)
    callbacks=names['_output_callbacks']if right else 0x7b8f0
    for off,target in [(40,0x400100),(36,0x400200),(16,0x400300)]:put(callbacks+off,target)
    u.mem_write(0x200000,bytes(50));u.mem_write(0x20000c,bytes([kind,0]));u.mem_write(0x20000e,struct.pack('<H',flags));put(0x200004,offset);put(0x200020,0xa5a50000|type);text=b'x'*length;u.mem_write(0x202000,text+b'\0')
    put(0x300800,0x400000);put(0x300804,0xfeedbeef);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000)
    saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
    for register,value in saved:u.reg_write(register,value)
    display=names['@symbol_display_name@4']if right else 0x4e000;diagnostic=names['@format_function_name@4']if right else 0x4df70;copy=names['@copy_codeview_string@8']if right else 0x69520;events=[];record=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [display,diagnostic,copy,0x400100,0x400200,0x400300]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0
        if a in [display,diagnostic]:assert ecx==0x200000 and a==diagnostic;events.append('name');result=0x202000
        elif a==copy:assert edx==0x202000;events.append('copy');m.mem_write(ecx,bytes([length])+text);m.mem_write(ecx+length+2,b'\0');result=length+1
        elif a==0x400100:
            assert ecx==0x201000;events.append('select')
            if mutate:
                put(0x200004,word(0x200004)+13);u.mem_write(0x20000e,struct.pack('<H',flags^0x1000));u.mem_write(0x200020,struct.pack('<H',type^16))
        elif a==0x400200:
            assert edx==length+13;events.append('data');actual=bytes(m.mem_read(ecx,edx));post_flags=flags^0x1000 if mutate else flags;post_offset=(offset+13)&0xffffffff if mutate else offset;post_type=type^16 if mutate else type
            expected=struct.pack('<HHIHHB',length+11,0x20e if kind==1 and post_flags&0x1000 else 0x202,post_offset if coff and not byte_first else 0,0,post_type,length)+text
            assert actual==expected;record.append(actual.hex())
        else:assert ecx==(2-(length+11))&0xffffffff and edx==0x200000;events.append('size')
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@write_codeview_global_data@4']if right else 0x69f30,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804 and word(0x300804)==0xfeedbeef and all(u.reg_read(r)==v for r,v in saved)
    return dict(events=events,record=record,segment_offset=word(0x201004))
rows=[]
for values in itertools.product([0,1],[0,1],[1,3],[0,0x1000],[0,11,247,255],[0,0xfffffffc],[0,4109,65535],[0,1]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(input=values,output=a))
Path('cases/codeview-global-data-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole191-byte original/canonical global data/TLS writer versus independent packed record model.768 states cover global kinds202/20e,COFF/byte-first flags,post-select descriptor flags/offset/typeWORD mutations,unsigned offset patterns,name lengths0/11/247/255 and exact count/name/size displacement/segment offset. Name/format/select/copy/data/size children explicitly mocked, full controls do not imply cold child execution. Saved registers/RET0/caller stack guards. Actual sole closed hash symbolID116/kind1/flags2304/type117/name_omf_unknown/coff0/debugoffset559 separately observed; unused nonCOFF source offset and descriptor tails are not ABI input/dumped/seeded. Names>255 original pre-truncation output-length behavior remains unexercised, no guard or normalization. Historical full models remain explicit, lightweight mode keeps actual negatives/extent/layout.'),indent=2)+'\n');print(len(rows),'whole global CodeView data controls/10 actual ABI rejects pass')
