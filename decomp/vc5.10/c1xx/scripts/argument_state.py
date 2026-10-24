"""Restore-only real iterator and borrowed Invoke argument matcher boundary."""
import copy,json,re,struct
from scaler_probe import symbols
from scaler_state import fp,semantic
from allocation_imports import pe_read


def check(s,d,original,native,trial,text,state,entry_state,vector_result):
    def need(ok,message):s.require(ok,'argument cursor: '+message)
    def one(name):
        rows=json.loads((native/name).read_text());need(len(rows)==1,'one actual '+name);return rows[0]
    def raw(phase,suffix):return(original/('locale-'+phase+'-'+suffix+'.bin')).read_bytes()
    names=symbols(native/'native-executable.elf');rules=json.loads((native/'options-grammar-layout.json').read_text())['option_rules']['address']
    continuation=trial['trial']=='_match_option'and bool(trial.get('matcher_observers'))
    need((continuation or trial['trial']=='_next_argument'and trial['test_seed']=='restore-nonzero')and trial['argument_observers']=={'entry_rva':0x39428,'end_rva':0x3944a,'return_rva':0x390b3,'next_rva':0x3944a,'scope':'restore-only actual Invoke26 to25, borrowed argv[1], real four-formal matcher; normal held at historical dependency entry'},'exact restore-only scope')
    own=[r for r in trial['ranges']if r['owner']=='_next_argument'];need(own==[{'owner':'_next_argument','start_rva':0x39428,'end_rva':0x3944a,'entry':1}],'whole34-byte leaf')
    entry=one('vector-next-entries.json');end=one('argument-returns.json');matcher=one('argument-matchers.json')
    a,b=[struct.unpack('<12I',(original/('argument-'+phase+'-registers.bin')).read_bytes())for phase in('entry','return')]
    need(a[1]==0x10439428 and a[8:]==(0x104390b3,a[2],0x10400000,0)and b[1]==a[8]and b[2]==a[2]+4 and b[4:8]==a[4:8]and b[9:]==a[9:]and a[0]<b[0],'original real entry/RET0/nonvolatiles')
    old_argv_raw=(original/'invoke-entry-argv.bin').read_bytes();old_at=8;old_items=[]
    need(struct.unpack_from('<I',old_argv_raw,4)[0]==26,'actual original Invoke26')
    for i in range(26):
        pointer,length=struct.unpack_from('<II',old_argv_raw,old_at);old_at+=8;value=old_argv_raw[old_at:old_at+length];old_at+=length;old_items.append((pointer,value))
    need(old_at==len(old_argv_raw)and old_items[1][1]==b'-il\0','actual original Invoke argument owner/full bytes')
    old_result=old_items[1][0];need(b[3]==old_result and(original/'argument-result.bin').read_bytes()==b'-il\0','returned original borrowed Invoke slot1 full bytes')
    initial=entry_state['frontend'];need(initial['command_argc']==26 and initial['command_argv']==entry_state['invocation']['argv_address'],'actual input Invoke26, distinct from HOST CRT arguments')
    argv=(native/'argument-entry-argv.bin').read_bytes();need(struct.unpack_from('<II',argv)==(initial['command_argv'],26),'complete bound Invoke vector dump')
    at=8;pointers=[]
    for i in range(26):
        pointer,length=struct.unpack_from('<II',argv,at);at+=8;value=argv[at:at+length];at+=length;pointers.append(pointer)
        need(value.endswith(b'\0')and b'\0'not in value[:-1],'complete ordered Invoke strings')
        if i==1:need(value==b'-il\0','actual slot1 bytes')
    need(at==len(argv),'exact bound Invoke vector bytes')
    need(entry['pc']==names['next_argument'][0]and entry['args']==[]and entry['formal_inventory']==[]and entry['caller']=='process_compiler_options','actual zero-formal source ABI')
    need(end['pc']==entry['return_pc']and end['entry_esp']==entry['esp']and end['esp']==entry['esp']+4 and all(end[k]==entry[k]for k in('ebx','esi','edi','ebp'))and end['eax']==pointers[1]and end['result']=='2d696c00','native real borrowed result/RET0/nonvolatiles')
    binary=(native/'native-executable.elf').read_bytes()
    def direct_call(pc,target):
        code=bytes(d.elf_byte(native,pc-5+i)for i in range(5));need(code[0]==0xe8 and pc+struct.unpack_from('<i',code,1)[0]==target,'actual direct caller encoding')
    direct_call(entry['return_pc'],entry['pc']);direct_call(matcher['return_pc'],matcher['pc'])
    need(pe_read(original/'loaded-target.bin',0x390b3,11)==bytes.fromhex('506860614d10e88c030000'),'actual original caller pushEAX/rules/matcherCALL/return390be')
    expected=copy.deepcopy(initial);expected['command_argc']=25;expected['command_argv']+=4
    need(state['frontend']==expected,'complete final modeled state differs only argc/borrowed argv advance')
    guard=copy.deepcopy(entry['guard']);bss=bytearray.fromhex(guard['bss']);storage=initial['storage'];base=initial['bss_address']
    for name,value in [('command_argc',25),('command_argv',initial['command_argv']+4)]:struct.pack_into('<I',bss,storage[name][0]-base,value)
    guard['bss']=bss.hex();need(end['guard']==matcher['guard']==guard,'full native target/saved/external guard differs only two fields')
    old=bytearray(raw('argument-entry','target-BSS'));argc,old_argv=struct.unpack_from('<II',old,0xc1708-0xb9000);need(argc==26 and old_argv==struct.unpack_from('<I',old_argv_raw)[0],'actual original argc26/borrowed Invoke root')
    struct.pack_into('<II',old,0xc1708-0xb9000,25,old_argv+4)
    need(raw('argument-return','target-BSS')==raw('argument-next','target-BSS')==bytes(old),'whole original BSS/no unknown-neighbor mutation')
    for phase in('argument-return','argument-next'):
        for kind in('data','snapshot'):need((original/('frontend-'+kind+'-locale-'+phase+'.bin')).read_bytes()==(original/('frontend-'+kind+'-locale-argument-entry.bin')).read_bytes(),'original whole initialized/snapshot preservation')
        for suffix in('words','mbctype','ctype','old-ctype','result','fp','errors','CRT-argv','CRT-environment','Invoke-argv'):need(raw(phase,suffix)==raw('argument-entry',suffix),'original provider/vector/error/fullFP lifetime '+suffix)
    first_locale={k:v for k,v in entry['locale'].items()if k!='maps'};first_fp=semantic(entry['fp'])
    for row,phase in((end,'argument-return'),(matcher,'argument-next')):
        need({k:v for k,v in row['locale'].items()if k!='maps'}==first_locale and row['last_error']==203 and semantic(row['fp'])==first_fp,'native locale/owners/namespace/errors/architecturalFP')
        for kind,baseline in(('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):need((native/('locale-native-'+phase+'-'+kind+'.bin')).read_bytes()==(native/baseline).read_bytes(),'private ordered native vectors preserved')
    need(state['invocation']==entry_state['invocation']and {'cw':state['x87']['control'],'sw':state['x87']['status'],'tag':state['x87']['tag'],'top':(state['x87']['status']>>11)&7,'live':[]if state['x87']['tag']==65535 else None}==first_fp,'actual Invoke strings/root lifetime and frontier architecturalFP')
    need(matcher['pc']==names['match_option'][0]and matcher['args']==[rules,pointers[1],names['next_argument'][0],0]and matcher['formal_inventory']==[{'name':k,'type':v}for k,v in [('rules','CompilerOptionRule *'),('argument','char *'),('next','char *(*)(void)'),('copy','int')]]and matcher['caller']=='process_compiler_options','actual four-formal matcher raw ABI/grammar/borrowed argument/callback/scalar')
    need(state['symbol']=='match_option'and state['args']==dict(zip(('rules','argument','next','copy'),matcher['args'])),'actual matcher source formals')
    if continuation:need(state['pc']==matcher['pc']and state['esp']==matcher['esp'],'direct physical matcher pre-body checkpoint')
    else:need(names['match_option'][0]<state['pc']<=sum(names['match_option'])and d.elf_byte(native,state['pc']-1)==0xcc and b'Program received signal SIGTRAP'in(native/'gdb.txt').read_bytes(),'actual named source INT3 matcher frontier')
    traps=re.findall(r'^trap seq=(\d+) rva=(\w+) .*stack=([0-9a-f,]+)$',text,re.M);tail=[r for r in traps if int(r[0])>=a[0]][:2]if continuation else[r for r in traps if int(r[0])>=a[0]]
    need(len(tail)==2 and [r[1]for r in tail]==['00039428','0003944a']and [int(v,16)for v in tail[-1][2].split(',')]==[0x104390be,0x104d6160,old_result,0x10439428,0]and re.findall(r'^frontier rva=(\w+)$',text,re.M)==(['00039632']if continuation else['0003944a']),'complete original actual matcher4 stack slots and final named trap')
    final_sequence=int(tail[-1][0])
    need((original/('pool-bss-%03d.bin'%(final_sequence+1))).read_bytes()==bytes(old),'direct actual final trap fullBSS equals pre-prologue matcher observation')
    for kind in('data','snapshot'):need((original/('frontend-'+kind+'-%03d.bin'%final_sequence)).read_bytes()==(original/('frontend-'+kind+'-locale-argument-entry.bin')).read_bytes(),'direct actual final trap initialized/snapshot bytes')
    need(semantic(fp((original/('x87-context-%03d.bin'%final_sequence)).read_bytes()))==first_fp,'original direct final architecturalFP')
    return dict(frontier='_match_option',code_bytes=34,executed_case='actual normal/restore accepted-prefix iterator checkpoint'if continuation else'restore-nonzero only',normal_case='ordinary matcher continuation; no additional iterator acceptance'if continuation else'held at historical genuine SigForPbCb entry; no fresh normal leaf claim',argc_before=26,argc_after=25,argv_advance_bytes=4,result='borrowed Invoke argv[1] -il NUL',matcher_formals=4,vector_prefix=vector_result)
