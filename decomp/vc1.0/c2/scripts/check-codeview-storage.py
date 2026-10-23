"""Typed CodeView cursor/allocation storage identity in accepted CFGs."""
from pathlib import Path
import copy,importlib.util,json,struct,time
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM
from pe_indexes import indexes,publics
started=time.perf_counter();pe,canonical,config,left,right=indexes();names=publics();mapping={0x7e8ac:names['_codeview_scope_cursor'],0x84ff8:names['_codeview_allocation_class'],0x7b8ec:names['_codeview_type_file'],0x85d9c:names['_input_file']}
def word(image,a):
    section=next(s for s in image.sections if image.base+s['rva']<=a<image.base+s['rva']+s['virtual_size'])
    return 0 if section['flags']&0x80 else struct.unpack('<I',image.read(a,4))[0]
assert len(set(mapping.values()))==4
for a,b in mapping.items():assert word(pe,a)==word(canonical,b)==0
spec=importlib.util.spec_from_file_location('codeview_storage_graph',Path('scripts/audit-graph.py'));g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g);base=g.FunctionDecoder;decoders=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};decoders.append(self)
    def instruction(self,a):
        item=super().instruction(a)
        if item is not None:self.visited[a]=item
        return item
g.FunctionDecoder=Decoder
def references(image,records,row,addresses):
    body=image.read(row['start'],row['end']-row['start'])
    if not any(struct.pack('<I',a)in body for a in addresses):return []
    g.edges(image,records,row);d=decoders[-1];found=[]
    for a,size,mn,ops in d.visited.values():
        for n,op in enumerate(d.detail(a,size).operands):
            value=op.mem.disp&0xffffffff if op.type==X86_OP_MEM else op.imm&0xffffffff if op.type==X86_OP_IMM else None
            if value in addresses:found.append(dict(site=hex(a),storage=hex(value),mnemonic=mn,operand=n,type='memory'if op.type==X86_OP_MEM else'address',width=op.size))
    return found
right_names={r['name']:r for r in right};bindings=[]
for row in left:
    if row['name']not in config['accepted']or row['name']not in right_names:continue
    a=references(pe,left,row,mapping);b=references(canonical,right,right_names[row['name']],set(mapping.values()))
    if a or b:bindings.append(dict(function=row['name'],original=a,canonical=b))
def validate(rows,storage):
    assert storage==mapping and len(set(storage.values()))==4
    for row in rows:
        assert {hex(storage[int(r['storage'],16)])for r in row['original']}=={r['storage']for r in row['canonical']}
        assert all(r['width']==4 for r in row['original']+row['canonical'])
    assert {'@end_debug_function@4','@write_debug_scopes@4','@write_debug_symbol@4','_finish_debug_module','@read_codeview_initializers@4'}<={r['function']for r in rows}
validate(bindings,mapping);rejected=[]
for label,change in [('separate same-zero allocation storage',lambda b,m:m.update({0x84ff8:m[0x84ff8]+4})),('missing allocation def-use',lambda b,m:next(r for r in b if r['function']=='@write_debug_symbol@4').update(canonical=[])),('short cursor access',lambda b,m:next(r for r in b if r['function']=='@write_debug_scopes@4')['canonical'][0].update(width=2))]:
    b=copy.deepcopy(bindings);m=mapping.copy();change(b,m)
    try:validate(b,m)
    except AssertionError:rejected.append(label)
    else:raise AssertionError('Accepted wrong CodeView storage identity: '+label)
Path('cases/codeview-storage-validation.json').write_text(json.dumps(dict(mapping={hex(a):hex(b)for a,b in mapping.items()},bindings=bindings,rejected=rejected,seconds=time.perf_counter()-started,scope='Original zero-initialized7e8ac DebugScope cursor and84ff8 allocation DWORD plus7b8ec db FILE owner bind to distinct existing typed canonical storage. All accepted CFG direct-memory/address-construction operands including displacement/base/index forms checked; byte filter only selects candidates. No arbitrary indirect alias/deep scope traversal proof. Actual entry cursor/head/override observations separate.'),indent=2)+'\n');print(len(bindings),'CodeView storage bindings and3 negatives pass')
