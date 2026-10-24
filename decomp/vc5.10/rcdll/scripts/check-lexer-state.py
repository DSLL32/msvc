#!/usr/bin/env python3
"""Bounded data-reference review and production-object lexer branch controls.

This inspects data operands only; function extents and CFG remain Rust-audited.
Synthetic branch inputs stop at dependencies and are not original case outputs.
"""
import argparse,json,re,shutil,struct,subprocess,time
from pathlib import Path
import capstone
from protocol import HERE,ROOT,sha

p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
assert not a.output.exists();a.output.mkdir(parents=True);started=time.perf_counter()
for source,name in [('build/RCDLL.DLL','rebuilt.dll'),('build/rcdll.map','rebuilt.map'),('build/native/rcdll-host','production.elf'),('build/native/lexer-state-host','probe.elf'),('build/native/lexer.o','lexer.o'),('build/native-lexer-state-source.json','compiled-source.json')]:
    shutil.copy2(HERE/source,a.output/name)
target=ROOT/'toolchains/5.10/bin/RCDLL.DLL';assert sha(target)=='bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0'
mapping=(a.output/'rebuilt.map').read_text()
def public(name):
    rows=re.findall(r'\s_'+re.escape(name)+r'(?:@\d+)?\s+([0-9a-fA-F]{8})\b',mapping);assert len(rows)==1,name
    return int(rows[0],16)
def pe_span(data,address,length):
    pe=struct.unpack_from('<I',data,60)[0];opt=pe+24;base=struct.unpack_from('<I',data,opt+28)[0]
    for index in range(struct.unpack_from('<H',data,pe+6)[0]):
        _,start,size,offset=struct.unpack_from('<IIII',data,opt+struct.unpack_from('<H',data,pe+20)[0]+40*index+8)
        if start<=address-base and address-base+length<=start+size:
            raw=offset+address-base-start;return raw,data[raw:raw+length]
    raise AssertionError('unbacked PE range')
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True;md.skipdata=True
selected=['main','rc_next_token','rc_evaluate_condition','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_scan_line_start','rc_parse_define','rc_define_macro','rc_find_macro','rc_read_macro_replacement','rc_list_macro','rc_open_source']
config=json.loads((HERE/'functions.json').read_text());functions={f['name']:f for f in config['functions']}
def references(data,start,length,address,span):
    _,body=pe_span(data,start,length);rows=[]
    for ins in md.disasm(body,start):
        if not ins.id:continue
        for operand in ins.operands:
            if operand.type==capstone.x86.X86_OP_MEM and operand.mem.base==0 and address<=operand.mem.disp<address+span:
                rows.append(dict(pc=hex(ins.address),offset=operand.mem.disp-address,width=operand.size,instruction=ins.mnemonic+' '+ins.op_str,bytes=ins.bytes.hex(),displacement_offset=ins.disp_offset))
            elif operand.type==capstone.x86.X86_OP_IMM and address<=operand.imm<address+span:
                rows.append(dict(pc=hex(ins.address),offset=operand.imm-address,width=None,instruction=ins.mnemonic+' '+ins.op_str,bytes=ins.bytes.hex()))
    return rows
# One table: exact original storage, source name, actual access widths and roles.
spec=[('rc_expanding_macro',0x16f00,4,'macro expansion depth; zero boundary vs signed<=0 identifier dispatch'),('rc_source_depth',0x14a10,4,'include/source stack depth; distinct from expansion'),('rc_apstudio',0x25058,4,'command /S, macro listing and internal marker share one DWORD'),('rc_expression_token',0x149dc,1,'expression lookahead; distinct from lexer token'),('rc_token_kind',0x144f4,1,'lexer token byte'),('rc_expression_parentheses',0x18e7c,4,'expression parenthesis count'),('rc_in_condition_expression',0x18e6c,4,'directive expression mode'),('rc_in_directive',0x18530,4,'lexer newline/directive mode'),('rc_cpp_active',0x1811c,4,'preprocessor output state'),('rc_cpp_E',0x16cc4,4,'preprocessor output option'),('rc_cpp_nesting_level',0x149d4,4,'conditional nesting'),('rc_condition_state',0x227a8,128,'32 conditional state DWORDs'),('rc_macro_parameter_count',0x21e94,4,'macro definition parameter count'),('rc_macro_replacement',0x21e98,4,'macro replacement cursor'),('rc_macro_hash',0x16cec,1,'macro hash byte'),('rc_macro_name_length',0x16ce0,4,'name length DWORD; some callers consume only low BYTE'),('rc_macro_buckets',0x21ea0,1024,'256 macro pointer buckets'),('rc_source_cursor',0x18120,4,'UTF16 cursor pointer'),('rc_preparing_line',0x16ce8,4,'current source line'),('rc_prepared_lines',0x29458,4,'aggregate prepared line counter'),('rc_filename',0x149c4,4,'current source filename pointer'),('rc_header_mode',0x16efc,4,'include-name lexer mode'),('rc_token_value',0x16cd0,4,'token value pointer; operator path writes only low BYTE'),('rc_token_extra',0x16cd4,2,'intentional union: hashBYTE0/lengthBYTE1 or string lengthWORD'),('rc_macro_listing_readonly',0x28fc8,4,'listing readonly state'),('rc_macro_listing_hidden',0x28fcc,4,'listing hidden state')]
original=target.read_bytes();rebuilt=(a.output/'rebuilt.dll').read_bytes();table=[]
for name,rva,size,role in spec:
    record=dict(source_symbol=name,target_address=hex(0x71c20000+rva),storage_bytes=size,rebuilt_address=hex(public(name)),role=role,functions={})
    for function in selected:
        f=functions[function];old=references(original,0x71c20000+int(f['start'],16),int(f['end'],16)-int(f['start'],16),0x71c20000+rva,size)
        # MAP public next ownership is enough for this bounded operand review;
        # use function's actual COFF/MAP symbol size via following public code.
        start=public(function);following=[int(v,16)for v in re.findall(r'\s_[A-Za-z][\w@]*\s+([0-9a-fA-F]{8})\s+f\s',mapping)if int(v,16)>start]
        end=min(following);new=references(rebuilt,start,end-start,public(name),size)
        if old or new:
            assert old and new,(function,name,'one-sided data identity')
            old_widths={r['width']for r in old if r['width']};new_widths={r['width']for r in new if r['width']}
            assert new_widths<=old_widths or name in ['rc_macro_name_length','rc_token_value','rc_token_extra'],(function,name,old_widths,new_widths)
            record['functions'][function]=dict(original=old,rebuilt=new)
    assert record['functions'],name
    table.append(record)
by_name={r['source_symbol']:r for r in table}
assert 'rc_next_token'not in by_name['rc_source_depth']['functions']
for name in ['rc_define_macro','rc_list_macro','rc_next_token','main']:assert name in by_name['rc_apstudio']['functions']
assert not re.search(r'\s_(?:rc_keep_internal_marker|rc_macro_listing_enabled)\s',mapping)
meta=json.loads((HERE/'preprocess-data.json').read_text());alias=next(v for v in meta['globals']if v['name']=='rc_macro_listing_enabled')
assert alias['native_expression']=='rc_apstudio'and int(alias['original_rva'],16)==0x25058 and alias['bytes']==4
source=json.loads((a.output/'compiled-source.json').read_text())['contents']
assert all(not re.search(r'\brc_(?:keep_internal_marker|macro_listing_enabled)\b',v['text'])for k,v in source.items()if '/src/'in k)
def elf_body(path):
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(path)],text=True)
    rows=re.findall(r'^([0-9a-f]+) ([0-9a-f]+) T rc_next_token$',symbols,re.M);assert len(rows)==1
    address,size=[int(v,16)for v in rows[0]];image=path.read_bytes();offset=struct.unpack_from('<I',image,28)[0];width,count=struct.unpack_from('<HH',image,42)
    for index in range(count):
        kind,raw,start,_,length=struct.unpack_from('<5I',image,offset+width*index)
        if kind==1 and start<=address and address+size<=start+length:return bytearray(image[raw+address-start:raw+address-start+size])
    raise AssertionError('ELF function not backed')
# Prove the auxiliary and production functions are the same object code,
# allowing only the object's actual relocation fields and GOT32X relaxation.
obj=(a.output/'lexer.o').read_bytes();offset=struct.unpack_from('<I',obj,32)[0];width,count=struct.unpack_from('<HH',obj,46)
sections=[struct.unpack_from('<10I',obj,offset+width*index)for index in range(count)];names_section=sections[struct.unpack_from('<H',obj,50)[0]];names=obj[names_section[4]:names_section[4]+names_section[5]]
section_names=[names[row[0]:names.index(b'\0',row[0])].decode()for row in sections]
text_section=sections[section_names.index('.text')];object_body=bytearray(obj[text_section[4]:text_section[4]+text_section[5]])
relocation_section=sections[section_names.index('.rel.text')];relocations=[struct.unpack_from('<II',obj,at)for at in range(relocation_section[4],relocation_section[4]+relocation_section[5],8)]
production_body=elf_body(a.output/'production.elf');probe_body=elf_body(a.output/'probe.elf');assert len(production_body)==len(probe_body)==len(object_body)
for at,info in relocations:
    kind=info&255;assert kind in [4,9,10,43]and 0<=at<=len(object_body)-4
    for body in [object_body,production_body,probe_body]:
        body[at:at+4]=b'\0'*4
        if kind==43:
            assert body[at-2]in [0x8b,0x8d];body[at-2]=0x8b
assert object_body==production_body==probe_body,'auxiliary differs outside genuine linker relocation fields'
object_identity=dict(function='rc_next_token',bytes=len(object_body),actual_object_relocations=len(relocations),scope='All non-relocation bytes match the same production lexer.o; only recorded ELF relocation fields and standard GOT32X MOV-to-LEA relaxation differ.')
record=by_name['rc_apstudio'];record['intentional_observer_aliases']=['rc_macro_listing_enabled -> native_expression rc_apstudio'];record['retired_wrong_storage']=['rc_keep_internal_marker','rc_macro_listing_enabled source definition']
controls=[]
def canonical_operand(image,row,expected):
    pc=int(row['pc'],16);_,body=pe_span(image,pc,len(bytes.fromhex(row['bytes'])))
    instruction=next(md.disasm(body,pc))
    assert any(op.type==capstone.x86.X86_OP_MEM and op.mem.disp==expected for op in instruction.operands),'wrong canonical storage operand'
# Coherent same-width duplicate-storage mutations still fail address identity,
# even when all stored values happen zero. No outer receipt hash is tested here.
for name,function,wrong in [('rc_expanding_macro','rc_next_token','rc_source_depth'),('rc_apstudio','rc_next_token','rc_result'),('rc_apstudio','rc_define_macro','rc_result'),('rc_apstudio','rc_list_macro','rc_result'),('rc_expression_token','rc_evaluate_condition','rc_token_kind')]:
    row=next(r for r in by_name[name]['functions'][function]['rebuilt']if r.get('displacement_offset'))
    canonical_operand(rebuilt,row,public(name)+row['offset'])
    pc=int(row['pc'],16);raw,_=pe_span(rebuilt,pc,len(bytes.fromhex(row['bytes'])));mutant=bytearray(rebuilt)
    struct.pack_into('<I',mutant,raw+row['displacement_offset'],public(wrong));assert mutant!=rebuilt
    _,changed=pe_span(mutant,pc,len(bytes.fromhex(row['bytes'])));ins=next(md.disasm(changed,pc));addresses=[op.mem.disp for op in ins.operands if op.type==capstone.x86.X86_OP_MEM]
    assert public(wrong)in addresses and public(name)not in addresses
    try:canonical_operand(mutant,row,public(name)+row['offset'])
    except AssertionError as error:assert str(error)=='wrong canonical storage operand'
    else:raise AssertionError('coherent wrong-storage mutant passed')
    controls.append(dict(name=name+'-'+function+'-coherent-wrong-storage',wrong_symbol=wrong,semantic_rejection='actual data operand is not the canonical MAP symbol',same_width=True))
probe=a.output/'probe.elf';runs=[]
for identifier,expansion,depth,expected in [(1,0,7,'rc_read_identifier'),(1,1,0,'rc_read_macro_identifier'),(1,-1,7,'rc_read_identifier'),(0,0,7,'rc_refill_source'),(0,1,0,'rc_resume_input'),(0,-1,0,'rc_resume_input')]:
    label='%d_%d_%d'%(identifier,expansion,depth);script=a.output/(label+'.gdb.py')
    script.write_text("import gdb,json\nfrom pathlib import Path\nf=gdb.newest_frame()\nPath("+repr(str(a.output/(label+'.json')))+").write_text(json.dumps(dict(function=f.name(),caller=f.older().name(),expansion=int(gdb.parse_and_eval('rc_expanding_macro')),depth=int(gdb.parse_and_eval('rc_source_depth')),expansion_address=int(gdb.parse_and_eval('&rc_expanding_macro')),depth_address=int(gdb.parse_and_eval('&rc_source_depth')),expansion_bytes=gdb.parse_and_eval('rc_expanding_macro').type.sizeof,depth_bytes=gdb.parse_and_eval('rc_source_depth').type.sizeof,pc=int(gdb.parse_and_eval('$pc'))),indent=2)+'\\n')\n")
    command=['gdb','--batch','-q',str(probe),'-ex','set pagination off']
    for name in ['rc_read_identifier','rc_read_macro_identifier','rc_refill_source','rc_resume_input']:command+=['-ex','break '+name]
    command+=['-ex','run %d %d %d'%(identifier,expansion,depth),'-ex','source '+str(script)]
    before=time.perf_counter();result=subprocess.run(command,capture_output=True,timeout=5)
    (a.output/(label+'.stdout')).write_bytes(result.stdout);(a.output/(label+'.stderr')).write_bytes(result.stderr)
    assert result.returncode==0
    state=json.loads((a.output/(label+'.json')).read_text());assert state['function']==expected and state['caller']=='rc_next_token'
    assert state['expansion']==expansion and state['depth']==depth and state['expansion_address']!=state['depth_address']and state['expansion_bytes']==state['depth_bytes']==4
    runs.append(dict(input=[identifier,expansion,depth],observed=state,seconds=time.perf_counter()-before))
report=dict(status='pass',target_sha256=sha(target),table=table,production_object_identity=object_identity,branch_controls=runs,duplicate_storage_controls=controls,scope='Bounded static operand-identity review, no CFG discovery or new function/data credit. Auxiliary native runs use real unmodified production lexer.o with synthetic states and stop at selected dependency before executing it. APSTUDIO marker/listing positive behavior remains unreached by current ASCII case; static canonical state identity is verified, not those complete behaviors. Historical equal-value/raw receipts are unchanged and not retrospectively upgraded.',files={f.name:sha(f)for f in a.output.iterdir()if f.is_file()},checker_sha256=sha(Path(__file__)),seconds=time.perf_counter()-started)
(a.output/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status='pass',globals=len(table),branches=len(runs),controls=len(controls),seconds=report['seconds'])))
