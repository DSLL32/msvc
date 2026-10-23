"""Bind the fill branch and closed C2 scope constructors to original operands."""
from pathlib import Path
import hashlib,importlib.util,json,re,struct,subprocess,time
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM
from binary import PE
started=time.perf_counter();config=json.loads(Path('functions.json').read_text());pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
spec=importlib.util.spec_from_file_location('coff_contract_graph',Path('scripts/audit-graph.py'));graph=importlib.util.module_from_spec(spec);spec.loader.exec_module(graph)
base=graph.FunctionDecoder;seen=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};seen.append(self)
    def instruction(self,address):
        item=super().instruction(address)
        if item is not None:self.visited[address]=item
        return item
graph.FunctionDecoder=Decoder
from pe_indexes import indexes
original,unused,cfg,records,canonical=indexes();analysis_only={}if '@coff_write_comment@4'in config['accepted']else{'@coff_write_comment@4':0x6ee8c}
records=[dict(r,end=analysis_only[r['name']],provisional=False)if r['name']in analysis_only else r for r in records]
slots={0x851a0:'section vector',0x851bc:'active scope'};refs={a:[]for a in slots};calls={0x6f1c0:[],0x6db80:[],0x6daa0:[]};instructions={}
for row in config['driver']:
    if row['name']not in config['accepted']and row['name']not in analysis_only:continue
    if row['name']in analysis_only:row=dict(row,end=analysis_only[row['name']],provisional=False)
    graph.edges(original,records,row);decoder=seen[-1]
    for address,size,mnemonic,operands in decoder.visited.values():
        instructions[address]=(size,mnemonic,operands,row['name'])
        if not (mnemonic=='call' or any(hex(a)in operands for a in slots)):continue
        ins=decoder.detail(address,size)
        for op in ins.operands:
            if op.type==X86_OP_MEM and op.mem.disp in slots:
                assert not op.mem.base and not op.mem.index and mnemonic=='mov','Global address escaped or gained unreviewed arithmetic'
                refs[op.mem.disp].append(dict(address=hex(address),field_address=address+ins.disp_offset,function=row['name'],instruction=mnemonic+' '+operands,access=op.access))
            elif op.type==X86_OP_IMM:
                assert op.imm not in slots,'Global address escape'
                if mnemonic=='call'and op.imm in calls:calls[op.imm].append(dict(address=hex(address),function=row['name']))
code=next(s for s in pe.sections if s['flags']&0x20);codebase=pe.base+code['rva'];data=pe.read(codebase,code['virtual_size'])
for address,uses in refs.items():
    raw={codebase+i for i in range(len(data)-3)if data[i:i+4]==struct.pack('<I',address)}
    assert raw=={r['field_address']for r in uses},(hex(address),'unclassified global reference',raw-{r['field_address']for r in uses})
assert {int(r['address'],16)for r in calls[0x6f1c0]}=={0x6ca3e,0x6d75a,0x6d914,0x6d998}
assert {r['function']for r in calls[0x6db80]}=={'@coff_initialize@4','@coff_open_contribution@8','@coff_write_comment@4'}
assert len(calls[0x6daa0])==1 and calls[0x6daa0][0]['function']=='_coff_open_module'
# These are the actual Section.state def/use instructions, not similarly
# numbered fields of contribution or vector records. EDI/EBX/EAX provenance
# is the section constructor/vector result in the named original functions.
state_addresses=[0x6e50b,0x6e59b,0x6e600,0x6e67a,0x6e67d,0x6e681,0x6e685,0x6e687,
 0x6e701,0x6e705,0x6e709,0x6e70d,0x6e806,0x6e80a,0x6e80f,0x6e814]
state=[]
for address in state_addresses:
    size,mnemonic,operands,owner=instructions[address];state.append(dict(address=hex(address),function=owner,instruction=mnemonic+' '+operands))
assert instructions[0x6e50b][1:3]==('or','word ptr [ebx + 0x12], 8')
assert instructions[0x6e59b][1:3]==('or','word ptr [ebx + 0x12], 8')
assert instructions[0x6e701][1:3]==('or','si, 1') and instructions[0x6e709][1:3]==('or','si, 2')
assert instructions[0x6e80f][1:3]==('and','dx, 0xfffd')
assert pe.read(0x6f201,10).hex()=='83fe50a1649007007744'
assert pe.read(0x6f243,12).hex()=='83430c5083ee5083fe5076bc'
assert pe.read(0x6f1fa,7).hex()=='b914000000f3ab' #20 DWORD stores establish80 defined scratch bytes.
nm=subprocess.check_output(['nm','-S','build/native/C23232.EXE'],text=True)
match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [bB] coff_fill_scratch$',nm,re.M);assert match and int(match[2],16)==80,'Native scratch extent differs'
Path('cases/coff-fill-contract.json').write_text(json.dumps(dict(target_sha256=hashlib.sha256(pe.data).hexdigest(),
 analysis_only={n:dict(end=hex(a),accepted=False,unmasked=False)for n,a in analysis_only.items()},caller_sites=calls[0x6f1c0],scope_constructor_sites=calls[0x6db80],configuration_site=calls[0x6daa0],global_operand_refs={hex(a):rows for a,rows in refs.items()},section_state_provenance=state,
 invariant='For the currently verified case and its private C2 COFF section construction domain: arena7 allocator starts zeroed; constructors add8, open adds1/2, close clears2. No method introduces state4; contribution kind=(state&4)==0 is1. String/trailer scopes also have explicit kind1. Global addresses do not escape; indexed and displacement references are bound to true operands and all literal reference candidates are accounted for.',
 alias_review='Section pointers are retained only by private section/contribution vectors and coff_current_section. Selected contributions supply active scopes; temporary name/trailer scopes restore or replace it with kind1. Allocator/error callbacks receive size or diagnostic arguments, not Section pointers. Source fields and original register/displacement def-use were reviewed; similarly numbered non-Section fields are not rewritten.',
 scratch=dict(original='7e8d0..7e920',original_initializer='20 DWORD REPSTOSD stores',native_size=80,native_symbol='coff_fill_scratch'),direct_branch='Original JA6f24f and JBE6f20b retain unsigned <=80. Typed80-byte scratch;102 independent original/native probes include mocked direct fwrite request shapes and fully defined buffered/count-only contents/counters.',
 limits=['No actual oversized direct-file read or adjacent bytes are represented or claimed matching. Mocked request proof stops at fwrite contract.', 'Case-specific scope proof, not a universal claim that future kind0 paths are unreachable. A future real oversized direct read must HOLD for root-approved bounded native policy; never invent adjacent storage or change predicates. Arbitrary corrupted states and external COFF library APIs are not covered. Failure callbacks remain named boundaries.'],seconds=time.perf_counter()-started),indent=2)+'\n')
print('4 original fill callers;3 scope constructors; section state/global operand contract verified')
