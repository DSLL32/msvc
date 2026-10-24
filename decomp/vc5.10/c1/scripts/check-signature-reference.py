#!/usr/bin/env python3
"""Read-only authentication of one original-only public dependency response."""
import argparse,hashlib,json,re,struct,time
from pathlib import Path
from provenance import sha
from allocation_imports import pe_read,export_rva

PLAN='e2555a9047c4793567e6cc69486d4f7f577b3c489f4480ef2a77c55a697c2334'
PROVIDER='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
TARGET='7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d'
CRT='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'

def check(directory):
 started=time.perf_counter();directory=directory.resolve();r=json.loads((directory/'receipt.json').read_text())
 assert(r['kind'],r['mode'],r['probe'],r['exit'])==('original-dependency-reference','masked','normal',125)
 assert r['signature_reference_plan_sha256']==PLAN==sha(directory/'signature-reference-plan.json')
 for name,row in r['artifacts'].items():
  p=directory/name;assert Path(name).name==name and p.is_file()and p.stat().st_size==row['size']and sha(p)==row['sha256'],'changed artifact '+name
 assert sha(directory/'loaded-target.bin')==r['target_sha256']==TARGET and sha(directory/'loaded-msvcrt.bin')==r['canonical_crt_sha256']==CRT
 provider=directory/'loaded-mspdb50.bin';assert sha(provider)==PROVIDER and provider.stat().st_size==167424 and export_rva(provider,'SigForPbCb')==0xf22d
 config=json.loads((directory/'functions.json').read_text());candidate=json.loads((directory/'candidate.json').read_text())
 assert len([row for row in config['driver']if row.get('role')=='application'])==73 and'_next_argument'not in config['accepted']
 assert candidate['functions'][-1]['name']=='_new_option_vector'
 ranges=[(int(a,16),int(b,16),int(e))for a,b,e in [line.split()for line in(directory/'ranges.txt').read_text().splitlines()]]
 assert ranges==[(x['start_rva'],x['end_rva'],x['entry'])for x in candidate['ranges']]
 assert not any(a<=0x12f9c<b for a,b,e in ranges)
 events=(directory/'events.txt').read_text();assert'failure 'not in events
 assert events.count('signature-reference-plan accepted73=1 next_argument-masked=1 provider-execution-analysis-only=1\n')==1
 assert re.findall(r'^frontier rva=(\w+)$',events,re.M)==['00012f9c']and'dependency-frontier 'not in events
 assert re.findall(r'^signature-reference phase=(\w+) ',events,re.M)==['entry','return']
 a,b=[list(struct.unpack('<16I',(directory/('signature-reference-'+phase+'-registers.bin')).read_bytes()))for phase in ('entry','return')]
 module=int(re.findall(r'^module name=c1n32_l.dll actual=(\w+) ',events,re.M)[0],16);pbase=int(re.findall(r'^module name=mspdb50.dll actual=(\w+) ',events,re.M)[0],16)
 assert module==0x10600000 and a[1]==pbase+0xf22d and b[1]==module+0x12c31 and b[2]==a[2]+4
 assert a[10:12]==b[10:12]==[module+0x12c31,pbase]and a[12:]==[module+0x6318c,1,0xffffffff,a[15]]
 assert a[6:10]==b[6:10]and b[12:14]==a[12:14]and b[14]==b[3]and b[14]!=a[14]
 body=bytearray(pe_read(provider,0xf22d,53));assert len(body)==53 and body[-1]==0xc3 and struct.unpack_from('<I',body,34)[0]==0x10321a30
 struct.pack_into('<I',body,34,pbase+0x21a30)
 assert(directory/'signature-provider-body-000.bin').read_bytes()==bytes(body)
 table=pe_read(provider,0x21a30,1024)
 assert(directory/'signature-provider-table-000.bin').read_bytes()==(directory/'signature-provider-table-after-000.bin').read_bytes()==table
 for phase in ('entry','return'):
  assert(directory/('signature-reference-'+phase+'-input-000.bin')).read_bytes()==b'C\0'
 assert pe_read(directory/'loaded-target.bin',0x6318c,2)==b'C\0'
 assert(directory/'signature-reference-entry-vector-000.bin').read_bytes()==(directory/'signature-reference-return-vector-000.bin').read_bytes()and len((directory/'signature-reference-entry-vector-000.bin').read_bytes())==32768
 suffixes=['words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv','target-data','target-BSS','target-snapshot']
 for suffix in suffixes:assert(directory/('locale-signature-entry-'+suffix+'.bin')).read_bytes()==(directory/('locale-signature-return-'+suffix+'.bin')).read_bytes(),'changed observable '+suffix
 errors=list(struct.unpack('<3I',(directory/'locale-signature-return-errors.bin').read_bytes()));assert errors==[0,0,0]
 for region in ('state','compiler-data','compiler-bss','compiler-snapshot'):assert(directory/('%s-%03d.bin'%(region,a[0]))).read_bytes()==(directory/('%s-%03d.bin'%(region,b[0]))).read_bytes()
 last=re.findall(r'^trap seq=(\d+) rva=00012f9c esp=(\w+) eax=(\w+) ecx=(\w+) stack=([\w,]+)$',events,re.M);assert len(last)==1;seq=int(last[0][0]);words=[int(v,16)for v in last[0][4].split(',')]
 assert a[0]<b[0]<seq and int(last[0][2],16)==b[3]and words[0]==module+0x12c45
 before=bytearray((directory/('compiler-bss-%03d.bin'%b[0])).read_bytes());assert struct.unpack_from('<I',before,0x54de0-0x50000)[0]==0xffffffff;struct.pack_into('<I',before,0x54de0-0x50000,b[3]);assert bytes(before)==(directory/('compiler-bss-%03d.bin'%seq)).read_bytes()
 for region in ('state','compiler-data','compiler-snapshot'):assert(directory/('%s-%03d.bin'%(region,b[0]))).read_bytes()==(directory/('%s-%03d.bin'%(region,seq))).read_bytes()
 assert(directory/'locale-signature-return-fp.bin').read_bytes()==(directory/('fpu-%03d.bin'%seq)).read_bytes()
 text=(directory/'relocated-text.bin').read_bytes()
 assert text[0x12c2b-0x1000:0x12c39-0x1000]==bytes.fromhex('ff15d4d1651083c40ca3e04d6510')
 assert text[0x12c39-0x1000:0x12c45-0x1000]==bytes.fromhex('6a00689c2f6110e857030000')
 assert text[0x12f9c-0x1000]==0xa1
 return dict(status='PASS',kind='one original-only exact external dependency response; no app/provider acceptance or shared success',receipt_sha256=sha(directory/'receipt.json'),plan_sha256=PLAN,target_sha256=TARGET,provider_sha256=PROVIDER,provider_export=dict(name='SigForPbCb',ordinal=127,rva=0xf22d),input_hex='43',defined_literal_hex='4300',byte_count=1,input_signature=4294967295,return_value=b[3],return_hex='%08x'%b[3],stack_signature_before=a[14],stack_signature_after=b[14],private_stack_qualification='third incoming parameter slot modified; exact caller ADD12 discards all3 slots before any read/escape; consumes only EAX',nonvolatile_preserved=True,full_original_FP_unchanged=True,errors=errors,target_delta_after_caller=dict(rva=0x54de0,bytes=4,value=b[3]),next_named_trap='next_argument12f9c',source_helper_sha256=r['helper_sha256'],functions_sha256=r['functions_sha256'],seconds=time.perf_counter()-started)

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--output',type=Path);a=p.parse_args();result=check(a.directory);content=json.dumps(result,indent=2)+'\n'
 if a.output:assert not a.output.exists();a.output.write_text(content)
 print(content,end='')
