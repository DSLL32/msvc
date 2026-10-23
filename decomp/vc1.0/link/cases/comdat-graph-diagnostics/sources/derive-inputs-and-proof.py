from pathlib import Path
import sys,json,struct,hashlib
sys.path.insert(0,'scripts')
from indexes import indexes
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
p,*_=indexes();case=Path('cases/comdat-graph-diagnostics');d=Cs(CS_ARCH_X86,CS_MODE_32)
targets=[('comdat_usage_label',0x17520,0x17538,'cdecl const char *(ComdatNode *)'),('print_comdat_node',0x17540,0x175d3,'cdecl int(ComdatNode *); actual final diagnostic printer result'),('print_comdat_edge',0x175e0,0x175f1,'cdecl int(ComdatEdge *); delegates result'),('print_comdat_graph',0x17600,0x176df,'cdecl int(ComdatNode *); actual final fflush result'),('debug_printf',0x3a880,0x3a8ff,'cdecl int(const char *,...); actual consumed varargs; no defined result claimed when all routes suppressed')]
section=next(s for s in p.sections if s['name']=='.text');textbase=p.base+section['rva'];text=p.read(textbase,section['virtual_size']);proof=[]
for name,start,end,abi in targets:
 ins=list(d.disasm(p.read(start,end-start),start));assert sum(i.size for i in ins)==end-start
 (case/'analysis'/f'{name}.txt').write_text(''.join('%08x %-32s %s %s\n'%(i.address,i.bytes.hex(),i.mnemonic,i.op_str) for i in ins))
 calls=[dict(site=hex(i.address),target=i.op_str) for i in ins if i.mnemonic=='call']
 xrefs=[textbase+i for i in range(len(text)-4) if text[i] in (0xe8,0xe9) and textbase+i+5+struct.unpack_from('<i',text,i+1)[0]==start]
 absolute=[];q=0
 while True:
  q=p.data.find(struct.pack('<I',start),q)
  if q<0:break
  absolute.append(hex(q));q+=1
 proof.append(dict(name=name,start=start,end=end,abi=abi,extent_bytes=end-start,calls=calls,rel32_xrefs=[hex(x) for x in xrefs],allfile_dword_occurrences=absolute,code_sha256=hashlib.sha256(p.read(start,end-start)).hexdigest()))
# This is a caller-owned serialized input, never original data copied into implementation.
base=0x12000000;data=bytearray(b'\xa5'*4096)
def zero(offset,size):data[offset:offset+size]=b'\0'*size
def put(offset,*values):struct.pack_into('<'+'I'*len(values),data,offset,*values)
def string(offset,s):data[offset:offset+len(s)+1]=s+b'\0';return base+offset
sectionname=string(0x300,b'.textXYZlong');groupname=string(0x340,b'.text$grouplong');modulepath=string(0x380,b'graph.obj');nodename=string(0x3c0,b'entry')
for offset,size in [(0x40,20),(0x80,20),(0xc0,12),(0x100,64),(0x140,64),(0x180,56),(0x1c0,40),(0x200,72)]:zero(offset,size)
put(0x40,base+0x100,1,nodename,base+0xc0,base+0x80);put(0x80,base+0x140,0,0,0,0)
put(0xc0,0,base+0x80,base+0x140)
put(0x108,0x2000);put(0x130,base+0x40,base+0x180,base+0x1c0,0)
put(0x170,base+0x80,base+0x180,base+0x1c0,0)
put(0x184,modulepath);put(0x1c0,groupname);put(0x1dc,base+0x200);put(0x200,sectionname)
(case/'inputs/graph.bin').write_bytes(data)
meta=dict(status='Independent whole-body/layout/xref proof prepared before original execution',original_sha256=hashlib.sha256(p.data).hexdigest(),functions=proof,original_global_data=dict(usage_label_pointers=[dict(va=hex(a),pointer=hex(struct.unpack('<I',p.read(a,4))[0]),value=p.string(struct.unpack('<I',p.read(a,4))[0])) for a in [0x7a050,0x7a054]],debug_log=dict(va='0x7f07c',initial=struct.unpack('<I',p.read(0x7f07c,4))[0]),debug_flags=dict(va='0x5c06c',count=100,initial='loader-zeroed .bss; actual startup snapshot mandatory',consumed_indices=[1,2])),input=dict(path='inputs/graph.bin',bytes=4096,sha256=hashlib.sha256(data).hexdigest(),requested_base=base,root=base+0x40,allocation='VirtualAllocEx exact requested base; native MAP_FIXED_NOREPLACE, fail rather than replace if unavailable',layout=dict(node_size=20,edge_size=12,contribution_size=64,module_size=56,group_size=40,section_size=72),owned_offsets=dict(nodes=[0x40,0x80],edge=0xc0,contributions=[0x100,0x140],module=0x180,group=0x1c0,section=0x200,strings=[0x300,0x340,0x380,0x3c0]),guards='Entire4096 bytes unchanged, including all A5 gap/guard bytes'),scope='Explicit external direct test only, not CLI reach. Address bytes are controlled caller input. No global mode seed or output normalization. Allbyte rel32 scan independent of linear decoding; lone17540 absolute DWORD at file2945c is displacement bytes of unrelated39a5b call50fa0, not a pointer.',unvisited='debug_printf log-file and suppressed-output routes; open_debug_log3a900 remains a named trapping stub until actually reached. Source must preserve undefined suppressed-result status, not invent zero.',next='Compile/check external structure offsets; twice original baseline; fresh mask/native acceptance in actual order')
(case/'analysis/proof.json').write_text(json.dumps(meta,indent=2)+'\n')
