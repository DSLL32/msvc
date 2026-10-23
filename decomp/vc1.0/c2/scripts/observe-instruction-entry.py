"""Read-only actual MachineInstruction input at the first text-listing entry."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--function',default='@write_instruction_text@4');parser.add_argument('--receipt',type=Path,default=Path('cases/dbglist-instruction-abi'));args=parser.parse_args()
started=time.perf_counter();receipt=args.receipt;receipt.mkdir()
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
name=args.function
assert name in ['@write_instruction_text@4','@write_special_instruction@4','@write_instruction_mnemonic@4','@write_instruction_comment@4','@write_instruction_operand@12','@write_operand_prefix@12']
nargs=3 if name in ['@write_instruction_operand@12','@write_operand_prefix@12']else 1
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='instruction-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for row in phase['inputs']:
        source=fixture/row['path'];assert sha(source)==row['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name)
    plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],0x100|nargs,name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment()
    for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
    env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
    command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)]
    begin=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
    (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
scratch=Path(tempfile.mkdtemp(prefix='instruction-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="import gdb,json; p=int(gdb.parse_and_eval('instruction')); s=gdb.parse_and_eval('recon_iob[1]'); used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0; print('C2_INSTRUCTION '+json.dumps(dict(pointer=p,instruction_hex=bytes(gdb.selected_inferior().read_memory(p,32)).hex(),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex()if used else ''))))"
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break '+name.split('@')[1],'-ex','run > native-inferior.stdout','-ex','python '+snapshot]
if nargs==3:
    command+=['-ex',"python import gdb,json; p=int(gdb.parse_and_eval('operand')); print('C2_OPERAND '+json.dumps(dict(pointer=p,bytes=int(gdb.parse_and_eval('bytes')),operand_hex=bytes(gdb.selected_inferior().read_memory(p,8)).hex())))"]
command+=[str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log
native=json.loads(re.search('C2_INSTRUCTION (.*)',log)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name]
arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==nargs and arguments[0]and native['pointer']
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};original_bytes=bytes.fromhex(lines[4].removeprefix('instruction_hex '));native_bytes=bytes.fromhex(native['instruction_hex'])
# Retain both complete raw records. Next/previous addresses are host-specific;
# compare actual topology and all remaining scalar bytes, after inspecting the
# first encountered PUSH register record. Do not generalize pointer operands.
original_words=list(struct.unpack('<8I',original_bytes));native_words=list(struct.unpack('<8I',native_bytes))
assert original_words[2:]==native_words[2:]==[0xd40,1,1,5,0,0],(original_words,native_words)
assert original_words[:2]==[arguments[0]+32,arguments[0]-32]
assert native_words[:2]==[native['pointer']+32,native['pointer']-32]
assert [bool(v)for v in original_words[:2]]==[bool(v)for v in native_words[:2]]
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,formals=[dict(name='instruction',location='ECX',type='MachineInstruction pointer',original=arguments[0],native=native['pointer'])],hidden_inputs=[],callee_stack_cleanup=0,original_words=original_words,native_words=native_words,original_state=state,native=native,original_stdout=stdout,target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual formal pointer and full32-byte input record retained. Entry consumes ECX instruction; original callee-saved registers saved before assignment, RET0 (full body analyzed separately). Pointee scalar projection is restricted to this inspected first record; linked addresses prove adjacent+32/-32 topology; PUSH EBP record has only register operands, not universal deep equivalence. Known implicit stdout/column/x87 observed.')
if nargs==3:
    operand=json.loads(re.search('C2_OPERAND (.*)',log)[1]);raw=lines[5].removeprefix('operand_hex ')
    assert arguments[1]==arguments[0]+16 and operand['pointer']==native['pointer']+16
    assert arguments[2]==operand['bytes']==4 and raw==operand['operand_hex']=='0100000005000000'
    r['formals']+=[dict(name='operand',location='EDX',type='MachineOperand pointer',original=arguments[1],native=operand['pointer'],original_offset_from_instruction=16,native_offset_from_instruction=16,defined_input_hex=raw),dict(name='bytes',location='entryESP+4',type='unsigned32',value=4)]
    r['native_operand']=operand;r['callee_stack_cleanup']=4
    location='2a10c'if name=='@write_instruction_operand@12'else '2a7b9'
    r['scope']=r['scope'].replace('Entry consumes ECX instruction;', 'Entry consumes ECX instruction, EDX operand and entryESP+4 unsigned width;').replace('RET0 (full body analyzed separately)', 'RET4 at'+location+' (full body analyzed separately)')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual instruction formal and record/state match',original_words)
