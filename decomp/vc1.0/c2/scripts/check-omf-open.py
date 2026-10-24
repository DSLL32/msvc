"""Whole OMF opener controls, zero-input ABI and contextual data identities."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument("--abi-only",action="store_true");args=ap.parse_args()
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();name='_omf_open_module';row=next(r for r in config['driver']if r['name']==name);path=Path(row['abi']['entry_observation']);receipt=json.loads(path.read_text());binding=json.loads(path.with_name('original-binding.json').read_text());sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();assert all(sha(path.parent/n)==h for n,h in binding['files'].items())
def validate(r):
    assert r['matched']and r['function']==name and r['profile']=='omf'and r['no_hidden_inputs_proof']
    assert r['formals']==row['abi']['parameters']==[]and r['hidden_inputs']==row['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
    assert r['original_values']==r['native']['values']==[]and r['omf_state']==dict(coff_object=0)and r['native']['coff_object']==0
    assert r['native']['stdout']=={k:v for k,v in r['original_stdout'].items()if k not in ['counter','cursor','value','position']}
    assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}and r['native']['column']==0xffffffff
validate(receipt);rejected=[]
for label,change in [('invented formal',lambda r:r.update(formals=[dict(name='guess')])),('unknown hidden argument',lambda r:r.update(hidden_inputs=['out?'])),('wrong format state',lambda r:r['native'].update(coff_object=1)),('wrong initial column',lambda r:r['native'].update(column=0)),('missing no-input proof',lambda r:r.update(no_hidden_inputs_proof=''))]:
    bad=copy.deepcopy(receipt);change(bad)
    try:validate(bad)
    except(AssertionError,KeyError):rejected.append(label)
    else:raise AssertionError('Accepted wrong OMF entry '+label)
def word_image(p,a):
    section=next(s for s in p.sections if p.base+s['rva']<=a<p.base+s['rva']+s['virtual_size'])
    return 0 if section['flags']&0x80 else struct.unpack('<I',p.read(a,4))[0]
assert word_image(original,0x84fc4)==0x84fb8 and original.read(0x84fb8,8)==bytes(8)
canonical_group=word_image(canonical,names['_omf_default_group']);assert canonical_group==names['_omf_group_storage'];section=next(s for s in canonical.sections if canonical.base+s['rva']<=canonical_group<canonical.base+s['rva']+s['virtual_size']);assert section['flags']&0x80 or canonical.read(canonical_group,8)==bytes(8)
fields={'object_file':0x84fb0,'object_filename':0x80600,'source_filename':0x805ec,'generated_source':0x85db4,'object_generation_started':0x83158,'floating_frontend':0x83150,'codeview_debug':0x8313c,'dos_object':0x80684,'target_segment_group':0x7b8e4,'code_segment':0x7b8d4,'data_segment':0x7b8d8,'const_segment':0x7b8dc,'bss_segment':0x7b8d0,'debug_symbols_segment':0x7914c,'debug_types_segment':0x7b8e0,'segment_list':0x84fac,'omf_default_group':0x84fc4,'omf_empty_name':0x7a8d8,'omf_next_group':0x8507c,'omf_next_segment':0x85080,'omf_next_frame_thread':0x85088,'omf_next_target_thread':0x8508c}
for n,a in [('omf_next_group',0x8507c),('omf_next_segment',0x85080),('omf_next_frame_thread',0x85088),('omf_next_target_thread',0x8508c)]:assert word_image(original,a)==word_image(canonical,names['_'+n])==(1 if n in ['omf_next_group','omf_next_segment']else 0)
assert word_image(original,0x80d40)==word_image(canonical,names['_listing_column'])==0xffffffff
assert row['start']==0x6a350 and row['end']==0x6a7e5 and original.read(0x6a7e4,1)==b'\xc3'
if args.abi_only:print('OMF opener actual no-input ABI,5 rejects,extent/group storage/counter PE identities pass');raise SystemExit
calls={'init':('_omf_initialize_records',0x6a9a0),'header':('@omf_write_module_header@4',0x6a980),'directive':('@add_module_directive@8',0x69120),'directives':('_write_module_directives',0x69420),'flush':('_omf_flush_records',0x6a7f0),'name':('@omf_name_index@4',0x6a940),'segment':('@omf_write_segment_definition@4',0x6a890),'group':('@omf_write_group_definition@8',0x6a850),'thread':('@omf_define_fixup_thread@16',0x6a800),'display':('@symbol_display_name@4',0x4e000),'fopen':('_fopen',0x71fd0),'ftell':('_ftell',0x71d50),'error':('@report_context_error@12',0x70840)}
def run(image,is_canonical,debug,floating,dos,extra,open_fail,mutate):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x30000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    put=lambda a,v:u.mem_write(a,struct.pack('<I',v&0xffffffff));word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
    globals={n:names['_'+n]if is_canonical else a for n,a in fields.items()}
    def set(n,v):put(globals[n],v)
    def get(n):return word(globals[n])
    texts={};cursor=[0x220000]
    def text(s):
        if s not in texts:texts[s]=cursor[0];u.mem_write(cursor[0],s.encode()+b'\0');cursor[0]+=len(s)+1
        return texts[s]
    def string(p):return bytes(u.mem_read(p,256)).split(b'\0')[0].decode()
    for n,v in [('object_file',0),('object_filename',text('omftypes.obj')),('source_filename',text('omftypes.c')),('generated_source',text('compiler generated')),('object_generation_started',0),('floating_frontend',floating),('codeview_debug',debug),('dos_object',dos),('target_segment_group',text('FLAT'if dos else'DGROUP')),('omf_default_group',0x205000),('omf_empty_name',0),('omf_next_group',1),('omf_next_segment',1),('omf_next_frame_thread',0),('omf_next_target_thread',0)]:set(n,v)
    u.mem_write(0x205000,b'\xa5'*8);segments=[]
    for i,(n,label,cls)in enumerate([('code_segment','_TEXT','CODE'),('data_segment','_DATA','DATA'),('const_segment','CONST','CONST'),('bss_segment','_BSS','BSS'),('debug_symbols_segment','$$SYMBOLS','DEBSYM'),('debug_types_segment','$$TYPES','DEBTYP')]):
        p=0x200000+i*128;set(n,p);u.mem_write(p,bytes(72));put(p+16,text(label));put(p+40,text(cls));put(p+48,4);put(p+64,i+1);u.mem_write(p+52,bytes([1 if i in [0,4,5]else 0,0 if i>=4 else 2,i,0]));segments.append(p)
    if not debug:segments=segments[:4]
    if extra:
        # Ordinary unnamed/custom classes and separately owned COMDAT symbols.
        for k in range(4):
            p=0x201000+k*128;u.mem_write(p,bytes(72));put(p+16,text(''if k==0 else'custom'+str(k)));put(p+40,text(''if k==0 else'CUSTOM'));u.mem_write(p+52,bytes([0,2,1,0]));segments.append(p)
            if k>=2:
                owner=0x207000+k*128;u.mem_write(owner,bytes(84));u.mem_write(owner+12,bytes([4 if extra!=2 else 1]));put(owner+36,0x50 if extra!=3 else 0x30);u.mem_write(owner+82,bytes([0x20]));put(owner+16,text(''if k==2 else'function_owner'));put(p+64,owner);u.mem_write(p+53,bytes([7]));u.mem_write(p+52,b'\1')
    for i,p in enumerate(segments):put(p+36,segments[i+1]if i+1<len(segments)else 0)
    set('segment_list',segments[0]);put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);targets={names[n]if is_canonical else a:k for k,(n,a)in calls.items()};events=[];name_indices={};position=[100]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in targets:return
        call=targets[a];esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0;pop=4
        if call=='fopen':assert string(word(esp+4))=='omftypes.obj'and string(word(esp+8))=='w';events.append(['fopen']);result=0 if open_fail else 0x206000
        elif call=='error':assert ecx==0x53 and string(edx)=='compiler generated'and string(word(esp+4))=='omftypes.obj';events.append(['error']);pop=8
        elif call=='header':assert string(ecx)=='omftypes.c';events.append(['header'])
        elif call=='directive':events.append(['directive',ecx,string(edx)])
        elif call=='name':
            value=string(ecx)
            if value not in name_indices:name_indices[value]=len(name_indices)+1
            result=name_indices[value];events.append(['name',value,result])
            if mutate and value:set('omf_empty_name',0x789)
        elif call=='segment':
            assert ecx in segments and word(ecx+64)==get('omf_next_segment'),(hex(ecx),word(ecx+64),get('omf_next_segment'));set('omf_next_segment',get('omf_next_segment')+1);events.append(['segment',ecx]);position[0]+=23
        elif call=='group':assert ecx in segments and edx==0x205000;put(ecx+24,edx);events.append(['group',ecx]);position[0]+=7
        elif call=='thread':
            target=word(esp+4);thread=word(esp+8);events.append(['thread',ecx&255,edx&255,target,thread]);result=thread;pop=12
        elif call=='display':assert ecx in [0x207100,0x207180];result=word(ecx+16);events.append(['display',string(result)])
        elif call=='ftell':assert word(esp+4)==(0 if open_fail else 0x206000);result=position[0];events.append(['ftell',result])
        else:events.append([call])
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+pop)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if is_canonical else row['start'],0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,segments=[bytes(u.mem_read(p,72)).hex()for p in segments],group=bytes(u.mem_read(0x205000,8)).hex(),threads=bytes(u.mem_read(names['_omf_fixup_threads']if is_canonical else 0x7a8c4,20)).hex(),globals={n:get(n)for n in ['object_file','object_generation_started','omf_empty_name','omf_next_group','omf_next_segment','omf_next_frame_thread','omf_next_target_thread']})
rows=[]
for values in itertools.product([0,1],[0,1],[0,1],range(4),[0,1],[0,1]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a['events'],b['events']);rows.append(dict(input=values,event_count=len(a['events'])))
Path('cases/omf-open-validation.json').write_text(json.dumps(dict(actual_zero_input_abi=True,rejected=rejected,cases=rows,default_group_identity=dict(original_pointer='0x84fc4',original_object='0x84fb8',canonical_pointer=hex(names['_omf_default_group']),canonical_object=hex(canonical_group),bytes=8,initial='two zero DWORDs with existing self-owned pointer'),seconds=time.perf_counter()-started,scope='Whole1173-byte original/canonical opener, original string/directive bytes, true zero-input ABI and exact PE column/counters/group pointer.128 controls compare all72-byte standard/extra segment descriptors, OMF class/header overlays, threads/group/counters/call order across C/alternate language, debug on/off, DOS/group modes, owned COMDAT eligibility and empty/custom names. Name/vector/record/file/symbol/error children explicitly mocked; controlled error callback returning is not real error-path acceptance. Actual case and each child frontier independently observed; no new CRT implementation.'),indent=2)+'\n');print(len(rows),'whole original/canonical OMF open states and5 ABI rejects pass')
