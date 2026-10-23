"""Captured x87 values, QWORD compiler conversion, and owned reserve-only output."""
import copy,json,re,struct
from fractions import Fraction
from pathlib import Path
from scaler_probe import describe,symbols as elf_symbols
from allocation_imports import pe_read,export_rva


def fp(raw):
    assert len(raw)==112
    cw,sw,tag=(v&0xffff for v in struct.unpack_from('<3I',raw));top=(sw>>11)&7
    registers=[raw[28+i*10:38+i*10].hex()for i in range(8)]
    return dict(cw=cw,sw=sw,tag=tag,top=top,registers=registers,live=[dict(logical=i,tag=(tag>>(2*((top+i)&7)))&3,raw80=registers[i])for i in range(8)if ((tag>>(2*((top+i)&7)))&3)!=3])


def semantic(row):return {key:row[key]for key in ('cw','sw','tag','top','live')}


def value80(value):
    raw=bytes.fromhex(value);assert len(raw)==10
    significand,sign=struct.unpack('<QH',raw);exponent=sign&0x7fff
    if not significand:return Fraction(0)
    assert exponent not in (0,0x7fff) and significand>>63==1
    power=exponent-16383-63;result=Fraction(significand<<power)if power>=0 else Fraction(significand,1<<-power)
    return -result if sign&0x8000 else result


def check(s,d,original,native,trial,text,state,final,symbol_table,traps):
    def require(value,message):s.require(value,'reservation scaler: '+message)
    def read(name):return json.loads((native/(name+'.json')).read_text())
    def one(name):
        rows=read(name);require(len(rows)==1,'wrong '+name+' inventory');return rows[0]
    def call(ret,destination):
        code=bytes(d.elf_byte(native,ret-5+i)for i in range(5));require(code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==destination,'not actual native CALL')
    names=elf_symbols(native/'native-executable.elf');metadata=read('scaler-probe-metadata');require(metadata==describe(native/'native-executable.elf'),'stale/guessed conversion instruction metadata')
    require(trial.get('support_imports',[])==['import:_ftol'] and trial['trial']=='_calculate_arena_reservation','compiler support not paired with sole calculator candidate')
    imports=[row for row in trial['imports']if row['name']=='import:_ftol'];require(len(imports)==1 and (imports[0]['start'],imports[0]['end'],imports[0]['provider'])==(0x10438f04,0x10438f0a,{'dll':'MSVCRT.dll','name':'_ftol'}),'wrong compiler-support import ownership')
    expected_points=[0x38ed7,0x38edd,0x38ee3,0x38ee9,0x38eee,0x38e81,0x38ea9,0x38eaf]
    observer=trial.get('scaler_observers',{});require(observer.get('original_points')==expected_points and observer.get('ftol_thunk_rva')==0x38f04 and observer.get('ftol_iat_rva')==0xc90c8 and observer.get('provider_bytes')==39 and observer.get('x87_entry_profile')==[0x027f,0x0020,0xffff],'undeclared math/provider observation scope')
    rows=re.findall(r'^scaler point=(\d+) seq=(\d+) rva=(\w+) eax=(\w+) edx=(\w+) ecx=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) esp=(\w+) error=(\d+) cw=(\w+) sw=(\w+) tag=(\w+) stack=([0-9a-f,]+)$',text,re.M)
    require(len(rows)==8 and [(int(r[0]),int(r[1]),int(r[2],16))for r in rows]==[(i,i,rva)for i,rva in enumerate(expected_points)],'actual original arithmetic/return/API point order differs')
    old=[]
    for i,row in enumerate(rows):
        item=dict(zip(('eax','edx','ecx','edi','esi','ebx','esp'),[int(v,16)for v in row[3:10]]));item.update(fp=fp((original/('scaler-fp-%03d.bin'%i)).read_bytes()),error=int(row[10]),stack=[int(v,16)for v in row[14].split(',')])
        require([int(v,16)&0xffff for v in row[11:14]]==[item['fp'][k]for k in ('cw','sw','tag')] and item['error']==203,'original raw arithmetic FP/error words differ');old.append(item)
    # This is the actual loaded hFile and resolved IAT, not a shipped-file guess.
    crt=original/'loaded-msvcrt.bin';require(s.sha(crt)=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210' and export_rva(crt,'_ftol')==0x4dc1,'wrong compiler provider hFile/export')
    module=int(re.search(r'^module name=MSVCRT.dll actual=(\w+) ',text,re.M|re.I)[1],16)
    require(re.findall(r'^ftol-provider iat=000c90c8 resolved=(\w+)$',text,re.M)==['%08x'%(module+0x4dc1)] and (original/'ftol-provider-code.bin').read_bytes()==pe_read(crt,0x4dc1,39),'actual _ftol provider/code differs')
    require(pe_read(original/'loaded-target.bin',0x38f04,6)==bytes.fromhex('ff25c8904c10') and pe_read(original/'loaded-target.bin',0xc93b0,8)==struct.pack('<d',-0.01),'original import thunk/point-of-use double constant differs')
    # The only supplied architecture is the independently reviewed empty entry.
    reference=s.sha(original/'receipt.json');binding=read('x87-entry-binding');initial=one('scaler-initial');profile=one('scaler-profile')
    require(binding['original_receipt_sha256']==reference and binding['original_directory']==read('system-info-binding')['original_directory'] and binding['original_float_save_sha256']==s.sha(original/'x87-context-000.bin') and binding['trial_sha256']==s.sha(original/'trial.json'),'entry profile closure differs')
    require((binding['guard_rva'],binding['cw'],binding['sw'],binding['tag'],binding['top'],binding['live_registers'])==(0x2e3c6,0x027f,0x0020,0xffff,0,[]) and s.sha(native/binding['factory_artifact'])==binding['factory_sha256'],'entry profile values/factory content differ')
    require(profile['pc']==names['recon_apply_x87_empty_entry_profile'][0] and profile['args']==[reference,0x027f,0x0020,0xffff] and profile['applications']==0 and semantic(profile['fp'])==dict(cw=0x027f,sw=0,tag=0xffff,top=0,live=[]) and profile['last_error']==203,'not actual one-time empty native prestate before target entry')
    require(initial['pc']==names['_DllMainCRTStartup'][0] and initial['args']==[state['registered_token'],1,0] and initial['return_pc'] and initial['esp'] and initial['profile']=={'reference':reference,'applications':1} and initial['last_error']==203 and semantic(initial['fp'])==semantic(fp((original/'x87-context-000.bin').read_bytes())),'actual first DLL entry/profile reference/count/ABI/FP words differ')
    entry=one('scaler-entries');returned=one('scaler-returns');records=read('scaler-fpu');require([row['label']for row in records]==['instruction-%d'%i for i in range(9)]+['converted'],'wrong native conversion observation order')
    require([row['pc']for row in records[:9]]==[row['pc']for row in metadata['instructions']] and records[-1]['pc']==metadata['converted_pc'],'actual native instruction points differ')
    require(entry['pc']==metadata['start'] and entry['descriptor']==state['frontend']['storage']['arena_descriptors'][0] and entry['last_error']==returned['last_error']==203 and semantic(entry['fp'])==semantic(old[0]['fp']),'calculator actual descriptor/input FP state differs')
    for index,old_index in [(0,0),(1,1),(3,2),(5,3),(9,4)]:require(semantic(records[index]['fp'])==semantic(old[old_index]['fp']),'actual FP live80/CW/SW/tag/depth mismatch at conversion checkpoint')
    source=(original/'scaler-data-000.bin').read_bytes();percent=struct.unpack_from('<i',source,0x1c28)[0]
    descriptor=list(struct.unpack('<5I',(original/'arena-descriptors-000.bin').read_bytes()[:20]));reserve=struct.unpack('<i',struct.pack('<I',descriptor[1]))[0];unit=descriptor[2]
    require(descriptor==[0,0x3200000,0x10000,0x21,1] and value80(old[1]['fp']['registers'][0])==reserve and value80(old[2]['fp']['registers'][0])==reserve*percent,'signed reserve/percent operand order differs')
    require([len(row['fp']['live'])for row in records[:6]]==[0,1,2,1,2,1] and value80(records[2]['fp']['registers'][0])==percent and records[2]['fp']['registers'][1]==old[1]['fp']['registers'][0] and value80(records[4]['fp']['registers'][0])==Fraction.from_float(-0.01) and records[4]['fp']['registers'][1]==old[2]['fp']['registers'][0],'extra native FILD/FLD temporary operands/stack differ')
    for index in (2,4):
        now,before=records[index]['fp'],records[index-1]['fp'];require(now['cw']==before['cw']==0x027f and now['sw']&~0x3800==before['sw']&~0x3800 and now['top']==6 and now['tag']==0x0fff,'extra native temporary status/tag/CW differs')
    require(records[6]['fp']['cw']==0x027f and records[7]['fp']['cw']==records[8]['fp']['cw']==0x0e7f,'temporary native truncation control differs')
    converted=records[-1];require(converted['qword_address']==converted['ebp']+metadata['qword_ebp_displacement'],'not actual FISTP QWORD output location')
    qword=bytes.fromhex(converted['qword_bytes']);require(len(qword)==8 and struct.unpack('<Q',qword)[0]==(old[4]['edx']<<32)|old[4]['eax'] and struct.unpack('<q',qword)[0]==int(value80(old[3]['fp']['registers'][0])),'actual signed64 conversion/high-low words differ')
    rounded=(unit-(struct.unpack('<Q',qword)[0]&0xffffffff)-1)&(~(unit-1)&0xffffffff);rounded=rounded or unit
    require(returned['eax']==old[5]['eax']==rounded and returned['esp']==entry['esp']+4 and returned['pc']==entry['return_pc'] and all(returned[r]==entry[r]for r in ('edi','esi','ebx')) and semantic(returned['fp'])==semantic(old[5]['fp'])==semantic(converted['fp']),'whole calculator rounded result/RET0/nonvolatile/FP return differs');call(returned['pc'],metadata['start'])
    require(all(row['guard']==entry['guard'] and row['last_error']==203 for row in records+[returned]),'native calculator modified modeled/saved/external state')
    for i in range(1,8):
        for name in ('data','saved','support'):require((original/('scaler-%s-%03d.bin'%(name,i))).read_bytes()==(original/('scaler-%s-000.bin'%name)).read_bytes(),'calculator/API changed unrelated original '+name)
    baseline=(original/'scaler-bss-000.bin').read_bytes();require(len(baseline)==0xf69c and baseline==(original/'arena-bss-size-frontier.bin').read_bytes(),'missing whole original calculator input BSS')
    for i in range(1,6):require((original/('scaler-bss-%03d.bin'%i)).read_bytes()==baseline,'calculator changed original BSS')
    before=one('arena-api-entries');after=one('arena-api-returns');api=names['recon_VirtualAlloc'][0];member_start,member_size=names['_ZN13CompilerArena7ReserveEPK15ArenaDescriptorPh']
    require(before['pc']==api and before['args']==[0,rounded,0x102000,4] and member_start<before['return_pc']<member_start+member_size and after['pc']==before['return_pc'] and after['esp']==before['esp']+20 and all(after[r]==before[r]for r in ('edi','esi','ebx')),'actual native reserve API args/caller/RET16/nonvolatile differ');call(before['return_pc'],api)
    b=re.findall(r'^arena-api-before address=(\w+) bytes=(\w+) kind=(\w+) protection=(\w+) provider=(\w+) esp=(\w+) error=(\d+)$',text,re.M);a=re.findall(r'^arena-api-after result=(\w+) esp=(\w+) error=(\d+) region-base=(\w+) bytes=(\w+) state=(\w+) protect=(\w+) allocation-protect=(\w+) type=(\w+)$',text,re.M)
    require(len(b)==len(a)==1,'wrong original reservation inventory');b,a=b[0],a[0];kernel=int(re.search(r'^module name=KERNEL32.dll actual=(\w+) ',text,re.M)[1],16)
    require([int(v,16)for v in b[:4]]==[0,rounded,0x102000,4] and int(b[4],16)==kernel+export_rva(original/'loaded-kernel32.bin','VirtualAlloc') and int(a[1],16)==int(b[5],16)+16 and int(b[6])==int(a[2])==before['last_error']==after['last_error']==203,'actual original reservation provider/args/RET16/error differs')
    old_base=int(a[0],16);new_base=after['eax'];require(old_base and new_base and not(old_base%65536 or new_base%65536),'reservation alignment/result differs')
    region=struct.unpack('<7I',(original/'arena-region.bin').read_bytes());require(region==(old_base,old_base,4,rounded,0x2000,0,0x20000) and [int(v,16)for v in a[3:]]==[old_base,rounded,0x2000,0,4,0x20000],'actual original whole reserve-only region differs')
    require(after['owned_reservations']==[{'address':new_base,'bytes':rounded}],'native output is not exactly owned reserved region')
    maps=[]
    for line in after['maps'].splitlines():
        columns=line.split();lo,hi=[int(v,16)for v in columns[0].split('-')]
        if lo<=new_base<hi:maps.append((lo,hi,columns[1]))
    require(len(maps)==1 and maps[0][1]>=new_base+rounded and maps[0][2]=='---p','native region not fully PROT_NONE reservation')
    require(before['guard']==after['guard'] and semantic(before['fp'])==semantic(after['fp'])==semantic(returned['fp']),'reservation API changed target state or FP context')
    before_raw=bytearray(baseline);struct.pack_into('<I',before_raw,0xc6500-0xb9000+20,rounded)
    require(bytes(before_raw)==(original/'scaler-bss-006.bin').read_bytes()==(original/'scaler-bss-007.bin').read_bytes(),'reservation/size assignment changed unrelated original BSS')
    reset=one('arena-reset-entries');reset_pc,reset_size=names['_ZN13CompilerArena5ResetEv'];arena=state['frontend']['storage']['arenas'][0]
    require(reset['pc']==reset_pc and reset['ecx']==arena and reset['formal_inventory']==[{'name':'this','type':'CompilerArena *'}] and member_start<reset['return_pc']<member_start+member_size and state['symbol']=='CompilerArena::Reset' and state['args']=={'this':arena},'genuine pending Reset ECX/no-stackargs/caller differs');call(reset['return_pc'],reset_pc)
    old_reset=re.findall(r'^trap seq=\d+ rva=00038f0a .*ecx=(\w+) edi=\w+ stack=([0-9a-f,]+)$',text,re.M);require(len(old_reset)==1 and int(old_reset[0][0],16)==0x104c6500 and int(old_reset[0][1].split(',')[0],16)==0x10438ec8,'original pending Reset this/caller differs')
    expected=copy.deepcopy(read('arena-size-entry')['frontend']);expected['arenas'][2]=new_base;expected['arenas'][5]=rounded
    require(state['frontend']==expected and reset['fp']['live']==[] and semantic(reset['fp'])==semantic(returned['fp']),'final typed arena/neighbor/FP state differs')
    final_bss=bytearray(before_raw);struct.pack_into('<I',final_bss,0xc6500-0xb9000+8,old_base);require(bytes(final_bss)==(original/'arena-bss-reset-frontier.bin').read_bytes(),'reservation continuation changed original unmodeled BSS')
    require(list(struct.unpack('<72I',(original/('arenas-'+final+'.bin')).read_bytes()))==[old_base if i==2 else 0x104d5af8 if i==4 else rounded if i==5 else 0 for i in range(72)],'actual original final arena record differs')
    for name in ('frontend-data','frontend-snapshot','state'):
        suffix='%03d'%int(next(row[0]for row in traps if row[1]=='00038ed2'))
        require((original/(name+'-'+suffix+'.bin')).read_bytes()==(original/(name+'-'+final+'.bin')).read_bytes(),'whole original initialized/saved/support state changed')
    return {'signed_qword':struct.unpack('<q',qword)[0],'rounded':rounded,'opaque_reserved_bytes':rounded,'native_live_depths':[len(r['fp']['live'])for r in records[:6]],'full_FP_IP_DP_empty_payload_equivalence':False}
