#!/usr/bin/env python3
"""Partition exact image extents; this is not code or CFG discovery."""
import argparse,json,struct
from collections import Counter
from pathlib import Path
from binary import PE,u32
from mask import BINARY,ROOT,sha
from layout import loader_data
from runtime import identify
p=argparse.ArgumentParser();p.add_argument('--audit',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
c=json.loads((BINARY/'functions.json').read_text());image=PE(ROOT/c['target']['path']);assert sha(image.data)==c['target']['sha256']
runtime=identify();section=next(s for s in image.sections if s['name']=='.text');base=image.base+section['rva'];end=base+section['virtual_size'];labels=[None]*(end-base)
def mark(start,stop,label):
 assert base<=start<stop<=end
 for address in range(start,stop):
  old=labels[address-base];assert old in (None,label),('classification overlap',address,old,label)
  labels[address-base]=label
for f in c['functions']:mark(f['start'],f['end'],'application-code')
for f in runtime['functions']:mark(f['start'],f['end'],'import-thunks' if f['name'].startswith('import:') else 'verified-crt-code')
for f in c['functions']:
 for start,stop in f.get('text_data',[]):mark(start,stop,'encountered-application-readonly-data')
for r in loader_data(image):mark(r['start'],r['end'],'parsed-loader-data')
for r in runtime['data']:mark(r['start'],r['end'],'verified-crt-scope-data')
# Three stale records immediately follow the one published PE debug directory.
# They are retained metadata, not an extra loader-restoration permission.
peoff=u32(image.data,60);timestamp=u32(image.data,peoff+8)
published=image.base+u32(image.data,image.optional+96+6*8);published_size=u32(image.data,image.optional+100+6*8)
assert (published,published_size)==(0x10010a0,28)
records=[]
for address,kind,size,pointer in [(0x10010a0,4,0x110,0x3a00),(0x10010bc,4,0x110,0x6be8),(0x10010d8,3,0x190,0x6cf8),(0x10010f4,2,0x1b,0xffbee5c0)]:
 fields=struct.unpack('<IIHHIIII',image.read(address,28));assert fields==(0,timestamp,0,0,kind,size,0,pointer)
 backed=pointer<=len(image.data) and size<=len(image.data)-pointer
 record=dict(start=address,end=address+28,type=kind,type_name={2:'CODEVIEW',3:'FPO',4:'MISC'}[kind],characteristics=0,timestamp=timestamp,major_version=0,minor_version=0,size_of_data=size,address_of_raw_data=0,pointer_to_raw_data=pointer,payload_end=pointer+size,payload_file_backed=backed,published=address==published,sha256=sha(image.read(address,28)))
 if backed:
  payload=image.data[pointer:pointer+size];record['payload_sha256']=sha(payload)
  assert struct.unpack_from('<II',payload)==(1,0x110)
  assert payload[12:27]==b'exe\\cvtres.dbg\0'
  record['misc_data_type']=1;record['misc_length']=0x110;record['misc_filename']='exe\\cvtres.dbg'
 else:record['payload_status']='stale/outside the 15120-byte file; not a valid loaded payload'
 records.append(record)
mark(0x10010bc,0x1001110,'orphaned-debug-records')
graph_path=a.audit/'graph.stdout';graph=json.loads(graph_path.read_text());assert graph['checked_functions']==22
references=[]
for name,item in graph['original_accepted_graphs'].items():
 for kind in ('calls','references'):
  for value in item[kind]:
   if 0x10010bc<=value<0x1001110:references.append(dict(function=name,kind=kind,address=value))
assert references==[],'application graph references orphaned metadata'
assert all(not 0x10010bc<=value<0x1001110 for value in runtime['bindings'].values()),'runtime relocation references orphaned metadata'
# Account only for still-unowned, exact uniform alignment runs. Unexpected
# bytes fail instead of being treated as code, data, or accepted functions.
for n,label in enumerate(labels):
 if label is None:
  value=image.read(base+n,1)[0];assert value in (0,0xcc),('unclassified byte',hex(base+n),value)
  labels[n]='zero-alignment' if value==0 else 'int3-alignment'
ranges=[]
for n,label in enumerate(labels):
 if n==0 or label!=labels[n-1]:ranges.append(dict(start=base+n,end=base+n+1,kind=label))
 else:ranges[-1]['end']+=1
for r in ranges:r.update(bytes=r['end']-r['start'],sha256=sha(image.read(r['start'],r['end']-r['start'])))
counts=dict(Counter(labels));assert sum(counts.values())==section['virtual_size'];assert counts['application-code']==7445
assert all(f['status']=='accepted' for f in c['functions']) and len(c['functions'])==22
raw_tail=image.read(end,section['size']-section['virtual_size']);assert raw_tail==b'\0'*34
report=dict(schema=1,target=c['target'],target_bytes=len(image.data),section=section,virtual_range=[base,end],categories=counts,ranges=ranges,unclassified_bytes=0,application_functions=22,accepted_application_extent_bytes=7445,crt_functions=4,crt_code_bytes=400,import_thunks=4,import_thunk_bytes=24,raw_alignment_tail=dict(start=end,end=base+section['size'],bytes=len(raw_tail),sha256=sha(raw_tail)),debug_records=records,debug_classification_evidence=dict(published_directory=[published,published+published_size],orphaned_range=[0x10010bc,0x1001110],layout='Three contiguous 28-byte IMAGE_DEBUG_DIRECTORY records share the published record timestamp/zero characteristics/version/RVA; known MISC/FPO/CODEVIEW types and stale payload file offsets. Published MISC points to bounded exe\\cvtres.dbg payload.',application_graph_references=references,graph_report=str(graph_path.resolve()),graph_sha256=sha(graph_path.read_bytes()),runtime_relocations_reference_orphans=False,mask_policy='Orphaned records remain INT3-masked; they are not loader-preservation or application acceptance ranges.',limits='Constant call/data references from all 22 audited application extents plus exact shipped-runtime relocation bindings. No claim that arbitrary computed pointers cannot address metadata.'),other_sections=[s for s in image.sections if s['name']!='.text'],qualification='All identified application extents are accepted; this partition is not branch/input/API/resource completeness. Only encountered data is reconstructed. Invalid bitmap depths and unexercised formats/error branches remain unvalidated.')
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(categories=counts,unclassified_bytes=0,application_functions=22),indent=2))
