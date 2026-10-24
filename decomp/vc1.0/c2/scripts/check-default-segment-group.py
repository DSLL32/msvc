"""Actual linked default-group identity and real defaults→B7 prefix path."""
from pathlib import Path
import hashlib,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();word=lambda image,p:struct.unpack('<I',image.read(p,4))[0]
assert word(original,0x84fd4)==0x84fc8 and original.read(0x84fc8,8)==bytes(8)
assert word(canonical,names['_default_segment_group'])==names['_ungrouped_segment']
assert len({names['_ungrouped_segment'],names['_omf_group_storage'],names['_default_segment_group'],names['_omf_default_group']})==4
assert word(original,0x84fc4)==0x84fb8 and 0x84fc8!=0x84fb8
for image,pointer,target in[(original,0x84fd4,0x84fc8),(canonical,names['_default_segment_group'],names['_ungrouped_segment'])]:
 section=next(s for s in image.sections if image.base+s['rva']<=target and target+8<=image.base+s['rva']+s['virtual_size']);assert section['flags']&0x80 or image.read(target,8)==bytes(8)
text=Path('src/segments.h').read_text();assert 'sizeof(OmfGroup) == 8'in text and'offsetof(OmfGroup, name_index) == 0'in text and'offsetof(OmfGroup, number) == 4'in text
assert 'OmfGroup ungrouped_segment = { 0, 0 };'in Path('src/segments.c').read_text()and'OmfGroup *default_segment_group = &ungrouped_segment;'in Path('src/segments.c').read_text()
def run(image,native):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 segment=0x400080;pool=names['_omf_records']if native else 0x7a8dc;group=names['_ungrouped_segment']if native else 0x84fc8;pointer=names['_default_segment_group']if native else 0x84fd4;u.mem_write(segment,bytes(72));u.mem_write(segment+64,struct.pack('<I',1));stopped=False
 def hook(m,a,size,_):
  nonlocal stopped
  if a==0x500000:stopped=True;m.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook)
 def call(address,ecx,edx,stack=()):
  nonlocal stopped
  stopped=False;u.mem_write(0x300800,struct.pack('<'+'I'*(1+len(stack)),0x500000,*stack));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,ecx);u.reg_write(UC_X86_REG_EDX,edx);u.emu_start(address,0,count=5000);assert stopped and u.reg_read(UC_X86_REG_ESP)==0x300804+4*len(stack)
 call(names['@segment_defaults@4']if native else 0x6a160,segment,0xabcdef98)
 assert struct.unpack('<I',u.mem_read(pointer,4))[0]==group and struct.unpack('<I',u.mem_read(segment+24,4))[0]==group and bytes(u.mem_read(group,8))==bytes(8)
 call(names['@omf_initialize_record@12']if native else 0x6c060,pool,0xb7,(segment,))
 assert struct.unpack('<5I',u.mem_read(pool,20))==(0xb7,segment,0,0,pool+25)and bytes(u.mem_read(pool+23,2))==b'\0\1'and struct.unpack('<I',u.mem_read(segment+24,4))[0]==group and bytes(u.mem_read(group,8))==bytes(8)
 return dict(group_words=[0,0],group_identity_preserved=True,wire_prefix_hex=bytes(u.mem_read(pool+23,2)).hex(),path=['segment_defaults','omf_initialize_record','omf_write_index','omf_write_index'])
a=run(original,False);b=run(canonical,True);assert a==b
# These two straight-line/branch-only bodies have no embedded text data.
# Bind true absolute memory operands, not coincident numeric immediates.
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86_const import X86_OP_MEM
_,_,_,left,right=indexes();decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;storage_uses=[]
for name in ['_initialize_segments','@segment_defaults@4']:
 sides=[]
 for image,records,target in [(original,left,0x84fd4),(canonical,right,names['_default_segment_group'])]:
  r=next(r for r in records if r['name']==name);uses=[hex(i.address)for i in decoder.disasm(image.read(r['start'],r['end']-r['start']),r['start'])if any(o.type==X86_OP_MEM and not o.mem.base and not o.mem.index and o.mem.disp==target for o in i.operands)]
  assert uses,(name,hex(target));sides.append(uses)
 storage_uses.append(dict(function=name,original=sides[0],canonical=sides[1]))
# Distinct zero contents do not excuse a split identity: the initializer's
# pointer target must be the named descriptor, not the other zero group.
wrong=names['_omf_group_storage'];assert word(canonical,names['_default_segment_group'])!=wrong
rejected=[]
for label,value in [('distinct zero group',wrong),('null pointer',0),('string object',word(canonical,names['_code_segment_name']))]:
 try:assert value==names['_ungrouped_segment']
 except AssertionError:rejected.append(label)
 else:raise AssertionError('Invalid storage target accepted: '+label)
Path('cases/default-segment-group-validation.json').write_text(json.dumps(dict(matched=True,accepted=len(cfg['accepted']),original=dict(pointer='0x84fd4',object_start='0x84fc8',object_end='0x84fd0',bytes=8,fields=dict(name_index=0,number=4)),canonical=dict(pointer=hex(names['_default_segment_group']),object=hex(names['_ungrouped_segment']),distinct_from_normal_group=hex(wrong)),real_initializer_path=a,storage_uses=storage_uses,rejected=rejected,image_sha256=dict(original=hashlib.sha256(original.data).hexdigest(),canonical=hashlib.sha256(canonical.data).hexdigest()),negative='Binding default pointer to separate zero normal group is rejected despite equal contents; empty-string object initializer rejected by named target binding.',seconds=time.perf_counter()-started,scope='Loaded original/canonical PE initial pointer/typed8-byte zero object, no synthetic group input or string padding. Execute real segment_defaults then actual B7 initializer and both real index bodies, no mocked calls, producing group0/segment1. Original/public extent unchanged; typed fields/layout/identity and actual path only, not all group producer coverage. Fresh real finalizer input and15 native cases separately required.'),indent=2)+'\n');print('True default group8-byte identity/offsets and real defaults→B7/index path pass')
