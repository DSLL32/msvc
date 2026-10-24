"""Complete matcher candidate through its genuine action-call frontier."""
import copy,json,re,struct
from allocation_imports import pe_read
from scaler_probe import symbols
from scaler_state import fp,semantic


def check(s,d,original,native,trial,text,state,prefix):
    def need(ok,message):s.require(ok,'option matcher: '+message)
    def one(name):
        rows=json.loads((native/name).read_text());need(len(rows)==1,'one actual '+name);return rows[0]
    def raw(phase,suffix):return(original/('locale-'+phase+'-'+suffix+'.bin')).read_bytes()
    def regs(phase):return struct.unpack('<16I',(original/('matcher-'+phase+'-registers.bin')).read_bytes())
    seed=bool(trial.get('test_seed'));names=symbols(native/'native-executable.elf');entry=one('argument-matchers.json');ne=one('matcher-next-entries.json');nr=one('matcher-next-returns.json');action=one('matcher-actions.json');stage=json.loads((native/'matcher-entry-state.json').read_text());initial=stage['frontend'];before=regs('entry');nextin=regs('next-entry');nextout=regs('next-return');final=regs('action')
    need(trial['trial']=='_match_option'and trial['matcher_observers']=={'entry_rva':0x3944a,'action_rva':0x39632,'next_callback_return_rva':0x3957b,'signature_return_rva':0x3909f,'original_table_rvas':[[0x395e8,0x39632]],'scope':'ordinary complete matcher candidate; own CPP Sig53B/table1024 pre/post then real apply action frontier, no standalone API-only role'},'exact ordinary candidate scope')
    own=[r for r in trial['ranges']if r['owner']=='_match_option'];need(own==[{'owner':'_match_option','start_rva':0x3944a,'end_rva':0x395e2,'entry':1},{'owner':'_match_option','start_rva':0x72573,'end_rva':0x72589,'entry':0},{'owner':'_match_option','start_rva':0x395e8,'end_rva':0x39632,'entry':0,'kind':'text_data'}],'complete430B+74data, padding excluded')
    model=json.loads((native/'state-model.json').read_text());need(model['schema']==37 and initial['known_sizes']['bss']==2828 and initial['storage']['current_argument']==[initial['bss_address']+2824,4],'new borrowed pointer exact typed layout')
    def current(suffix):
        b=(original/('current-argument-'+suffix+'.bin')).read_bytes();need(len(b)==4,'current pointer observation width');return struct.unpack('<I',b)[0]
    pre=json.loads((native/'pre.json').read_text());phase=json.loads((native/'phase.json').read_text())
    need(current('000')==0,'original zeroBSS default')
    if seed:
        old_saved=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0]
        need(current('006')==old_saved+4096 and pre['frontend']['current_argument']==pre['frontend']['saved_state']+4096 and text.count('current-argument-seed owned-snapshot+4096 never-dereferenced=1 no-code-mock=1\n')==1,'explicit nonzero owned-pointer reset input')
    else:need(current('006')==pre['frontend']['current_argument']==0 and 'current-argument-seed 'not in text,'normal default input not seeded')
    need(current('phase')==phase['frontend']['current_argument']==initial['current_argument']==0,'new pointer participates in genuine BSS reset before semantic consumer')
    def argv(path):
        b=path.read_bytes();root,count=struct.unpack_from('<II',b);at=8;rows=[]
        for i in range(count):
            pointer,n=struct.unpack_from('<II',b,at);at+=8;value=b[at:at+n];at+=n;rows.append((pointer,value))
        need(at==len(b)and count==26,'whole actual Invoke26 vector');return root,rows
    oldroot,oldargs=argv(original/'invoke-entry-argv.bin');root,args=argv(native/'matcher-entry-argv.bin')
    need(oldargs[1][1]==args[1][1]==b'-il\0'and oldargs[2][1]==args[2][1]==(original/'matcher-value.bin').read_bytes(),'full owned callback value and argument bytes')
    need(initial['command_argc']==25 and initial['command_argv']==root+4 and entry['args'][1]==args[1][0]and before[10:15]==(0x104390be,0x104d6160,oldargs[1][0],0x10439428,0),'actual matcher four-formal entry/borrowed owners')
    need(before[1]==0x1043944a and nextin[1]==0x10439428 and nextin[10]==0x1043957b and nextout[1]==0x1043957b and nextout[2]==nextin[2]+4 and nextout[6:10]==nextin[6:10]and nextout[3]==oldargs[2][0],'actual original zero-formal callback/RET0/nonvolatile/result')
    need(ne['pc']==names['next_argument'][0]and ne['args']==[]and ne['formal_inventory']==[]and ne['caller']=='match_option'and nr['pc']==ne['return_pc']and nr['entry_esp']==ne['esp']and nr['esp']==ne['esp']+4 and nr['eax']==args[2][0]and nr['value']==args[2][1].hex()and all(nr[k]==ne[k]for k in('ebx','esi','edi','ebp')),'actual native indirect callback/RET0/nonvolatile/result')
    rules=json.loads((native/'options-grammar-layout.json').read_text())['option_rules']['address'];patterns=json.loads((native/'options-native-patterns.json').read_text());need(patterns[31]=='-il$','actual grammar rule31')
    need(final[1]==0x10439632 and final[10:14]==(0x10439589,0x104d62d4,oldargs[2][0],0)and action['pc']==names['apply_option_action'][0]and action['args']==[rules+31*12,args[2][0],0]and action['value']==args[2][1].hex()and action['caller']=='match_option'and action['formal_inventory']==[{'name':k,'type':v}for k,v in [('rule','CompilerOptionRule *'),('value','char *'),('copy','int')]],'complete genuine action boundary: actual rule/value/copy physical inputs')
    need(state['symbol']=='apply_option_action'and state['args']==dict(zip(('rule','value','copy'),action['args']))and names['apply_option_action'][0]<state['pc']<=sum(names['apply_option_action'])and d.elf_byte(native,state['pc']-1)==0xcc and b'Program received signal SIGTRAP'in(native/'gdb.txt').read_bytes(),'actual named action INT3 frontier')
    expected=copy.deepcopy(initial);expected['current_argument']=args[1][0];expected['command_argc']=24;expected['command_argv']=root+8;need(state['frontend']==expected and state['invocation']==stage['invocation'],'entire typed final state differs only encountered borrowed pointer/cursor')
    guard=copy.deepcopy(entry['guard']);buf=bytearray.fromhex(guard['bss']);storage=initial['storage'];base=initial['bss_address'];struct.pack_into('<I',buf,storage['current_argument'][0]-base,args[1][0]);guard['bss']=buf.hex();need(ne['guard']==guard,'whole candidate input guard changes only borrowed current argument before callback')
    for field,value in(('command_argc',24),('command_argv',root+8)):struct.pack_into('<I',buf,storage[field][0]-base,value)
    guard['bss']=buf.hex();need(nr['guard']==action['guard']==guard,'entire target/saved/external native guard at callback return/action')
    old=bytearray(raw('matcher-entry','target-BSS'));need(struct.unpack_from('<II',old,0xc1708-0xb9000)==(25,oldroot+4)and struct.unpack_from('<I',old,0xbe96c-0xb9000)[0]==0,'actual original cursor and current pointer input')
    struct.pack_into('<I',old,0xbe96c-0xb9000,oldargs[1][0]);need(raw('matcher-next-entry','target-BSS')==bytes(old),'whole original matcher mutation before callback')
    struct.pack_into('<II',old,0xc1708-0xb9000,24,oldroot+8);need(raw('matcher-next-return','target-BSS')==raw('matcher-action','target-BSS')==bytes(old),'whole original callback return/action storage and unmodeled neighbors')
    baseline_locale={k:v for k,v in entry['locale'].items()if k!='maps'};baseline_fp=semantic(entry['fp'])
    for phase_name,row in(('matcher-next-entry',ne),('matcher-next-return',nr),('matcher-action',action)):
        need(row['last_error']==203 and semantic(row['fp'])==baseline_fp and {k:v for k,v in row['locale'].items()if k!='maps'}==baseline_locale,'complete native provider owners/tables/errors/FP continuity')
        for suffix in('words','mbctype','ctype','old-ctype','result','errors','fp','CRT-argv','CRT-environment','Invoke-argv'):need(raw(phase_name,suffix)==raw('matcher-entry',suffix),'complete original provider/namespace/FP lifetime '+suffix)
        for kind in('data','snapshot'):need((original/('frontend-'+kind+'-locale-'+phase_name+'.bin')).read_bytes()==(original/('frontend-'+kind+'-locale-matcher-entry.bin')).read_bytes(),'whole original initialized/snapshot bytes unchanged')
        for kind,baseline in(('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):need((native/('locale-native-'+phase_name+'-'+kind+'.bin')).read_bytes()==(native/baseline).read_bytes(),'native actual ordered private vectors at each phase')
    seq=final[0];need((original/('pool-bss-%03d.bin'%(seq+1))).read_bytes()==bytes(old)and re.findall(r'^frontier rva=(\w+)$',text,re.M)==['00039632'],'direct actual final trap BSS and unambiguous action frontier')
    need(semantic(fp((original/('x87-context-%03d.bin'%seq)).read_bytes()))==baseline_fp and {'cw':state['x87']['control'],'sw':state['x87']['status'],'tag':state['x87']['tag'],'top':(state['x87']['status']>>11)&7,'live':[]if state['x87']['tag']==65535 else None}==baseline_fp,'direct final frontier architecturalFP')
    for kind in('data','snapshot'):need((original/('frontend-'+kind+'-%03d.bin'%seq)).read_bytes()==(original/('frontend-'+kind+'-locale-matcher-entry.bin')).read_bytes(),'direct final initialized/saved trap bytes')
    table=(original/'relocated-text.bin').read_bytes()[0x395e8-0x1000:0x39632-0x1000];need(table==pe_read(original/'loaded-target.bin',0x395e8,74),'exact74 relocated jump+selector data')
    data=[r for r in trial['ranges']if r.get('kind')=='text_data'and r['owner']=='_match_option'];need(len(data)==1 and data[0]['entry']==0,'table ownership never an entry/root/code credit')
    result=None
    if not seed:
        from signature_call_state import check as check_signature
        result=check_signature(s,d,original,native,trial,text)
        after=one('signature-returns.json');first=one('vector-next-entries.json');need({k:v for k,v in after['locale'].items()if k!='maps'}=={k:v for k,v in first['locale'].items()if k!='maps'},'public-return to iterator actual provider owner/vector continuity')
    else:need(json.loads((native/'vector-public-entries.json').read_text())==json.loads((native/'signature-returns.json').read_text())==[],'restore does not execute skipped provider')
    return dict(frontier='_apply_option_action',code_bytes=430,table_data_bytes=74,table_code_credit=0,new_BSS_semantic_bytes=4,argument='borrowed Invoke[1] -il',value='borrowed Invoke[2]',argc_before=25,argc_after=24,rule_index=31,rule_action=0x22,signature=result,prefix=prefix,unmodeled_initialized_bytes=6031)
