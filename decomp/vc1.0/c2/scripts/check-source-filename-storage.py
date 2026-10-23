"""Bind the primary/current/diagnostic filenames to distinct existing storage."""
from pathlib import Path
import copy,hashlib,importlib.util,json,struct,time
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM
from pe_indexes import indexes,publics
started=time.perf_counter();pe,canonical,config,left,right=indexes();symbols=publics()
mapping={0x805ec:symbols['_source_filename'],0x7901c:symbols['_current_source_filename'],0x85db4:symbols['_generated_source']}
assert len(set(mapping.values()))==3
def word(image,a):
    section=next(s for s in image.sections if image.base+s['rva']<=a<image.base+s['rva']+s['virtual_size'])
    return 0 if section['flags']&0x80 else struct.unpack('<I',image.read(a,4))[0]
for a in [0x805ec,0x7901c]:assert word(pe,a)==word(canonical,mapping[a])==0
assert pe.string(word(pe,0x85db4))==canonical.string(word(canonical,mapping[0x85db4]))=='compiler generated'
# Option is12 bytes, not a native host-pointer layout. Bind the actual-f# row.
assert pe.string(word(pe,0x80884))=='-f#'and word(pe,0x80888)==0x805ec and pe.read(0x8088d,1)==b'\x22'
option=symbols['_compiler_options']
for i in range(100):
    pattern=word(canonical,option+i*12)
    if not pattern:raise AssertionError('Missing canonical-f# option')
    if canonical.string(pattern)=='-f#':break
assert word(canonical,option+i*12+4)==mapping[0x805ec]and canonical.read(option+i*12+9,1)==b'\x22'
spec=importlib.util.spec_from_file_location('filename_storage_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;observed=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};observed.append(self)
    def instruction(self,a):
        item=super().instruction(a)
        if item is not None:self.visited[a]=item
        return item
g.FunctionDecoder=Decoder
def references(image,records,row,addresses):
    # Lossless candidate filter for these absolute32-bit data addresses; decoded
    # operands/CFG, not this byte search, supply the binding proof.
    body=image.read(row['start'],row['end']-row['start'])
    if not any(struct.pack('<I',a)in body for a in addresses):return []
    g.edges(image,records,row);d=observed[-1];found=[]
    for a,size,mn,ops in d.visited.values():
        for n,op in enumerate(d.detail(a,size).operands):
            value=op.mem.disp&0xffffffff if op.type==X86_OP_MEM else op.imm&0xffffffff if op.type==X86_OP_IMM else None
            if value in addresses:found.append(dict(site=hex(a),storage=hex(value),mnemonic=mn,operand=n,type='memory'if op.type==X86_OP_MEM else'address',width=op.size))
    return found
rindex={r['name']:r for r in right};bindings=[]
for row in left:
    if row['name']not in config['accepted']or row['name']not in rindex:continue
    a=references(pe,left,row,mapping);b=references(canonical,right,rindex[row['name']],set(mapping.values()))
    if a or b:bindings.append(dict(function=row['name'],original=a,canonical=b))
def validate(values,storage):
    assert len(set(storage.values()))==3 and storage==mapping
    for row in values:
        assert {hex(storage[int(r['storage'],16)])for r in row['original']}=={r['storage']for r in row['canonical']},row
        assert all(r['width']==4 for r in row['original']+row['canonical'])
    assert {'@prepare_source_listing@4','@emit_machine_instructions@4','@coff_begin_function@16','_coff_open_module','_parse_options'}<={r['function']for r in values}
validate(bindings,mapping);rejected=[]
for label,edit in [('same-valued separate source storage',lambda b,m:m.update({0x805ec:m[0x805ec]+4})),('wrong function filename domain',lambda b,m:next(r for r in b if r['function']=='@coff_begin_function@16')['canonical'][0].update(storage=hex(m[0x85db4]))),('wrong current-source store domain',lambda b,m:next(r for r in b if r['function']=='@prepare_source_listing@4')['canonical'][0].update(storage=hex(m[0x805ec])))]:
    b=copy.deepcopy(bindings);m=mapping.copy();edit(b,m)
    try:validate(b,m)
    except AssertionError:rejected.append(label)
    else:raise AssertionError('Accepted wrong filename domain: '+label)
Path('cases/source-filename-storage-validation.json').write_text(json.dumps(dict(mapping={hex(k):hex(v)for k,v in mapping.items()},bindings=bindings,option_original='0x80884',option_canonical=hex(option+i*12),rejected=rejected,seconds=time.perf_counter()-started,scope='Three existing distinct typed storages: primary805ec, current7901c, diagnostic85db4. Initial PE values/string and actual-f# option destination/action. All accepted original/canonical CFG instruction memory/address operands, including base/index displacement and address escape constructions, bind to the same domain sets per function. Does not prove arbitrary indirect aliases or unaccepted code. Failed begin-lines handle10/12 and callback formal pointer evidence retained separately; no handle normalization or new initialization.'),indent=2)+'\n');print(len(bindings),'accepted filename domain bindings and3 negatives pass')
