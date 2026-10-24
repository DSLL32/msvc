"""Actual ordinary CPP public-call result; no captured-response replay."""
import json,re,struct
from allocation_imports import pe_read,export_rva
from scaler_probe import symbols
from scaler_state import fp,semantic


def check(s,d,original,native,trial,text):
    def need(ok,message):s.require(ok,'CPP signature call: '+message)
    def raw(phase,suffix):return(original/('locale-signature-'+phase+'-'+suffix+'.bin')).read_bytes()
    def one(name):
        rows=json.loads((native/name).read_text());need(len(rows)==1,'one actual '+name);return rows[0]
    provider=original/'loaded-mspdb50.bin';need(s.sha(provider)=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'and export_rva(provider,'SigForPbCb')==0xf22d,'actual canonical provider hFile/export')
    a,b=[struct.unpack('<16I',(original/('signature-call-'+phase+'-registers.bin')).read_bytes())for phase in('entry','return')]
    dep=struct.unpack('<12I',(original/'signature-entry-registers.bin').read_bytes());base=a[11]
    need(a[1]==base+0xf22d and a[10]==b[1]==b[10]==0x1043909f and b[2]==a[2]+4 and b[11]==base and a[12:]==(0x104d6620,3,0xffffffff,a[15]),'original true public CPP/3/seed/RET0 ABI')
    need(a[0]==dep[0]+1 and a[1:3]==dep[1:3]and tuple(dep[3:7])==tuple(a[6:10])and dep[7:11]==(a[10],*a[12:15]),'same physical API entry; save_state advances sequence without target execution')
    need(a[6:10]==b[6:10]and b[12:14]==a[12:14]and b[14]==b[3],'original nonvolatile preservation and actual third private parameter-slot update')
    for phase in('entry','return'):need((original/('signature-call-'+phase+'-input.bin')).read_bytes()==b'CPP\0','complete original literal lifetime')
    need((original/'signature-provider-body.bin').read_bytes()==(original/'signature-provider-entry.bin').read_bytes()==pe_read(provider,0xf22d,53),'whole canonical53B body captured before entry breakpoint')
    table=(original/'signature-provider-table.bin').read_bytes();need(len(table)==1024 and table==(original/'signature-provider-table-after.bin').read_bytes()==pe_read(provider,0x21a30,1024),'complete canonical table before/after unchanged')
    body=pe_read(provider,0xf22d,53);need(body[0]==0x55 and body[-1:]==b'\xc3'and s.sha(original/'signature-provider-body.bin')=='0c863c2888221375919652e30969b70894039b67288033e727d321c29ef8f77c','whole provider entry/RET0 extent')
    code=pe_read(original/'loaded-target.bin',0x3909f,8);need(code==bytes.fromhex('83c40ca334084c10'),'actual caller immediately discards12 private bytes, consumes only EAX')
    entry=one('vector-public-entries.json');end=one('signature-returns.json');names=symbols(native/'native-executable.elf')
    need(entry['pc']==names['recon_mspdb_SigForPbCb'][0]and entry['args'][1:]==[3,0xffffffff]and entry['input']=='43505000'and entry['caller']=='process_compiler_options'and entry['formal_inventory']==[{'name':k,'type':v}for k,v in [('bytes','const void *'),('byte_count','long'),('signature','unsigned long')]],'actual native adapter physical entry/formals/read-only literal')
    call=bytes(d.elf_byte(native,entry['return_pc']-5+i)for i in range(5));need(call[0]==0xe8 and entry['return_pc']+struct.unpack_from('<i',call,1)[0]==entry['pc'],'actual native direct public call')
    need(end['pc']==entry['return_pc']and end['esp']==entry['esp']+4 and end['entry_esp']==entry['esp']and end['eax']==b[3]and all(end[k]==entry[k]for k in('ebx','esi','edi','ebp')),'actual native RET0/EAX and nonvolatiles match original')
    need(end['argument_slots_after']==[entry['args'][0],3,end['eax']],'actual native third private incoming slot update agrees, other two preserved')
    need(end['guard']==entry['guard'],'public implementation modifies no target/initialized/saved/external storage')
    need(semantic(end['fp'])==semantic(entry['fp'])==semantic(fp(raw('entry','fp')))and raw('entry','fp')==raw('return','fp'),'full original112FP preserved/native architecturalFP')
    need(entry['last_error']==end['last_error']==203 and {k:v for k,v in entry['locale'].items()if k!='maps'}=={k:v for k,v in end['locale'].items()if k!='maps'},'all native provider tables/owners/selection/errors/private vectors unchanged')
    for suffix in('words','mbctype','ctype','old-ctype','result','errors','CRT-argv','CRT-environment','Invoke-argv','target-BSS'):need(raw('entry',suffix)==raw('return',suffix),'whole original state/lifetime '+suffix)
    for kind in('data','snapshot'):need((original/('frontend-'+kind+'-locale-signature-entry.bin')).read_bytes()==(original/('frontend-'+kind+'-locale-signature-return.bin')).read_bytes(),'whole original initialized/saved bytes unchanged')
    need((original/'signature-call-entry-vector.bin').read_bytes()==(original/'signature-call-return-vector.bin').read_bytes(),'entire owned32KiB arena bytes unchanged')
    for phase in('entry','return'):
        for kind,baseline in(('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):need((native/('locale-native-signature-'+phase+'-'+kind+'.bin')).read_bytes()==(native/baseline).read_bytes(),'native full private ordered vector bytes unchanged')
    stage=json.loads((native/'argument-entry-state.json').read_text());need(stage['frontend']['source_signature']==end['eax'],'actual accepted caller stores returned unsigned32 before iterator')
    first=one('vector-next-entries.json');expected=dict(end['guard']);expected['bss']=bytearray.fromhex(expected['bss']);storage=stage['frontend']['storage']['source_signature'][0]-stage['frontend']['bss_address'];struct.pack_into('<I',expected['bss'],storage,end['eax']);expected['bss']=expected['bss'].hex();need(first['guard']==expected,'complete native API-return to iterator target state changes only actual signature store')
    old=bytearray(raw('return','target-BSS'));struct.pack_into('<I',old,0xc0834-0xb9000,b[3]);need((original/'locale-argument-entry-target-BSS.bin').read_bytes()==bytes(old),'complete original API-return to iterator target state changes only signature store')
    for suffix in('words','mbctype','ctype','old-ctype','result','fp','errors','CRT-argv','CRT-environment','Invoke-argv'):need((original/('locale-argument-entry-'+suffix+'.bin')).read_bytes()==raw('return',suffix),'original direct API-return to iterator owner/vector/FP bridge '+suffix)
    for kind in('data','snapshot'):need((original/('frontend-'+kind+'-locale-argument-entry.bin')).read_bytes()==(original/('frontend-'+kind+'-locale-signature-return.bin')).read_bytes(),'original API-return to iterator initialized/saved bridge')
    need(struct.unpack_from('<I',raw('entry','target-BSS'),0xc0834-0xb9000)[0]==0xffffffff and struct.unpack_from('<I',(original/'locale-argument-entry-target-BSS.bin').read_bytes(),0xc0834-0xb9000)[0]==b[3],'original seed and actual caller global result store')
    return dict(input_hex='435050',defined_literal_hex='43505000',count=3,seed=0xffffffff,result=b[3],result_hex='%08x'%b[3],provider_code_bytes=53,provider_table_bytes=1024,private_stack='all3 slots observed pre/post, third equals result on both; caller ADD12 before any slot read/escape, only EAX consumed',provider_application_credit=0)
