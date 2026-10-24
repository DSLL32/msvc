"""Decode only the observed include-lower API; never certify case acceptance."""
import hashlib,json,re,struct
from pathlib import Path
from binary import PE,u32
from dependency_packet import exports
from dependency_response import phase_snapshot
from import_residuals import qualified_slots
from mktemp_state import CRT_SHA,original_states
from mask import construct
sha=lambda b:hashlib.sha256(b).hexdigest()

def decode(directory):
 d=Path(directory);raw=(d/'capture.json').read_bytes();c=json.loads(raw)
 assert c['case']=='mre-object'and c['mre_response']is True and c['baseline']is False and c['controller_error']is None
 assert c['helper_status']in(0,188)and c['command'][-2]=='1'
 def read(name):
  b=(d/name).read_bytes();assert c['files'][name]=={'bytes':len(b),'sha256':sha(b)},'changed raw '+name
  return b
 for n in c['files']:read(n)
 assert read('mre-observer.txt')==b'CL_MRE_SELECTOR_RESPONSE_V1\n'
 config=json.loads(read('config.json'));assert sha(read('config.json'))==c['config_sha256']
 assert {'_option_signature','_option_matching_selector'}<=set(config['accepted'])and'_compare_option_strings'not in config['accepted']
 assert sha(construct(config))==sha(read('loaded-target.bin'))==c['original_mask_sha256']
 assert sha(read('loaded-msvcrt.bin'))==c['loaded_msvcrt_sha256']==CRT_SHA
 assert sha(read('reference-debugger.exe'))==c['helper_sha256']==c['helper_build']['binary']['sha256']
 for n,h in c['helper_sources'].items():assert sha(read(n))==h==c['helper_build']['sources']['scripts/'+n]
 assert qualified_slots(PE(d/'loaded-target.bin'))[0x40a26c]=={'dll':'msvcrt.dll','name':'_mbslwr'}
 phases=('lower-before','lower-after');errors=original_states(d,phases,'observer/mre-events.txt');states={};contexts={}
 for p in phases:states[p],contexts[p]=phase_snapshot(d,read,p,errors[p],mre_prefix=True)
 b,a=(contexts[p]for p in phases)
 identity=struct.unpack('<6I',read('observer/lower-before-identity.bin'));assert read('observer/lower-after-identity.bin')==read('observer/lower-before-identity.bin')
 function,esp,buffer,ebp,length,base=identity;provider=PE(d/'loaded-msvcrt.bin');export=exports(provider)['_mbslwr'];assert base==provider.base and function==base+export['rva']
 assert u32(b,184)==0x403abc and u32(a,184)==0x403ac2
 assert u32(b,196)==u32(a,196)==esp and u32(b,180)==u32(a,180)==ebp
 assert u32(a,176)==buffer
 assert all(u32(b,o)==u32(a,o)for o in(156,160,164,180))
 before=read('observer/lower-before-buffer.bin');after=read('observer/lower-after-buffer.bin')
 assert len(before)==len(after)==length and before.endswith(b'\0')and after.endswith(b'\0')and b'\0'not in before[:-1]and b'\0'not in after[:-1]
 assert before.startswith(b'-inc=')and max(before)<128 and max(after)<128
 assert errors['lower-before']==errors['lower-after']
 assert b[28:140]==a[28:140]
 for k in states['lower-before']:
  if k not in('event','directory'):assert states['lower-before'][k]==states['lower-after'][k],'API changed '+k
 events=read('observer/mre-events.txt').decode('ascii')
 assert len(re.findall(r'^lower phase=before ',events,re.M))==len(re.findall(r'^lower phase=after ',events,re.M))==1
 profile={p:{'crt_words_hex':read('observer/'+p+'-crt-words.bin').hex(),'mb_state_hex':read('observer/'+p+'-mb-state.bin').hex(),'ctype_hex':read('observer/'+p+'-ctype.bin').hex()}for p in phases}
 return {'kind':'Observed original _mbslwr only; no selector/case/native acceptance from partial or complete capture','capture':{'path':str(d/'capture.json'),'sha256':sha(raw)},'capture_helper_status':c['helper_status'],'qualified_import':{'dll':'MSVCRT.DLL','sha256':CRT_SHA,'IAT':0x40a26c,'name':'_mbslwr','ordinal':export['ordinal'],'function':function},'ABI':{'cdecl_bytes':4,'ESP':esp,'EBP':ebp,'input':buffer,'result':u32(a,176),'result_alias':True,'nonvolatile_preserved':True},'before_hex':before.hex(),'after_hex':after.hex(),'initialized_bytes':length,'errors':errors,'x87_before_hex':b[28:140].hex(),'x87_after_hex':a[28:140].hex(),'profile_tables':profile,'state_before':states['lower-before'],'state_after':states['lower-after'],'directory_qualification':'Observer-owned snapshot outputs are present in directory records; both raw lists retained, no directory byte identity claimed.','limitations':['Helper188 capture is partial; final comparator/state proof absent.','No allocation tails observed; exact initialized NUL string only.','No generalized locale/lowercase behavior inferred; owner must review canonical implementation.'],'decoder_sha256':sha(Path(__file__).read_bytes())}
