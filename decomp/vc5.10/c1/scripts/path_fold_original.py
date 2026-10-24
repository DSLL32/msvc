"""Authenticate own actual _mbctolower scalar calls, no native success claim."""
import hashlib,json,re,struct,time
from pathlib import Path
from allocation_imports import pe_read,export_rva
from provenance import sha
from path_fold_fields import RVA,SIZE,seed,read
from multibyte_fields import read as read_scanner

def need(condition,message):
 if not condition:raise AssertionError('original path fold: '+message)

def record(path):
 raw=path.read_bytes();need(len(raw)==2637,'serialized record extent')
 sizes=[('meta',80),('state',72),('fp',112),('mb',257),('lower',256),('ctype',514),('old_ctype',514),('result',64),('fold',512),('classes',256)]
 result={};at=0
 for name,size in sizes:result[name]=raw[at:at+size];at+=size
 result['meta']=list(struct.unpack('<20I',result['meta']));result['state']=list(struct.unpack('<18I',result['state']));result['fold']=list(struct.unpack('<256H',result['fold']))
 return result

def check(directory):
 started=time.perf_counter();directory=directory.resolve();receipt=json.loads((directory/'receipt.json').read_text())
 need((receipt['kind'],receipt['mode'],receipt['exit'])==('explicit-original-capture','masked',125),'genuine masked role')
 for name,row in receipt['artifacts'].items():
  p=directory/name;need(Path(name).name==name and p.is_file()and p.stat().st_size==row['size']and sha(p)==row['sha256'],'artifact '+name)
 candidate=json.loads((directory/'candidate.json').read_text());target=directory/'loaded-target.bin';provider=directory/'loaded-msvcrt.bin'
 need(sha(target)==receipt['target_sha256']=='7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d','actual target hFile')
 need(sha(provider)==receipt['canonical_crt_sha256']=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'and provider.stat().st_size==277776,'actual provider hFile')
 bodies=[row for row in candidate['functions']if row['name']=='_initialize_path_fold_table'];need(len(bodies)==1,'exact fold vertex');body=bodies[0];need(body['name']=='_initialize_path_fold_table'and body['chunks']==[[0x12f3e,0x12f87],[0x307d2,0x307dc]],'sole83B body')
 following=candidate['expected_frontier']['kind']in ('environment-arguments-complete','option-vectors-complete','argument-cursor-complete','option-match-action','option-action-integer','option-integers-complete','option-path-normalizer','path-normalizer-complete','packing-initializer-allocator')
 expected=dict(rva=0x11e3c,native_symbol='new_option_vector',kind='environment-arguments-complete',calling_convention='cdecl',native_args=['count','mode'],caller=dict(native_symbol='parse_command_line',return_rva=0x12beb))if following else dict(rva=0x12f87,native_symbol='read_env_arguments',kind='path-fold-complete',calling_convention='cdecl',native_args=['arguments'],caller=dict(native_symbol='parse_command_line',return_rva=0x12bb3))
 if candidate['expected_frontier']['kind']in ('option-vectors-complete','argument-cursor-complete','option-match-action','option-action-integer','option-integers-complete','option-path-normalizer','path-normalizer-complete','packing-initializer-allocator'):
  from frontier_contract import validate_original_frontier
 else:need(candidate['expected_frontier']==expected,'actual next named ABI')
 need(candidate['path_fold_observers']==dict(entry_rva=0x12f3e,return_rva=0x12b80,iat_rva=0x5d05c,provider_rva=0xa5d3,api_return_rva=0x12f67,table_rva=0x55928,table_bytes=512,analysis_only_until_native_success=True),'exact observer plan')
 need(export_rva(provider,'_mbctolower')==0xa5d3,'canonical public export')
 need((directory/'mbctolower-provider-000.bin').read_bytes()==pe_read(provider,0xa5d3,54),'actual public body bytes')
 events=(directory/'events.txt').read_text();need('failure 'not in events,'controller failure')
 from frontier_contract import validate_original_frontier
 validate_original_frontier(events,candidate,receipt['probe'])
 need(events.count('path-fold-observer entry=00012f3e iat=0005d05c provider=0000a5d3 table=00055928 bytes=512 no-query-calls=1\n')==1,'observer inventory')
 module=int(re.findall(r'^module name=c1n32_l.dll actual=(\w+) ',events,re.M)[0],16);crt=int(re.findall(r'^module name=MSVCRT.dll actual=(\w+) ',events,re.M|re.I)[0],16)
 need(module==0x10600000,'preferred base only');source=(directory/'relocated-text.bin').read_bytes()
 need(source[0x12f40-0x1000:0x12f46-0x1000]==b'\x8b\x2d'+struct.pack('<I',module+0x5d05c)and source[0x12f65-0x1000:0x12f67-0x1000]==b'\xff\xd5','real EBP import call')
 initial=(directory/'compiler-bss-000.bin').read_bytes();entered=(directory/'path-fold-enter-target-BSS.bin').read_bytes();completed=(directory/'path-fold-complete-target-BSS.bin').read_bytes()
 need(read(initial)==[0]*256,'complete actual BSS defaults')
 classes=read_scanner((directory/'path-fold-enter-target-data.bin').read_bytes())['classes'];arguments=[i for i in range(256)if classes[i]!=35]
 need(len(arguments)==(255 if receipt['probe']=='restore'else 256),'case-specific API count')
 working=read(entered);need(working==(seed('helper')if receipt['probe']=='restore'else[0]*256),'actual helper input')
 if receipt['probe']=='restore':
  need((directory/'path-fold-reset-before-seed.bin').read_bytes()==bytes(512),'reset input before explicit seed')
  need((directory/'path-fold-helper-before-seed.bin').read_bytes()==bytes(512),'real BSS clear before helper seed')
  need(events.count('path-fold-seed phase=reset bytes=512 first=8101 step=37\n')==1 and events.count('path-fold-seed phase=helper bytes=512 first=4303 step=37\n')==1,'labeled auxiliary scope')
 else:need('path-fold-seed 'not in events,'no production input seed')
 need(len(list(directory.glob('path-fold-*-entry.bin')))==len(list(directory.glob('path-fold-*-return.bin')))==len(arguments),'complete actual API record count')
 reference_state=list(struct.unpack('<18I',(directory/'locale-next-words.bin').read_bytes()));fp=(directory/'locale-next-fp.bin').read_bytes();mapped=[];previous=-1;first_lower=None
 need(reference_state[0]==crt and reference_state[1:11]==[1,1252,0,0,1033,0,0,0,0,1252],'isolated SBCS prestate')
 for index,character in enumerate(arguments):
  for skipped in range(previous+1,character):need(classes[skipped]==35,'only genuine lead branch skips');working[skipped]=65535
  before=record(directory/('path-fold-%03d-entry.bin'%index));after=record(directory/('path-fold-%03d-return.bin'%index));a,b=before['meta'],after['meta']
  need(a[0:2]==[0,index]and b[0:2]==[1,index]and a[2]==b[2]and a[3]==crt+0xa5d3 and b[3]==module+0x12f67,'actual entry/return PC/index')
  need(a[4]+4==b[4]and a[4]==a[17]==b[17]and a[10]==b[10]==module+0x12f67 and a[11]==b[11]==character,'cdecl1 input/RET0')
  need(a[6:10]==b[6:10]and a[9]==crt+0xa5d3,'preserved EBX/ESI/EDI/cachedEBP')
  need(a[12:15]==b[12:15]==[0,0,0]and a[15:]==b[15:]and a[15:17]==[crt,module]and a[18]and a[19],'actual errors/PTD/provider identities')
  need(before['state']==after['state']==reference_state,'full locale scalars/pointer identity')
  need(before['fp']==after['fp']==fp,'full112 floating state unchanged')
  for key,suffix in [('mb','mbctype'),('ctype','ctype'),('old_ctype','old-ctype')]:need(before[key]==after[key]==(directory/('locale-next-'+suffix+'.bin')).read_bytes(),'full current/retained '+key)
  result=(directory/'locale-next-result.bin').read_bytes();need(before['result']==after['result']==result+bytes(64-len(result)),'return string lifetime')
  need(before['lower']==after['lower']and before['classes']==after['classes']==bytes(classes),'fresh provider map/target scanner')
  if first_lower is None:first_lower=before['lower']
  need(before['lower']==first_lower,'provider case map stable throughout actual call sequence')
  need(before['fold']==after['fold']==working,'fresh partial output and no API target writes')
  expected=before['lower'][character]if before['mb'][character+1]&16 else character
  need(b[5]==expected,'complete consumed unsigned result')
  mapped.append([character,b[5]]);working[character]=b[5]&65535;previous=character
 for skipped in range(previous+1,256):need(classes[skipped]==35,'final lead branch');working[skipped]=65535
 working[47]=92;need(read(completed)==working,'complete512B output/slash override')
 expected_bss=bytearray(entered);expected_bss[RVA-0x50000:RVA-0x50000+SIZE]=struct.pack('<256H',*working);need(bytes(expected_bss)==completed,'whole original BSS changes only owned table')
 for name in ('data','snapshot'):need((directory/('path-fold-enter-target-'+name+'.bin')).read_bytes()==(directory/('path-fold-complete-target-'+name+'.bin')).read_bytes(),'no unknown initialized/snapshot consumer '+name)
 returned=re.findall(r'^path-fold-helper-return seq=(\d+) pc=(\w+) esp=(\w+) entry-esp=(\w+) calls=(\d+)$',events,re.M);need(len(returned)==1,'actual helper return inventory')
 seq,pc,esp,entryesp,calls=returned[0];seq=int(seq);need(int(pc,16)==module+0x12b80 and int(esp,16)==int(entryesp,16)+4 and int(calls)==len(arguments),'helperRET/caller/count')
 final=re.findall(r'^trap seq=(\d+) rva=00012f87 .*stack=([0-9a-f,]+)$',events,re.M);need(len(final)==1,'actual named final entry');finalseq=int(final[0][0]);stack=[int(x,16)for x in final[0][1].split(',')];need(stack[:2]==[module+0x12bb3,0],'actual getenv absence and next argument')
 need(re.findall(r'^trap seq=%d rva=00012f87 .*eax=(\w+) '%finalseq,events,re.M)==['00000000'],'actual retained getenv result register')
 chain=b'\x68'+struct.pack('<I',module+0x631e8)+b'\xff\x15'+struct.pack('<I',module+0x5d050)+b'\x83\xc4\x04\x50\xe8'+struct.pack('<i',0x12f87-0x12bb3)
 need(source[0x12b9f-0x1000:0x12bb3-0x1000]==chain,'exact second getenv pushliteral/IAT/cleanup/PUSHEAX/consumer chain')
 need(pe_read(target,0x631e8,14)==b'MSC_CMD_FLAGS\0','original lookup literal bytes')
 from crt_vectors import vector
 root,environment=vector(directory/'locale-next-CRT-environment.bin')
 need(root==reference_state[15]and all(not value.upper().startswith(b'MSC_CMD_FLAGS=')for pointer,value in environment),'actual unchanged bound namespace finite absence')
 _,arguments_vector=vector(directory/'invoke-entry-argv.bin')
 need(len(arguments_vector)==11 and not arguments_vector[1][1].startswith(b'@'),'actual case takes direct environment path')
 need((directory/('compiler-bss-%03d.bin'%finalseq)).read_bytes()==completed,'next helper BSS equals genuine initializer return')
 need((directory/('fpu-%03d.bin'%seq)).read_bytes()==(directory/('fpu-%03d.bin'%finalseq)).read_bytes()==fp,'return and next fullFP')
 return dict(kind='own original-only actual path-fold/qualified CRT API proof; no native success or application acceptance',directory=str(directory),receipt_sha256=sha(directory/'receipt.json'),target_sha256=sha(target),provider_sha256=sha(provider),probe=receipt['probe'],calls=len(arguments),arguments=arguments,results=mapped,table_sha256=hashlib.sha256(struct.pack('<256H',*working)).hexdigest(),unchanged_errors=[0,0,0],full_FP_unchanged=True,second_getenv_proof=dict(kind='authenticated original call-site plus directly observed next-consumer result; not a separate original API entry or RET observation',call_rva=0x12ba4,iat_rva=0x5d050,literal_rva=0x631e8,literal='MSC_CMD_FLAGS',consumer_rva=0x12f87,result=0,namespace_sha256=sha(directory/'locale-next-CRT-environment.bin')),seconds=time.perf_counter()-started)

if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);a=parser.parse_args();print(json.dumps(check(a.directory),indent=2))
