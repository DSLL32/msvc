"""Encountered byte-indexed path fold table, outside initialized snapshot."""
import struct
RVA=0x55928
OFFSET=1860
COUNT=256
SIZE=512
BSS_SIZE=2376

def field_rows():
 return [dict(name='path_fold',original_rva=RVA,bytes=SIZE,native_field='c1_bss.path_fold',native_offset=OFFSET,default=[0]*COUNT,c_type='WORD[256]',snapshot=False,default_proof='complete own canonical pre-entry BSS; initializer12f3e fills every WORD; path consumer1344d indexes unsigned byte and testsffff')]

def read(raw):return list(struct.unpack_from('<256H',raw,RVA-0x50000))
def seed(which):return [((0x8101 if which=='reset'else 0x4303)+37*i)&65535 for i in range(COUNT)]

def expected_fields(fields):
 rows=[r for r in fields['modeled_bss']if r['name']=='path_fold'];assert rows==field_rows()
 assert fields['path_fold']==dict(original_rva=RVA,bytes=SIZE,native_offset=OFFSET,native_bss_size=BSS_SIZE,initializer_chunks=[[0x12f3e,0x12f87],[0x307d2,0x307dc]],consumer_rva=0x1344d,snapshot=False)
 return {'path_fold':(RVA,SIZE,OFFSET)}
