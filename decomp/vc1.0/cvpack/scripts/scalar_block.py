"""Only reviewed98 first callback scalar effect + separate real next entry."""
import hashlib,json,re
from state import BINARY,sha

def validate(config,pe,rebuilt=None,linked=None):
    record=config['scalar_block_observations'];path=BINARY/record['path'];assert sha(path)==record['sha256'],'scalar98 metadata changed';p=json.loads(path.read_text());assert p['schema']==1 and p['function']=='begin_symbol_block'and p['sequence']==98 and p['scope']=='first-module10-zero-suppression-callback-only','unknown scalar slice policy'
    assert p['entry']==0x25450 and p['return']==0x25172 and p['next']==0x28240 and p['allowed_effects']=={'symbol_scope_depth':[0,1]},'scalar slice effects/scope changed'
    for name,digest in p['sources'].items():assert sha(BINARY/name)==digest,('scalar proof source changed',name)
    for item in p['instructions']:assert hashlib.sha256(pe.read(item['start'],item['end']-item['start'])).hexdigest()==item['sha256'],'scalar original instruction proof changed'
    prior=BINARY/p['prior_module3']['path'];assert sha(prior)==p['prior_module3']['sha256']and json.loads(prior.read_text())['sequence']==97,'prior full module3 proof missing/changed'
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    md=Cs(CS_ARCH_X86,CS_MODE_32);body=list(md.disasm(pe.read(0x25450,33),0x25450))
    assert [(r.mnemonic,r.op_str)for r in body]==[('inc','dword ptr [0x335ec]'),('cmp','dword ptr [0x335e8], 0'),('je','0x25470'),('mov','eax, dword ptr [0x335e4]'),('mov','word ptr [eax + 2], 8'),('inc','dword ptr [0x335e8]'),('ret','')],'98 complete original body changed'
    assert pe.read(0x25170,2)==b'\xff\xd0'and pe.read(0x381b0,8)==bytes.fromhex('0602000050540200'),'callback selection/call proof changed'
    assert pe.read(0x17326,5)[0]==0xe8 and 0x1732b+int.from_bytes(pe.read(0x17327,4),'little',signed=True)==0x28240,'main next caller changed'
    assert next(r for r in config['functions']if r['name']=='begin_symbol_block')['parameters']==[],'98 ABI changed'
    next_f=next(r for r in config['functions']if r['name']=='strip_debug');assert next_f['parameters']==[dict(name='handle',slot=1,kind='integer',bits=32)],'next ABI observation missing'
    line=next(i for i,s in enumerate((BINARY/'src/types.c').read_text().splitlines(),1)if 'if (conversion->convert) conversion->convert();'in s);assert p['native_caller_line']==line,'native callback callsite source changed'
    if rebuilt is not None:
        a=linked['_begin_symbol_block'];end=min(x for x in linked.values()if x>a);body=[r for r in md.disasm(rebuilt.read(a,end-a),a)if r.mnemonic!='nop']
        assert [r.mnemonic for r in body]==['push','mov','push','push','push','inc','cmp','je','mov','mov','inc','pop','pop','pop','leave','ret'],'canonical complete callback shape differs'
        assert body[5].op_str=='dword ptr [0x%x]'%linked['_symbol_scope_depth']and body[6].op_str=='dword ptr [0x%x], 0'%linked['_suppressed_symbol_depth']and body[7].op_str=='0x%x'%body[11].address,'canonical no-heap zero branch changed'
        assert body[8].op_str=='eax, dword ptr [0x%x]'%linked['_symbol_input']and body[9].op_str=='word ptr [eax + 2], 8'and body[10].op_str=='dword ptr [0x%x]'%linked['_suppressed_symbol_depth'],'canonical suppressed branch changed'
    return p

def context(events,native):
    rows=re.findall(r'^progress-input (.*)$',events,re.M);assert len(rows)==1,'missing/duplicate actual entry context';words=rows[0].split();assert words[::2]==['module','module_address','input','cursor','capacity','directory','offset','size','cvbase','read_offset','output','output_cursor','bytes'];hexfields=('module_address','input','cursor','directory','output','output_cursor');old={k:v if k=='bytes'else int(v,16 if k in hexfields else 10)for k,v in zip(words[::2],words[1::2])}
    master=(BINARY/'cases/hello-debug-dll/input/hello-debug.dll').read_bytes()
    for row in(old,native):
        assert (row['module'],row['offset'],row['size'],row['capacity'],row['cvbase'],row['read_offset'])==(10,4754,43,4746,41972,4797),'scalar entry source/owner differs'
        assert row['cursor']==row['input']+4 and row['output_cursor']==row['output']+4,'scalar entry owned cursors differ'
        assert bytes.fromhex(row['bytes'])==master[46726:46769],'entry actual input differs from frozen DLL'
    assert native['callback']==dict(kind=518,length=35,index=20)and native['parameters']=={}and native['frames'][:2]==['begin_symbol_block','pack_new_types'],'scalar actual callback selection/ABI differs'
    return old

def compare(config,pe,events,native):
    p=validate(config,pe)
    keys=set('signal frames parameters scope_depth suppressed_depth module module_address input cursor capacity directory offset size cvbase read_offset output output_cursor bytes callback later_heap_equality before thread caller_pc caller_fp entry_pc function_address call_bytes caller_line return_pc return_fp return_thread return_function after conversion conversion_base conversion_kind conversion_target conversion_index'.split())
    assert set(native)==keys and native['signal']==0,'unapproved98 observation schema/mode/effects'
    entry=re.findall(r'^scalar-block entry (.*)$',events,re.M);ret=re.findall(r'^scalar-block return (.*)$',events,re.M);target=re.findall(r'^scalar-block target (.*)$',events,re.M)
    assert len(entry)==len(ret)==len(target)==1,'mandatory single actual callback entry/return/target absent'
    a=[int(x,16)for x in entry[0].split()];b=[int(x,16)for x in ret[0].split()];assert len(a)==len(b)==8 and a[0]!=0 and a[1]==0x25450 and b[1]==0x25172 and b[0]==a[0]and b[2]==a[2]+4 and b[3]==a[3],'original callback invocation/thread/return/frame mismatch'
    assert [int(x,16)for x in target[0].split()]==[0x25450,0x381b0,0x206,0x25450],'actual original callback target differs'
    assert a[5:7]==[0,0]and b[4:]==[a[4],0,1,a[7]],'original callback effects differ'
    assert re.findall(r'^exception ([0-9a-f]+) va ([0-9a-f]+) ',events,re.M)==[('80000003','00025172')]and 'observed-scalar-completion 00025172 calls 1\n'in events,'scalar checkpoint not genuine actual return'
    old=context(events,native);assert a[4]==old['cursor']and a[7]==old['output_cursor'],'original entry pointers not bound to actual owned inputs'
    assert native['before']==[native['cursor'],0,0,native['output_cursor']]and native['after']==[native['cursor'],0,1,native['output_cursor']],'native scalar effects differ'
    assert native['scope_depth']==native['suppressed_depth']==0 and native['later_heap_equality']is False,'unapproved native entry/scope claim'
    assert native['return_function']=='pack_new_types'and native['return_pc']==native['caller_pc']and native['return_fp']==native['caller_fp']and native['caller_line']==p['native_caller_line'],'native callback actual return/frame/site mismatch'
    assert len(native['thread'])==3 and native['thread'][0]>0 and native['return_thread']==native['thread'],'native callback thread mismatch'
    assert native['call_bytes'].endswith('ff5004')and native['conversion']==native['conversion_base']+20*8 and native['conversion_index']==20 and native['conversion_kind']==518 and native['conversion_target']==native['function_address'],'native callback identity/selection mismatch'
    return dict(entry=0x25450,return_site=0x25172,original_entry=a,original_return=b,native=native,original_context=old,scope=[0,1],suppression=[0,0],defined_heap_writes=0,unexercised='suppressed!=0 input write; no heap equivalence credit',later_heap_equality=False)

def progression(config,pe,events,native,trace):
    validate(config,pe)
    assert re.findall(r'^exception ([0-9a-f]+) va ([0-9a-f]+) ',events,re.M)==[('80000003','00028240')],'not genuine next masked strip_debug'
    words={int(a):int(b,16)for a,b in re.findall(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)};assert words[0]==0x1732b and words[1]==15,'actual next ABI input/return differs'
    memory={}
    for a,raw in re.findall(r'^memory ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):memory.update((int(a,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    read=lambda a:int.from_bytes(bytes(memory[a+i]for i in range(4)),'little')
    assert read(0x332a4)==15 and read(0x335ec)==read(0x335e8)==0,'original next consumed int/global scope differs'
    assert native['signal']==5 and native['frames']==['strip_debug','main']and native['parameters']=={'handle':15}and native['input_file']==15 and native['scope_depth']==native['suppressed_depth']==0 and native['later_heap_equality']is False,'native actual next ABI/state differs'
    call=bytes.fromhex(native['call_bytes']);assert len(call)==5 and call[0]==0xe8 and native['caller_pc']+int.from_bytes(call[1:],'little',signed=True)==native['function_address'],'native genuine caller CALL mismatch'
    assert native['thread'][0]>0 and native['caller_fp']>0,'native next frame/thread missing'
    apps={r['original']:r for r in config['functions']};entries=[apps[int(a,16)]['native']for a in re.findall(r'^runtime ([0-9a-f]+) [0-9a-f]+$',events,re.M)if int(a,16)in apps]+['strip_debug'];assert entries==trace and 'begin_symbol_block'in entries,'genuine complete entry projection differs'
    return dict(next='strip_debug',parameters={'handle':15},entry_trace=entries,original_return=words[0],native=native,later_heap_equality=False,scope='next scalar formal observed before original/native stub bodies; later file/page consumers require their actual typed state proof')
