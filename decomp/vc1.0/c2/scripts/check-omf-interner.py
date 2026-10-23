"""Actual interner inputs, typed data and whole hash/chain/allocation controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_intern_name@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='name',location='ECX',type='char pointer'),dict(name='index',location='EDX',type='unsigned32 output pointer')]
 a=e['original'];b=e['native'];assert e['formals'][0]['original']==a['pointer']and e['formals'][0]['native']==b['pointer']and a['pointer']and b['pointer']and e['formals'][1]['original']==a['output']==a['entry_sp']+8 and e['formals'][1]['native']==b['output']==b['output_owner']and b['output_bytes']==4 and b['caller']=='omf_name_index'
 assert a['name_hex']==b['name_hex']=='00'and a['buckets']==b['buckets']==[0]*16 and a['next_index']==b['next_index']==1
 assert e['output_contract']==dict(bytes=4,direction='write-only',prior_bytes='uninitialized, not input, not observed or seeded',original_owner='omf_name_index local at interner entryESP+8',native_owner='&omf_name_index.index',writes_before_each_return=['0x6c5e7','0x6c5f3'])
validate(e);rejected=[]
for label,change in [('missing output',lambda e:e['formals'].pop()),('wrong output register',lambda e:e['formals'][1].update(location='entryESP+4')),('unknown hidden input',lambda e:e.update(hidden_inputs=['?'])),('wrong native owner',lambda e:e['native'].update(output_owner=0)),('wrong original owner',lambda e:e['original'].update(entry_sp=0)),('wrong output width',lambda e:e['output_contract'].update(bytes=8)),('invented seed',lambda e:e['output_contract'].update(prior_bytes='zero')),('wrong bucket state',lambda e:e['native']['buckets'].__setitem__(0,1)),('wrong next index',lambda e:e['native'].update(next_index=0)),('missing string',lambda e:e['native'].update(name_hex=''))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c540 and row['end']==0x6c5ff and original.read(0x6c5fe,1)==b'\xc3';assert struct.unpack('<I',original.read(0x85084,4))[0]==struct.unpack('<I',canonical.read(names['_omf_next_name'],4))[0]==1
assert next(s for s in original.sections if original.base+s['rva']<=0x7b890<original.base+s['rva']+s['virtual_size'])['flags']&0x80
if args.abi_only:print('OMF interner actual output ownership/10 rejects,191-byte extent,typed BSS/counter pass');raise SystemExit
MASK=0xffffffff

def h(text):return sum((x if x<128 else x-256)for x in text[:5])&15

def run(image,native,text,chain,next_index,mutate):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 table=names['_omf_name_buckets']if native else 0x7b890;counter=names['_omf_next_name']if native else 0x85084;input=0x400000;out=0x404000;fresh=0x400800;copy_string=0x403000;inserted=0x400600;u.mem_write(input,text+b'\0');u.mem_write(out-4,b'\x5a'*12);u.mem_write(fresh,b'\xa5'*12);nodes=[]
 for i,old in enumerate(chain):
  address=0x400400+12*i;pointer=0x402000+256*i;index=[17,0x80000000,0xffffffff][i];link=address+12 if i+1<len(chain)else 0;node=struct.pack('<3I',pointer,index,link);u.mem_write(address,node);u.mem_write(pointer,old+b'\0');nodes.append((address,node))
 bucket=h(text);head=nodes[0][0]if nodes else 0;before=[0]*16;before[bucket]=head;u.mem_write(table,struct.pack('<16I',*before));u.mem_write(counter,struct.pack('<I',next_index));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,input);u.reg_write(UC_X86_REG_EDX,out);allocate=names['@arena_alloc@8']if native else 0x70ae0;duplicate=names['@arena_strdup@8']if native else 0x70c40;events=[]
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in[allocate,duplicate]:return
  ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP)
  assert bytes(m.mem_read(out-4,12))==b'\x5a'*12
  if a==allocate:assert ecx==7 and edx==12;events.append(['allocate',7,12]);result=fresh
  else:
   assert ecx==input and edx==7 and bytes(m.mem_read(fresh,12))==b'\xa5'*12;events.append(['duplicate',text.hex(),7]);m.mem_write(copy_string,text+b'\0');result=copy_string
   if mutate:
    m.mem_write(counter,struct.pack('<I',(next_index+17)&MASK));m.mem_write(inserted,struct.pack('<3I',0x403200,29,head));m.mem_write(table+4*bucket,struct.pack('<I',inserted))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=20000);assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 match=next((i for i,old in enumerate(chain)if old==text),None)
 if match is None:
  expected=(next_index+17)&MASK if mutate else next_index;counter_after=(expected+1)&MASK;before[bucket]=fresh;assert events==[['allocate',7,12],['duplicate',text.hex(),7]]and bytes(u.mem_read(fresh,12))==struct.pack('<3I',copy_string,expected,inserted if mutate else head)and bytes(u.mem_read(copy_string,len(text)+1))==text+b'\0'
 else:expected=[17,0x80000000,0xffffffff][match];counter_after=next_index;assert events==[]and bytes(u.mem_read(fresh,12))==b'\xa5'*12
 assert u.reg_read(UC_X86_REG_EAX)==int(match is not None)and bytes(u.mem_read(out-4,12))==b'\x5a'*4+struct.pack('<I',expected)+b'\x5a'*4 and bytes(u.mem_read(table,64))==struct.pack('<16I',*before)and struct.unpack('<I',u.mem_read(counter,4))[0]==counter_after and bytes(u.mem_read(input,len(text)+1))==text+b'\0'and all(bytes(u.mem_read(a,12))==v for a,v in nodes)and all(bytes(u.mem_read(0x402000+256*i,len(old)+1))==old+b'\0'for i,old in enumerate(chain))
 return dict(found=match is not None,index=expected,counter=counter_after,events=events,selected_bucket=bucket)

def collider(text,delta):
 if not text:return bytes([delta])
 first=(text[0]+delta)&255
 if not first:first=(first+16)&255
 return bytes([first])+text[1:]
rows=[]
for text,mode,index,mutate in itertools.product([b'',b'A',b'_TEXT',b'abcdeLONG',b'\x80xy',b'\xffabcde'],range(7),[1,0x80000000,0xffffffff],[False,True]):
 a=collider(text,16);b=collider(text,32);chains=[[],[a,b],[text,a,b],[a,text,b],[a,b,text],[text+b'\x10',a],[text[:5],a]];chain=chains[mode];assert all(h(t)==h(text)for t in chain)
 l=run(original,False,text,chain,index,mutate);r=run(canonical,True,text,chain,index,mutate);assert l==r;rows.append(dict(name_hex=text.hex(),chain_hex=[s.hex()for s in chain],next_index=index,duplicate_mutates_bucket_counter=mutate,result=l))
# Independently visit every bucket, beyond the selected high-byte/name shapes.
for b in range(16):
 text=bytes([0x40+b]);l=run(original,False,text,[],1,False);r=run(canonical,True,text,[],1,False);assert l==r and l['selected_bucket']==b;rows.append(dict(each_bucket=b,result=l))
Path('cases/omf-interner-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole191-byte original/canonical interner, all16 hash buckets/first5 signed bytes, empty/high-byte/prefix/odd/even strings, no-match and head/middle/tail chains, exact12-byte arena7 node+string allocation order, index wrap and post-child bucket/counter reload, output4 bytes+guards always overwritten and every old node/string unchanged. Allocation helpers mocked with explicit mutation for ordering; actual first empty-name/output caller owner/16null buckets/next1 observed. Initial output bytes remain unknown write-only input, never seeded in reconstruction.'),indent=2)+'\n');print(len(rows),'whole OMF interner controls and10 actual output-ABI rejects pass')
