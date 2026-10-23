"""One positively identified older decimal CRT/library binding, no CRT source.

The exact library matcher stays exact. This sole variant preserves the public
ABI and logical callees, and has an explicitly bounded actual conversion proof.
"""
import hashlib,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86_const import X86_OP_IMM,X86_OP_MEM
from binary import PE,COFF,archive,match_section
BINARY=Path(__file__).resolve().parents[1]
TC=BINARY.parents[2]/'toolchains/1.0-nt'
NAME='___strgtold12'
CALLEES=['___mtold12','___multtenpow12','__isctype']
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def default_record():
 return json.loads((BINARY/'runtime-compatible.json').read_text())['record']
def verify(record=None,original=None,proof_path=None):
 record=default_record() if record is None else record
 assert record['name']==NAME and (record['start'],record['end'])==(0x88520,0x88c40),'wrong compatible decimal extent/name'
 assert record['library']=='LIBC.LIB' and record['object']=='obj\\i386\\strgtold.obj'
 path=BINARY/record['compatible_proof'] if proof_path is None else Path(proof_path)
 assert sha(path)==record['compatible_proof_sha256'],'stale compatible decimal proof'
 proof=json.loads(path.read_text());base=path.parent
 assert proof['name']==NAME and proof['same_distinct_callees']==CALLEES,'wrong compatible decimal callee graph'
 assert proof['abi']==dict(convention='cdecl',stack_words=7,result_bytes=12,end_pointer_output_bytes=4,return_bits=32,hidden_register_inputs=[]),'wrong compatible decimal ABI'
 assert proof['source_credit']==proof['application_credit']==0
 for name,digest in proof['verification_scripts'].items():assert sha(BINARY/name)==digest, 'compatible decimal verifier/observer changed'
 for name,digest in proof['evidence_files'].items():
  evidence=base/name;assert evidence.resolve().is_relative_to(base.resolve())
  assert sha(evidence)==digest,'compatible decimal evidence changed: '+name
 assert sha(BINARY/proof['fixture_manifest']['path'])==proof['fixture_manifest']['sha256'],'frozen decimal case inputs changed'
 old=PE(TC/'bin/C13232.EXE');current=PE(TC/'bin/C1XX3232.EXE') if original is None else original
 assert hashlib.sha256(old.data).hexdigest()==proof['c1_target_sha256']
 assert hashlib.sha256(current.data).hexdigest()==proof['target_sha256']
 assert hashlib.sha256(current.read(0x88520,0x720)).hexdigest()==proof['original_body_sha256'],'wrong compatible original decimal body'
 library=TC/'lib/LIBC.LIB';assert sha(library)==proof['library_sha256']
 members=dict(archive(library));assert hashlib.sha256(members[record['object']]).hexdigest()==proof['member_sha256']
 source=json.loads((base/'old-variant-comparison.json').read_text());pairs=source['owned_callee_and_global_anchor_records'];objects={}
 def member(name):
  if name not in objects:objects[name]=COFF(members[name])
  return objects[name]
 def fragment(row):
  obj=member(row['object']);sym=next(s for s in obj.symbols.values() if s['name']==row['name'] and s['section']>0)
  section=obj.sections[sym['section']-1];begin=sym['value'];size=row['end']-row['start']
  return dict(section,data=section['data'][begin:begin+size],relocs=[(off-begin,t,k) for off,t,k in section['relocs'] if begin<=off<begin+size])
 for image,rows in zip([old,current],pairs):
  assert {r['name'] for r in rows}==set(CALLEES+['_toupper'])
  for row in rows:assert match_section(image.read(row['start'],row['end']-row['start']),fragment(row))==[0],'decimal callee/global anchor is not exact shipped body'
 for index,(symbol,expected) in enumerate([('__pctype',[0x735d0,0xaa650]),('___mb_cur_max',[0x737dc,0xaa85c])]):
  for image,rows,anchor,address in zip([old,current],pairs,source['crt_global_anchors'][index],expected):
   row=next(r for r in rows if r['name']==anchor['via_function']);part=fragment(row)
   off=anchor['relocation_offset'];target=next(t for n,t,k in part['relocs'] if n==off and k==6)
   assert target['name']==symbol
   actual=(struct.unpack('<I',image.read(row['start']+off,4))[0]-struct.unpack_from('<I',part['data'],off)[0])&0xffffffff
   assert actual==anchor['address']==address
 nls=member('misc\\obj\\i386\\NLSDATA1.OBJ');data=next(s for s in nls.sections if s['name']=='.data')
 for image,address in zip([old,current],[0x737dc,0xaa85c]):
  assert match_section(image.read(address,16),data)==[0]
  assert struct.unpack('<I',image.read(address+8,4))[0]==address+4 and image.string(address+4)=='.'
 address_map={0x9af20:0x68b00,0x9af37:0x68b17,0x88c10:0x58630,0xaa864:0x737e4,0xaa85c:0x737dc,0xaa650:0x735d0}
 cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
 code=[old.read(0x57f40,0x6f0),current.read(0x88520,0x6f0)];instructions=[list(cs.disasm(raw,start)) for raw,start in zip(code,[0x57f40,0x88520])]
 assert len(instructions[0])==len(instructions[1])==513
 normalized=bytearray(code[1]);calls=[]
 for a,b in zip(*instructions):
  offset=a.address-0x57f40;assert offset==b.address-0x88520 and a.id==b.id and a.size==b.size
  if a.mnemonic=='call':
   names=[next(r['name'] for r in rows if r['start']==ins.operands[0].imm) for ins,rows in zip([a,b],pairs)]
   assert names[0]==names[1];calls.append(dict(offset=offset,name=names[0]))
   at=offset+b.imm_offset;normalized[at:at+4]=code[0][at:at+4]
  for aa,bb in zip(a.operands,b.operands):
   if aa.type==bb.type==X86_OP_MEM and aa.mem.disp!=bb.mem.disp:
    assert address_map.get(bb.mem.disp)==aa.mem.disp and b.disp_size==4
    at=offset+b.disp_offset;normalized[at:at+4]=code[0][at:at+4]
   if aa.type==bb.type==X86_OP_IMM and aa.imm!=bb.imm and a.mnemonic!='call' and a.bytes!=b.bytes:
    assert address_map.get(bb.imm)==aa.imm and b.imm_size==4
    at=offset+b.imm_offset;normalized[at:at+4]=code[0][at:at+4]
 assert code[0]==normalized,'unexplained old decimal variant instruction change'
 assert hashlib.sha256(normalized).hexdigest()==source['normalized_code_sha256']
 assert calls==[dict(offset=r['offset'],name=r['name']) for r in source['calls']]
 targets=[list(struct.unpack('<12I',image.read(start+0x6f0,48))) for image,start in zip([old,current],[0x57f40,0x88520])]
 assert [x-0x57f40 for x in targets[0]]==[x-0x88520 for x in targets[1]]==source['dispatch_relative_targets']
 for image,address in zip([old,current],[0x68b00,0x9af20]):
  section=next(s for s in image.sections if image.base+s['rva']<=address<image.base+s['rva']+s['virtual_size'])
  assert section['flags']&0x80 and address+25<=image.base+section['rva']+section['virtual_size']
 section=next(s for s in current.sections if s['name']=='.text');raw=current.data[section['off']:section['off']+section['virtual_size']];workspace=[]
 for address in range(0x9af20,0x9af20+25):
  position=0
  while (position:=raw.find(address.to_bytes(4,'little'),position))>=0:
   absolute=current.base+section['rva']+position;assert 0x88520<=absolute<0x88c40,'decimal workspace has an external address consumer'
   workspace.append(absolute);position+=1
 assert workspace==[0x8852b,0x88aff,0x88ab7,0x88ac0]
 shipped=member(record['object']);text=next(s for s in shipped.sections if s['name']=='.text');symbol=next(s for s in shipped.symbols.values() if s['name']==NAME)
 assert symbol['value']==0
 shipped_calls=[t['name'] for off,t,k in text['relocs'] if k==20 and off<0x750]
 assert sorted(set(shipped_calls))==sorted(set(c['name'] for c in calls))==CALLEES and len(shipped_calls)==len(calls)==7
 parsed=list(cs.disasm(text['data'][:0x720],0));assert len(parsed)==517 and [i.op_str for i in parsed if i.mnemonic=='ret']==['']
 assert parsed[0].bytes==b'\x83\xec\x60' and instructions[0][0].bytes==b'\x83\xec\x44'
 # Actual public boundary: only the ten defined bytes are observations.
 observed=json.loads((base/'public-contract/receipt.json').read_text());left,right=observed['original'],observed['native'];contract=proof['public_contract']
 assert left['return_va']==0x74b57 and left['text_address']==0x8c158 and left['output_address']==left['entry_esp']+44
 assert right['caller']=='convert_floating_literal' and right['output_matches_caller_parsed'] and right['defined_output_bytes']==10
 assert all(left[k]==right[k] for k in ['text','after_text','output','result'])
 assert (left['text'],left['after_text'],left['output'],left['result'])==(contract['input_hex'],contract['after_input_hex'],contract['output_hex'],contract['status'])
 native=json.loads((base/'public-contract/native-observation.json').read_text());assert native==right
 raw=(base/'public-contract/original-observation.txt').read_text()
 for name,value in [('text',left['text']),('after_text',left['after_text']),('output',left['output']),('result',str(left['result']))]:assert '\n'+name+' '+value+'\n' in '\n'+raw
 probe=json.loads((base/'shipped-probe/receipt.json').read_text())
 assert (probe['input'],probe['after_input'],probe['output10'],probe['result'])==(left['text'],left['after_text'],left['output'],left['result'])
 assert (base/'shipped-probe/stdout.bin').read_bytes().replace(b'\r\n',b'\n')==b'result 0\noutput 00000000000000c0ff3f\nafter_input 312e3500\n'
 return dict(name=NAME,record=record,identity='positively identified older CRT implementation, compatible shipped-library graph/ABI and bounded actual public conversion; not exact library bytes',proof_sha256=sha(path),original_body_sha256=proof['original_body_sha256'],library_sha256=proof['library_sha256'],object_sha256=proof['member_sha256'],call_targets=CALLEES,source_credit=0,application_credit=0)
def original_records(path):
 if Path(path).resolve()!=(TC/'bin/C1XX3232.EXE').resolve():return []
 row=default_record();verify(row);return [row]
if __name__=='__main__':
 result=verify();print(json.dumps(result,indent=2))
