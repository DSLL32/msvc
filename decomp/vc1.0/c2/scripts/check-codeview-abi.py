"""Prove new CodeView entry ABIs in original and canonical machine code."""
from pathlib import Path
import importlib.util,json,time
from pe_indexes import indexes
started=time.perf_counter();spec=importlib.util.spec_from_file_location('codeview_abi_graph',Path('scripts/audit-graph.py'));graph=importlib.util.module_from_spec(spec);spec.loader.exec_module(graph)
base=graph.FunctionDecoder;observed=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};observed.append(self)
    def instruction(self,address):
        item=super().instruction(address)
        if item is not None:self.visited[address]=item
        return item
graph.FunctionDecoder=Decoder
original,canonical,config,left,right=indexes();checked=[]
for name in ['_initialize_codeview_output','_write_codeview_signature','_write_codeview_object_name','@copy_codeview_string@8']:
    sides=[]
    for image,records in [(original,left),(canonical,right)]:
        row=next(r for r in records if r['name']==name);graph.edges(image,records,row);instructions=list(observed[-1].visited.values());returns=[dict(address=hex(a),stack_bytes=int(op,0)if op else 0)for a,size,mn,op in instructions if mn.split()[-1]=='ret']
        if name=='_initialize_codeview_output'and image is original:
            assert not returns and (0x6910f,5,'jmp','0x69c60')in instructions
        else:assert returns and all(r['stack_bytes']==0 for r in returns),(name,returns)
        sides.append(returns)
    checked.append(dict(name=name,original=sides[0],canonical=sides[1],callee_stack_bytes=0,abi='fastcall two register arguments'if name.startswith('@')else'cdecl no arguments'))
Path('cases/codeview-abi-validation.json').write_text(json.dumps(dict(checked=checked,seconds=time.perf_counter()-started,tail='Original initializer tail-jumps to object-name writer; both use cdecl(void) with no callee stack cleanup'),indent=2)+'\n');print(len(checked),'CodeView cleanup ABIs match')
