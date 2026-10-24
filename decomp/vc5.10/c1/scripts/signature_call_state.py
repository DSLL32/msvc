"""Ordinary-case authenticated public CRC call; support receives no app credit."""
import copy,json,re,struct
from allocation_imports import pe_read,export_rva
from allocation_state import symbols,call
from provenance import captured_native_binary,sha
from scaler_state import semantic

PROVIDER='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'

def fp(raw):
 assert len(raw)==112
 cw,sw,tag=[v&65535 for v in struct.unpack_from('<3I',raw)];top=(sw>>11)&7
 return dict(cw=cw,sw=sw,tag=tag,top=top,live=[dict(logical=i,tag=(tag>>(2*((top+i)&7)))&3,raw80=raw[28+10*i:38+10*i].hex())for i in range(8)if((tag>>(2*((top+i)&7)))&3)!=3])

def validate(original,native,entry,end,next_entry):
 receipt=json.loads((original/'receipt.json').read_text());assert receipt['kind']=='explicit-original-capture'and receipt['probe']=='normal'and not receipt.get('signature_reference_plan_sha256')
 provider=original/'loaded-mspdb50.bin';assert sha(provider)==PROVIDER and provider.stat().st_size==167424 and export_rva(provider,'SigForPbCb')==0xf22d
 events=(original/'events.txt').read_text();assert re.findall(r'^signature-call phase=(\w+) ',events,re.M)==['entry','return']and'signature-reference-plan 'not in events
 assert events.count('option-match-plan ordinary-candidate=1 signature-continue=1 action-frontier=000131aa\n')==1
 a,b=[list(struct.unpack('<16I',(original/('signature-call-'+phase+'-registers.bin')).read_bytes()))for phase in ('entry','return')]
 pbase=int(re.findall(r'^module name=mspdb50.dll actual=(\w+) ',events,re.M)[0],16)
 assert a[1]==pbase+0xf22d and b[1]==0x10612c31 and b[2]==a[2]+4 and a[6:10]==b[6:10]
 assert a[10:12]==b[10:12]==[0x10612c31,pbase]and a[12:15]==[0x1066318c,1,0xffffffff]
 assert b[12:15]==[0x1066318c,1,b[3]]and b[3]==0xc2280058
 body=bytearray(pe_read(provider,0xf22d,53));assert body[-1]==0xc3 and struct.unpack_from('<I',body,34)[0]==0x10321a30;struct.pack_into('<I',body,34,pbase+0x21a30)
 assert(original/'signature-provider-body-000.bin').read_bytes()==bytes(body)
 table=pe_read(provider,0x21a30,1024);assert(original/'signature-provider-table-000.bin').read_bytes()==(original/'signature-provider-table-after-000.bin').read_bytes()==table
 for phase in ('entry','return'):assert(original/('signature-call-'+phase+'-input-000.bin')).read_bytes()==b'C\0'
 assert pe_read(original/'loaded-target.bin',0x6318c,2)==b'C\0'
 for suffix in ('words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv','target-data','target-BSS','target-snapshot'):
  assert(original/('locale-signature-entry-'+suffix+'.bin')).read_bytes()==(original/('locale-signature-return-'+suffix+'.bin')).read_bytes()
 for region in ('state','compiler-data','compiler-bss','compiler-snapshot'):assert(original/('%s-%03d.bin'%(region,a[0]))).read_bytes()==(original/('%s-%03d.bin'%(region,b[0]))).read_bytes()
 assert(original/'signature-call-entry-vector-000.bin').read_bytes()==(original/'signature-call-return-vector-000.bin').read_bytes()and len((original/'signature-call-entry-vector-000.bin').read_bytes())==32768
 binary=captured_native_binary(native);names=symbols(binary)
 assert entry['symbol']=='recon_mspdb_SigForPbCb'and entry['pc']==names['recon_mspdb_SigForPbCb'][0]
 assert entry['formal_inventory']==[dict(name=k,type=t)for k,t in [('bytes','const void *'),('byte_count','long'),('signature','unsigned long')]]
 assert entry['args']==dict(bytes=entry['words'][1],byte_count=1,signature=0xffffffff)and entry['input']==[67,0]
 assert entry['words']==[entry['caller']['pc'],entry['args']['bytes'],1,0xffffffff]and entry['caller']['symbol']=='parse_command_line';call(binary,entry['words'][0],entry['pc'])
 assert end['pc']==entry['words'][0]and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']and end['eax']==b[3]
 assert end['argument_slots_after']==[entry['args']['bytes'],1,b[3]],'actual native byvalue slot evolution, never guessed unchanged'
 assert entry['compiler_state']==end['compiler_state']and entry['invocation']==end['invocation']and entry['crt_state']==end['crt_state']and entry['errors']==end['errors']==[0,0,0]
 assert semantic(entry['fp'])==semantic(end['fp'])==fp((original/'locale-signature-entry-fp.bin').read_bytes())
 left,right=entry['locale'],end['locale'];assert {k:v for k,v in left.items()if k not in ('phase','fp')}=={k:v for k,v in right.items()if k not in ('phase','fp')}
 for suffix in ('mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv'):
  assert(native/('locale-native-signature-entry-'+suffix+'.bin')).read_bytes()==(native/('locale-native-signature-return-'+suffix+'.bin')).read_bytes()
 assert(native/'signature-call-entry-vector.bin').read_bytes()==(native/'signature-call-return-vector.bin').read_bytes()and len((native/'signature-call-entry-vector.bin').read_bytes())==32768
 expected=copy.deepcopy(end['compiler_state']);assert expected['bss']['pdb_signature']==0xffffffff;expected['bss']['pdb_signature']=b[3];assert next_entry['compiler_state']==expected
 seq=int(re.findall(r'^trap seq=(\d+) rva=00012f9c .*stack=10612c45,',events,re.M)[0]);raw=bytearray((original/('compiler-bss-%03d.bin'%b[0])).read_bytes());struct.pack_into('<I',raw,0x4de0,b[3]);assert(original/('compiler-bss-%03d.bin'%seq)).read_bytes()==bytes(raw)
 text=(original/'relocated-text.bin').read_bytes();assert text[0x12c2b-0x1000:0x12c39-0x1000]==bytes.fromhex('ff15d4d1651083c40ca3e04d6510')
 return dict(provider_sha256=PROVIDER,input='C',count=1,seed=0xffffffff,result=b[3],public_call_count=1,ordinary_case=True,third_argument_slots='actually modified to result on both sides; caller discards12 before any read/escape; no all-stack-unchanged claim',errors_preserved=True,full_original_FP_unchanged=True,architectural_native_FP_unchanged=True,defined_owned_vector_bytes_preserved=32768,application_credit=0)
