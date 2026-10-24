"""Authenticated, explicitly partial MRE prefix analysis; supplies no native success."""
import hashlib,json,re,struct
from pathlib import Path
from binary import PE,u32
from case_inputs import load,verify_dependencies
from dependency_packet import exports,vector,TARGET_SHA
from dependency_response import phase_snapshot
from mktemp_state import CRT_SHA,original_states,original as mktemp

PROJECT=Path(__file__).resolve().parents[1]
MSPDB_SHA='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
PHASES=tuple('mre-%d-%s'%(i,when)for i in range(1,4)for when in('before','after'))+('mre-signature-entry',)
sha=lambda raw:hashlib.sha256(raw).hexdigest()

def target_objects(read,phase):
 prefix='observer/'+phase+'-'
 arguments=struct.unpack('<3I',read(prefix+'prepare-jobs-arguments.bin'))
 driver=struct.unpack('<11I',read(prefix+'driver.bin'))
 addresses=read(prefix+'job-addresses.bin');assert len(addresses)%4==0
 jobs=[]
 for i,address in enumerate(struct.unpack('<%dI'%(len(addresses)//4),addresses)):
  header=struct.unpack('<3I',read(prefix+'job-%d-header.bin'%i))
  raw=read(prefix+'job-%d-input.bin'%i);assert len(raw)==16
  next_input,name,kind,explicit,sequence=struct.unpack('<3IHh',raw)
  text=read(prefix+'job-%d-source.bin'%i);assert text[-1:]==b'\0'and b'\0'not in text[:-1]
  jobs.append({'address':address,'next':header[0],'input_address':header[1],'status':header[2],
   'input':{'next':next_input,'name_pointer':name,'kind':kind,'explicit_kind':explicit,'sequence':sequence,'name':text[:-1].decode('ascii')}})
 assert arguments[1]==(jobs[0]['address']if jobs else 0)
 assert all(j['next']==(jobs[i+1]['address']if i+1<len(jobs)else 0)for i,j in enumerate(jobs))
 addresses=read(prefix+'option-addresses.bin');assert len(addresses)%4==0
 options=[]
 for i,address in enumerate(struct.unpack('<%dI'%(len(addresses)//4),addresses)):
  record=struct.unpack('<6I',read(prefix+'option-%d.bin'%i));strings={}
  for index,name in [(2,'matching'),(3,'name'),(4,'value')]:
   if record[index]:
    raw=read(prefix+'option-%d-field-%d.bin'%(i,index));assert raw[-1:]==b'\0'and b'\0'not in raw[:-1]
    strings[name]=raw[:-1].decode('ascii')
   else:strings[name]=None
  options.append({'address':address,'next':record[0],'flags':record[1],'matching_pointer':record[2],
   'name_pointer':record[3],'value_pointer':record[4],'origin':record[5],**strings})
 assert driver[0]==(options[0]['address']if options else 0)
 assert all(o['next']==(options[i+1]['address']if i+1<len(options)else 0)for i,o in enumerate(options))
 return {'prepare_jobs_arguments':list(arguments),'driver_words':list(driver),'jobs':jobs,'options':options}

def decode(directory):
 directory=Path(directory);capture_bytes=(directory/'capture.json').read_bytes();capture=json.loads(capture_bytes)
 assert capture['mre_analysis']is True and capture['baseline']is True
 assert not any(capture.get(n)for n in('dependency_analysis','dependency_complete','dependency_response','link_response','clock_analysis','clock_response'))
 assert capture['case']=='mre-prefix'and capture['controller_error']is None and capture['helper_status']==0 and capture['command'][-2]=='0'
 def read(name):
  assert name in capture['files'];p=directory/name;assert p.is_file()and not p.is_symlink()
  raw=p.read_bytes();assert capture['files'][name]=={'bytes':len(raw),'sha256':sha(raw)},'changed raw '+name
  return raw
 for name in capture['files']:read(name)
 assert read('mre-observer.txt')==b'CL_MRE_PREFIX_ANALYSIS_V2\n'
 assert capture['target_sha256']==sha(read('CL.EXE'))==sha(read('loaded-target.bin'))==TARGET_SHA
 assert sha(read('MSVCRT.DLL'))==sha(read('loaded-msvcrt.bin'))==capture['loaded_msvcrt_sha256']==CRT_SHA
 assert sha(read('MSPDB50.DLL'))==sha(read('loaded-mspdb50.bin'))==MSPDB_SHA
 assert sha(read('reference-debugger.exe'))==capture['helper_sha256']==capture['helper_build']['binary']['sha256']
 for name,digest in capture['helper_sources'].items():assert sha(read(name))==digest==capture['helper_build']['sources']['scripts/'+name]
 case=load('mre-prefix');assert all(capture[k]==case[k]for k in('case','arguments','inputs'))
 verify_dependencies(directory)
 for item in case['inputs']:assert read(item['name'])==(PROJECT/item['source']).read_bytes()
 events=read('observer/mre-events.txt').decode('ascii');target_events=read('events.txt').decode('ascii')
 assert re.findall(r'^snapshot phase=(\S+)',events,re.M)==list(PHASES)
 assert re.findall(r'^analysis-stop kind=(\S+) address=([0-9a-f]{8}) calls=(\d+) event=(\d+)$',events,re.M)==[('option-signature','0040393a','3',re.search(r'analysis-stop .*event=(\d+)',events)[1])]
 assert re.findall(r'^exit .*status=(\d+)$',target_events,re.M)==['28676']and not re.search(r'^trap |^exception ',target_events,re.M)
 assert 'mre-unexpected-call'not in target_events
 trace=read('trace.txt').decode('ascii').splitlines();assert 'execute_pass'not in trace and 'final_pass'not in trace
 assert read('mre-calls.txt')==b'3\n'
 assert not re.search(r'^module .*name=c1',target_events,re.M|re.I)
 errors=original_states(directory,PHASES,'observer/mre-events.txt');phases={};contexts={}
 for phase in PHASES:
  phases[phase],contexts[phase]=phase_snapshot(directory,read,phase,errors[phase],mre_prefix=True)
  phases[phase]['target_objects']=target_objects(read,phase)
  lowio=read('observer/'+phase+'-lowio-0.bin')[108:144]
  assert lowio[4]==(0 if phase=='mre-1-before'else 0x11)
  phases[phase]['descriptor3']={'record_raw_hex':lowio.hex(),'handle':u32(lowio,0),'flags':lowio[4],'meaning':'Initially closed; after Open binaryFOPEN|FNOINHERIT, provider-owned lifetime. Other opaque lock fields retained raw/unclaimed.'}
 provider=PE(directory/'loaded-mspdb50.bin');export_table=exports(provider)
 records=[];engine=driver=None;parent_frame=None
 for i,(site,slot,cleanup,name,ordinal)in enumerate([(0x403597,0x40a300,24,'_MREFOpenByName@24',75),(0x4035c1,0x40a314,8,'_MREQueryMreDrv@8',79),(0x4035ca,0x40a304,4,'_MREDrvOneTimeInit@4',70)],1):
  before='mre-%d-before'%i;after='mre-%d-after'%i;b=contexts[before];a=contexts[after]
  identity=struct.unpack('<4I',read('observer/'+before+'-identity.bin'))
  assert read('observer/'+before+'-identity.bin')==read('observer/'+after+'-identity.bin')
  base,function,ebp,esp=identity
  assert base==provider.base and export_table[name]['ordinal']==ordinal and function==base+export_table[name]['rva']
  assert u32(b,180)==u32(a,180)==ebp and u32(b,196)==esp and u32(a,196)==esp+cleanup
  if parent_frame is None:parent_frame=ebp
  else:assert parent_frame==ebp
  assert u32(b,184)==site+1 and u32(a,184)==site+7
  arguments=struct.unpack('<%dI'%(cleanup//4),read('observer/'+before+'-arguments.bin'))
  assert read('observer/'+before+'-arguments.bin')==read('observer/'+after+'-arguments.bin')==read('observer/'+before+'-stack.bin')[:cleanup]
  calls=re.findall(r'^mre sequence=%d phase=(before|after) call=([0-9a-f]{8}) return=([0-9a-f]{8}) function=([0-9a-f]{8}) esp=([0-9a-f]{8}) eax=([0-9a-f]{8}) event=(\d+)$'%i,events,re.M)
  assert len(calls)==2
  for row,phase,context in zip(calls,(before,after),(b,a)):
   assert row[0]==phase.rsplit('-',1)[1]and tuple(int(x,16)for x in row[1:6])==(site,site+6,function,u32(context,196),u32(context,176))and int(row[6])==phases[phase]['event']
  record={'name':name,'ordinal':ordinal,'qualified_IAT':slot,'callsite':site,'function':function,'stdcall_bytes':cleanup,'arguments':list(arguments),'before':phases[before],'after':phases[after],'result_word':u32(a,176)}
  if i==1:
   assert arguments[0]==ebp-12 and arguments[2]==ebp-20 and arguments[3]==0x4091b8 and arguments[4:]==(0,1)
   assert read('observer/'+before+'-pdb-path.bin')==read('observer/'+after+'-pdb-path.bin')==b'case.idb\0'
   assert u32(a,176)==1 and u32(read('observer/'+after+'-error-out.bin'),0)==0
   engine=u32(read('observer/'+after+'-engine-out.bin'),0);assert engine
   assert len(read('observer/'+before+'-message.bin'))==1024
   assert read('observer/'+before+'-message.bin')==read('observer/'+after+'-message.bin')
   record['out_pointer_contract']={'engine':engine,'error':u32(read('observer/'+after+'-error-out.bin'),0),'message_before_sha256':sha(read('observer/'+before+'-message.bin')),'message_unchanged':True,'initial_engine_and_error_payload':'uninitialized caller locals; raw retained, no input value/equality claim'}
  elif i==2:
   assert arguments[0]==engine and arguments[1]==ebp-4
   driver=u32(read('observer/'+after+'-driver-out.bin'),0);assert driver==engine+4
   record['out_pointer_contract']={'driver':driver,'initial_driver_payload':'uninitialized caller local; unclaimed','returned_word_consumption':'ignored by CL; raw EAX retained'}
  else:
   assert arguments==(driver,)
   record['return_consumption']='void/ignored EAX; raw retained, no result semantics invented'
  records.append(record)
 assert read('observer/mre-signature-entry-pending.bin')==bytes(20)
 frontier_stack=struct.unpack('<16I',read('observer/mre-signature-entry-stack.bin'))
 caller=struct.unpack('<3I',read('observer/mre-signature-entry-prepare-jobs-arguments.bin'))
 assert frontier_stack[:2]==(0x4035f2,caller[0])
 directory_changes={}
 before=phases['mre-1-before'];after=phases['mre-1-after']
 assert len(before['target_objects']['jobs'])==1 and before['target_objects']['jobs'][0]['input']['name']=='test.c'
 assert errors['mre-1-before']=={'errno':0,'doserrno':2,'last_error':2}
 assert all(errors[p]=={'errno':2,'doserrno':2,'last_error':0}for p in PHASES[1:])
 for p in PHASES:
  assert phases[p]['mb_codepage']==phases[p]['lc_codepage']==0 and phases[p]['lc_handles']==[0]*6 and phases[p]['x87_control_word']==639
  for field in('crt_environment_sha256','os_environment_sha256','mb_state_sha256','ctype_sha256','pctype_address','file0_2_words','streams'):
   assert phases[p][field]==before[field],'changed actual parent state '+field
  assert read('observer/'+p+'-context.bin')[28:140]==contexts['mre-1-before'][28:140],'actual live FP transition requires explicit handling'
  objects=phases[p]['target_objects'];initial=before['target_objects']
  assert objects['prepare_jobs_arguments']==initial['prepare_jobs_arguments']and objects['options']==initial['options']and objects['jobs']==initial['jobs']
  expected_driver=list(initial['driver_words'])
  if p=='mre-signature-entry':expected_driver[5]=0;expected_driver[6]=caller[0]+20
  assert objects['driver_words']==expected_driver
  if p!='mre-1-before':
   assert phases[p]['open_descriptors']==after['open_descriptors']
   assert phases[p]['open_descriptors'][3]['query']['position_low']==1024
  directory_changes[p]=phases[p]['directory']
 return {'kind':'partial original-only MRE prefix; no native success/acceptance','case':'mre-prefix','capture':{'sha256':sha(capture_bytes)},'provider_sha256':MSPDB_SHA,'calls':records,'signature_frontier':{'address':0x40393a,'caller_return':frontier_stack[0],'driver':frontier_stack[1],'caller_formals':list(caller),'pending_defined_zero':True,'full_state':phases['mre-signature-entry']},'entry_and_mktemp':mktemp(directory),'directory_snapshots':directory_changes,'opaque_roles':{'engine':engine,'driver':driver,'driver_offset':4,'object_contents_or_extent':'unobserved/unclaimed'},'file_effect_gap':{'case.idb':{'final_after_termination':capture['files']['case.idb'],'intermediate_API_return_bytes':'unobserved; final bytes not substituted','independently_opened_path_to_fd3_file_identity':'unobserved; next required capture must compare IDs and preserve cursor','live_CRT_descriptor':3,'position':1024}},'normal_completion':False,'application_credit':0}
