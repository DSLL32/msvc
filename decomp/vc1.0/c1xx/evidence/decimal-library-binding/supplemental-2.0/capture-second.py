"""Explicit actual _atoldbl public contract capture, outside native fixtures.

The original reference is untouched except for its public-entry breakpoint.
This does not accept/unmask any runtime extent or implement a CRT function.
"""
import argparse,fcntl,hashlib,importlib.util,json,os,re,shutil,subprocess,tempfile,time
from pathlib import Path
from binary import PE
from case_paths import paths,CAPTURE_ROOT,SCRATCH,canonical_parts,native_work
BINARY=Path('/root/msvc2/decomp/vc1.0/c1xx')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--case',required=True);parser.add_argument('--origin',type=Path,required=True);args=parser.parse_args()
 config=json.loads((BINARY/'functions.json').read_text());origin=json.loads((args.origin/'receipt.json').read_text())
 assert origin['candidate'] is None and origin['terminal']=='_begin_for_body'
 assert origin['configuration_sha256']==sha(BINARY/'functions.json') and origin['case']==args.case
 spec=importlib.util.spec_from_file_location('decimal_original_tool',BINARY/'scripts/wine-tool.py');tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
 directory=Path(tempfile.mkdtemp(prefix='decimal-boundary-',dir=CAPTURE_ROOT));original=PE(tool.TC/'bin/C1XX3232.EXE')
 assert sha(tool.TC/'bin/C1XX3232.EXE')==config['target']['sha256'] and original.read(0x84cf0,1)==b'\x83'
 data=bytearray(original.data);data[original.offset(0x84cf0)]=0xcc;image=directory/'public-entry-reference.exe';image.write_bytes(data)
 _,fixture,_,_=paths(config,args.case);phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1'];canonical=phase['canonical_working_directory']
 flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC/'bin'),include=tool.windows(tool.TC/'include'));program=phase['command_line_template'].format(bin=tool.windows(tool.TC/'bin'))
 lock=(CAPTURE_ROOT/'capture.lock').open('a');fcntl.flock(lock,fcntl.LOCK_EX);work=SCRATCH.joinpath(*canonical_parts(canonical));work.mkdir(parents=True,exist_ok=True)
 for p in work.iterdir():assert p.is_file() and not p.is_symlink();p.unlink()
 for item in phase['inputs']:
  source=fixture/item['path'];assert sha(source)==item['sha256'];shutil.copyfile(source,work/Path(item['path']).name)
 env=tool.environment();env['C1XX_DECIMAL_LOG']=tool.windows(directory/'original-observation.txt')
 command=['wine',tool.windows(Path('/root/.ai-tmp/msvc2/vc1.0/c1xx/decimal-second-call/second-observer.exe')),tool.windows(image),'',tool.windows(directory/'original-stdout.bin'),tool.windows(directory/'original-stderr.bin'),tool.windows(directory/'original-events.txt'),canonical,'MSC_CMD_FLAGS='+flags,program]
 started=time.perf_counter();subprocess.run(command,env=env,check=True,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=90);original_seconds=time.perf_counter()-started
 shutil.copytree(work,directory/'original-case');fcntl.flock(lock,fcntl.LOCK_UN);lock.close()
 raw=(directory/'original-observation.txt').read_text();left={}
 for key in ['return_va','output_address','text_address','entry_esp']:left[key]=int(re.search(r'^'+key+r' ([0-9a-f]+)$',raw,re.M)[1],16)
 for key in ['text','after_text','output']:left[key]=re.search(r'^'+key+r' ([0-9a-f]+)$',raw,re.M)[1]
 left['result']=int(re.search(r'^result (-?\d+)$',raw,re.M)[1]);assert left['output_address']==left['entry_esp']+44
 native,driveenv=native_work(directory/'native-drive',canonical)
 for item in phase['inputs']:shutil.copyfile(fixture/item['path'],native/Path(item['path']).name)
 commands=['break convert_floating_literal','ignore 1 1','run > '+str(directory/'native-stdout.bin')+' 2> '+str(directory/'native-stderr.bin'),'disable 1','break recon_atoldbl','continue','python','import gdb,json','frame=gdb.newest_frame(); caller=frame.older()','output=int(frame.read_var("output")); text=int(frame.read_var("text"))','parsed=int(caller.read_var("parsed").address)','assert caller.name()=="convert_floating_literal" and output==parsed','memory=gdb.selected_inferior()','def contents(address):','    value=bytearray()','    for i in range(129):','        b=bytes(memory.read_memory(address+i,1))[0]; value.append(b)','        if not b:return value.hex()','    raise AssertionError("unterminated numeric spelling")','before=contents(text)','entry_pc=int(gdb.parse_and_eval("$pc"))','end','finish','python','right=dict(text=before,after_text=contents(text),output=bytes(memory.read_memory(output,10)).hex(),result=int(gdb.parse_and_eval("(int)$eax")),entry="recon_atoldbl",entry_pc=entry_pc,caller=gdb.newest_frame().name(),output_matches_caller_parsed=True,output_prestate="unread/uncredited",defined_output_bytes=10)','open('+repr(str(directory/'native-observation.json'))+',"w").write(json.dumps(right,indent=2)+"\\n")','end']
 script=directory/'native.gdb';script.write_text('\n'.join(commands)+'\n')
 env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
 for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
 env.update(MSC_CMD_FLAGS=flags,RECON_PROGRAM_PATH=program,TZ='PST8PDT');env.update(driveenv)
 started=time.perf_counter();r=subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/C1XX3232.EXE')],cwd=native,env=env,capture_output=True,text=True,timeout=60);native_seconds=time.perf_counter()-started
 (directory/'native-gdb.log').write_text(r.stdout+r.stderr);print(directory,flush=True);assert r.returncode==0,r.stdout+r.stderr
 right=json.loads((directory/'native-observation.json').read_text());assert all(left[k]==right[k] for k in ['text','after_text','output','result'])
 words={int(i):int(v,16) for i,v in re.findall(r'^argument (\d+) ([0-9a-f]+)$',(args.origin/'events.txt').read_text(),re.M)}
 assert left['text_address']==0x8c158 and left['return_va']==0x74b57
 assert left['text']==left['after_text']=='322e3000' and left['output']=='00000000000000800040' and left['result']==0
 assert left['text']==left['after_text'] and len(bytes.fromhex(left['output']))==10
 for stream in ['stdout.bin','stderr.bin']:assert (directory/('original-'+stream)).read_bytes()==(directory/('native-'+stream)).read_bytes()
 assert {p.name:p.read_bytes() for p in (directory/'original-case').iterdir() if p.is_file()}=={p.name:p.read_bytes() for p in native.iterdir() if p.is_file()}
 receipt=dict(scope='Actual reached public CRT boundary only; untouched original reference apart from entry breakpoint, no acceptance or CRT source reconstruction',case=args.case,target_sha256=config['target']['sha256'],configuration_sha256=sha(BINARY/'functions.json'),origin=str(args.origin),origin_receipt_sha256=sha(args.origin/'receipt.json'),original=left,native=right,original_seconds=original_seconds,native_seconds=native_seconds,original_image_sha256=sha(image),observer_sha256=sha(Path('/root/.ai-tmp/msvc2/vc1.0/c1xx/decimal-second-call/second-observer.exe')),native_executable_sha256=sha(BINARY/'build/native/C1XX3232.EXE'))
 (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(directory/'receipt.json')
if __name__=='__main__':main()
