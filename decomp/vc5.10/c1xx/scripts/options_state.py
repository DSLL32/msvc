"""Own encountered option grammar, defaults, first helper and typed restore state."""
import copy,json,re,struct
from options_projection import SPECS,validate_layout
from options_probe import literal_defaults
from allocation_imports import pe_read
from scaler_probe import symbols

BASE=0x10400000
LITERALS={'path_separators':(0xda378,b'\\:/\0'),'il_prefix':(0xd06b8,b'\0'),'database_file':(0xda37c,b'vc50.pdb\0')}


def check(s,d,original,native,trial,text,pre,phase,state,seed,final,traps):
    def require(ok,message):s.require(ok,'options: '+message)
    def read(name):return json.loads((native/(name+'.json')).read_text())
    analysis=json.loads((original/'options-analysis.json').read_text());target=original/'loaded-target.bin'
    require(s.sha(target)==analysis['original_sha256'],'target inventory changed')
    require(trial['trial']in ('_process_compiler_options','_push_declaration_context','_initialize_include_aliases') and trial['options_parser_observers']['frontier_rva']==0x2b834,'wrong parser trial contract')
    names=symbols(native/'native-executable.elf');layout=read('options-grammar-layout');patterns=read('options-native-patterns')
    if (native/'options-abi-refinement.json').exists():
        from options_abi_refinement import check as check_refinement
        require(read('options-abi-refinement')==check_refinement(original,native),'scalar-context refinement receipt differs from actual inputs')
    storage=state['frontend']['storage'];model=read('state-model')
    def destination(rva):
        if rva==0xda388:return layout['letter_options']['address']
        fields=[v for v in model['fields']if v['original_address']==BASE+rva]
        require(len(fields)==1,'grammar destination is not a unique modeled alias')
        return storage[fields[0]['name']][0]
    require('options-data rules=100 letters=11 warnings=10 full-loaded-records=1 no-code-mock=1\n' in text,'loaded table observation absent')
    require(len(analysis['rules'])==100 and len(analysis['letter_rules'])==11 and len(analysis['warning_defaults'])==10,'table inventory wrong')
    for key,oldname,rva,size,stride in [('option_rules','options-rules-loaded',0xd6160,1212,12),('letter_options','options-letters-loaded',0xda388,144,12),('warning_defaults','options-warnings-loaded',0xd6628,88,8)]:
        raw=(original/(oldname+'.bin')).read_bytes();new=(native/(key+'-native.bin')).read_bytes()
        require(raw==pe_read(target,rva,size) and len(new)==size and layout[key]['bytes']==size and layout[key]['record_size']==stride,'actual table range/stride/immutable bytes differ')
        count=size//stride-1;require(raw[-stride:]==new[-stride:]==bytes(stride),'table terminator differs')
        for index in range(count):
            oldrow=raw[index*stride:(index+1)*stride];newrow=new[index*stride:(index+1)*stride]
            if key=='option_rules':
                ptr,dest,result,action=struct.unpack_from('<IIbB',oldrow);nptr,ndest,nresult,naction=struct.unpack_from('<IIbB',newrow);rule=analysis['rules'][index]
                require(ptr==BASE+rule['original_pattern_rva'] and dest==BASE+rule['destination_rva'] and (result,action)==(rule['result'],rule['action']),'primary original rule differs')
                require(pe_read(target,ptr-BASE,len(rule['pattern'])+1)==rule['pattern'].encode('ascii')+b'\0' and patterns[index]==rule['pattern'] and nptr and ndest==destination(dest-BASE) and (nresult,naction)==(result,action),'native primary literal/alias/action differs')
            elif key=='letter_options':
                letter,action,dest=struct.unpack('<iiI',oldrow);nletter,naction,ndest=struct.unpack('<iiI',newrow);rule=analysis['letter_rules'][index]
                require((letter,action,dest)==(ord(rule['letter']),rule['action'],BASE+rule['destination_rva']) and (nletter,naction,ndest)==(letter,action,destination(dest-BASE)),'Z-letter rule differs')
            else:
                code,flags=struct.unpack_from('<IB',oldrow);ncode,nflags=struct.unpack_from('<IB',newrow)
                require([code,flags]==analysis['warning_defaults'][index] and (code,flags)==(ncode,nflags),'warning default differs')
    require(patterns[-1] is None,'grammar sentinel pattern differs')
    require(layout['option_rules']['fields']=={'pattern':[0,4,'const char *'],'target':[4,4,'void *'],'result':[8,1,'signed char'],'action':[9,1,'unsigned char']} and layout['letter_options']['fields']=={'letter':[0,4,'int'],'action':[4,4,'int'],'target':[8,4,'int *']} and layout['warning_defaults']['fields']=={'code':[0,4,'unsigned int'],'flags':[4,1,'unsigned char']},'table semantic member layout differs')
    literals=read('options-native-literals');defaults=literal_defaults(native/'native-executable.elf')
    require(set(literals)==set(defaults),'literal inventory differs')
    for name,(rva,raw)in LITERALS.items():
        require(literals[name]==dict(defaults[name],actual_bytes=raw.hex()) and pe_read(target,rva,len(raw))==raw,'default literal identity/actual bytes differ')
    def original_values(suffix,kind,saved=False):
        if kind=='initialized':raw=(original/('frontend-'+('snapshot'if saved else'data')+'-'+suffix+'.bin')).read_bytes();base=0xcc000
        else:
            path=original/('options-bss-'+suffix+'.bin')
            if path.exists():
                raw=path.read_bytes();require(len(raw)==100,'typed BSS stream size differs');offset=0;values={}
                for name,rva,typ,width,lifetime in SPECS:
                    if lifetime=='bss':values[name]=struct.unpack_from('<i'if typ=='int'else'<I',raw,offset)[0];offset+=width
                return values
            require(not seed,'seeded state missing direct BSS observation')
            raw=(original/('pool-bss-%03d.bin'%(int(final)+1))).read_bytes();base=0xb9000
        values={}
        for name,rva,typ,width,lifetime in SPECS:
            if lifetime!=kind:continue
            item=raw[rva-base:rva-base+width];require(len(item)==width,'truncated original semantic field')
            values[name]=list(item)if width>4 else struct.unpack('<i'if typ=='int'else'<I',item)[0]
        return values
    for snapshot in (pre,phase,state):validate_layout(require,snapshot['frontend'])
    def equivalent(old,new,kind,owned=0,nowned=0):
        for index,(name,rva,typ,width,lifetime)in enumerate(SPECS):
            if lifetime!=kind:continue
            value=old[name];actual=new[name]
            if '*'in typ and value:
                if seed:
                    require(owned<=value<owned+8520 and actual==nowned+value-owned,'owned data-only pointer identity differs '+name)
                elif name=='include_aliases'and '_initialize_include_aliases'in trial['accepted']:
                    oldbase=struct.unpack_from('<I',(original/('arenas-'+final+'.bin')).read_bytes(),8)[0];newbase=state['frontend']['arenas'][2];reserve=state['frontend']['arenas'][5]
                    require(oldbase<=value<oldbase+reserve and actual==newbase+value-oldbase,'owned arena suballocation root identity differs')
                else:
                    require(name in LITERALS and value==BASE+LITERALS[name][0] and actual==defaults[name]['address'],'literal pointer identity differs '+name)
            else:require(value==actual,'typed semantic value differs '+name)
    initial=original_values('000','initialized')
    for name,rva,typ,width,kind in SPECS:
        if kind!='initialized':continue
        raw=pe_read(target,rva,width);value=list(raw)if width>4 else struct.unpack('<i'if typ=='int'else'<I',raw)[0]
        require(initial[name]==value,'loader default differs from own file-backed initializer '+name)
    if not seed:equivalent(initial,pre['frontend']['options_data'],'initialized')
    if (original/'options-bss-000.bin').exists():require(all(v==0 for v in original_values('000','bss').values()),'original loader BSS defaults not zero')
    oldowned=d.original_fields(original,'006')['saved_state'];nowned=pre['frontend']['saved_state']
    if seed:
        require(seed in ('restore-nonzero','restore-zero') and 'options-seed fields=106 initialized=81 bss=25 persistent=1 pointers=owned-interiors never-dereferenced=1 no-code-mock=1 allocation=%08x\n'%oldowned in text,'undeclared semantic seed roles')
        before_seed=read('options-before-seed')
        require(set(before_seed)=={v[0]for v in SPECS}|{'current_pdb_context'} and before_seed['current_pdb_context']==0,'pre-seed semantic inventory/persistent default differs')
        live=original_values('006','initialized');saved=original_values('006','initialized',True);bss=original_values('006','bss')
        for index,(name,rva,typ,width,kind)in enumerate(SPECS):
            oldbefore=initial[name]if kind=='initialized'else 0
            encoded=bytes(before_seed[name]and bytes.fromhex(before_seed[name]))
            if width>4:require(encoded==bytes(oldbefore),'native pre-seed array default differs '+name)
            elif '*'in typ and oldbefore:
                require(name in LITERALS and struct.unpack('<I',encoded)[0]==defaults[name]['address'],'native pre-seed literal identity differs '+name)
            else:require(struct.unpack('<i'if typ=='int'else'<I',encoded)[0]==oldbefore,'native pre-seed scalar default differs '+name)
            if kind=='initialized':
                require(live[name]==([0xa5]*width if width>4 else oldowned+4*(index+1)+512 if '*'in typ else 101+index),'wrong live restore input '+name)
                require(saved[name]==([0x5a]*width if width>4 else oldowned+4*(index+1)if '*'in typ else 17+index),'wrong saved restore input '+name)
            else:require(bss[name]==(oldowned+4*(index+1)+1024 if '*'in typ else 101+index),'BSS nonzero input omitted '+name)
        original_preseed=original_values('options-before-seed','initialized')
        require(original_preseed==initial,'original new semantic defaults changed before declared seed')
        require(all(v==0 for v in original_values('options-before-seed','bss').values()),'original new BSS defaults changed before declared seed')
        require(phase['frontend']['options_data']==pre['frontend']['saved_options_data']==phase['frontend']['saved_options_data']==state['frontend']['options_data']==state['frontend']['saved_options_data'],'new fields not restored from saved typed values')
        require(phase['frontend']['current_pdb_context']==pre['frontend']['current_pdb_context']==state['frontend']['current_pdb_context'],'persistent context incorrectly joined reset')
    for suffix,snapshot in [('006',pre),('phase',phase),(final,state)]:
        equivalent(original_values(suffix,'initialized'),snapshot['frontend']['options_data'],'initialized',oldowned,nowned)
        equivalent(original_values(suffix,'bss'),snapshot['frontend']['options_bss'],'bss',oldowned,nowned)
        if suffix!='006':
            equivalent(original_values(suffix,'initialized',True),snapshot['frontend']['saved_options_data'],'initialized',oldowned,nowned)
            if suffix=='phase':require((original/('frontend-data-'+suffix+'.bin')).read_bytes()==(original/('frontend-snapshot-'+suffix+'.bin')).read_bytes(),'complete ORIGINAL-only internal snapshot differs')
    require(all(v==0 for v in phase['frontend']['options_bss'].values()) and phase['frontend']['options_bss']==state['frontend']['options_bss'],'new BSS reset/preservation differs')
    for suffix,snapshot in [('006',pre),('phase',phase),(final,state)]:
        # Phase has no original support-register stream. Compare persistent
        # identity at actual pre/final boundaries, and native phase directly.
        if suffix=='phase':
            require(snapshot['frontend']['current_pdb_context']==pre['frontend']['current_pdb_context'],'native phase changed persistent PDB context')
            continue
        ptr=struct.unpack_from('<I',(original/('state-'+suffix+'.bin')).read_bytes())[0]
        require((ptr==BASE+0xcb030 and snapshot['frontend']['current_pdb_context']==storage['input_pdb_context.database'][0])if seed else ptr==snapshot['frontend']['current_pdb_context']==0,'persistent PDB context identity/reset lifetime differs')
    entries=read('options-helper-entries');require(len(entries)==1,'first helper count differs');entry=entries[0];cli=read('pool-cli-entries')[0]
    row=[r for r in traps if r[1]=='0002b834'];require(len(row)==1,'actual original helper trap absent');seq=int(row[0][0]);words=[int(v,16)for v in row[0][2].split(',')]
    require(words[:2]==[BASE+0x38fbe,0] and pe_read(target,0x38fb9,5)==b'\xe8'+struct.pack('<i',0x2b834-0x38fbe),'original NULL argument/caller differs')
    require(entry['pc']==names['push_declaration_context'][0] and entry['args']==[0] and entry['formal_inventory']==[dict(name='context',type='int' if (native/'options-abi-refinement.json').exists()or model['schema']>=33 else 'void *')] and (trial['trial']!='_process_compiler_options' or state['args']==dict(context=0)),'actual native helper inputs/formals differ')
    start,length=names['process_compiler_options'];ret=entry['return_pc'];code=bytes(d.elf_byte(native,ret-5+i)for i in range(5))
    require(start<ret<start+length and code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==entry['pc'],'native real CALL/ownership differs')
    expected_entry_guard=copy.deepcopy(cli['guard'])
    if trial.get('language_context_auxiliary'):
        seeded=bytearray.fromhex(expected_entry_guard['bss']);struct.pack_into('<10i',seeded,1504,*[1+(i&1)for i in range(10)]);expected_entry_guard['bss']=seeded.hex()
    require(entry['guard']==expected_entry_guard and entry['last_error']==cli['last_error']==203 and entry['fp']==cli['fp'],'parser changed state before first scalar-zero helper beyond declared data seed')
    expected_entry_bss=bytearray((original/('pool-bss-%03d.bin'%seq)).read_bytes())
    if trial.get('language_context_auxiliary'):struct.pack_into('<10i',expected_entry_bss,0xc13d0-0xb9000,*[1+(i&1)for i in range(10)])
    require((original/('pool-bss-%03d.bin'%(seq+1))).read_bytes()==bytes(expected_entry_bss),'original parser changed BSS before first helper beyond declared data seed')
    for prefix in ('frontend-data','frontend-snapshot','state'):
        require((original/(prefix+'-%03d.bin'%seq)).read_bytes()==(original/(prefix+'-%03d.bin'%(seq-1))).read_bytes(),'original parser changed prior initialized/saved/persistent state')
    return dict(new_fields=107,initialized_semantic_bytes=1824,bss_semantic_bytes=100,persistent_semantic_bytes=4,rules=100,z_rules=11,warning_defaults=10,first_helper_null_argument=True,unmodeled_initialized_bytes=6547)
