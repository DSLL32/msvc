#!/usr/bin/env python3
"""Fresh native-only initial MRE prefix experiment; never an acceptance receipt."""
import argparse,hashlib,json,os,shutil,subprocess,time
from pathlib import Path
from state import vector,environment
from case_inputs import load,copy_inputs,prepare_c_drive
from mre_native import Provider
from build_inputs import verify_native_build
from mktemp_state import original as mktemp

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',type=Path,required=True);parser.add_argument('--directory',type=Path,required=True)
parser.add_argument('--signature-candidate',action='store_true')
parser.add_argument('--selector-candidate',action='store_true')
args=parser.parse_args();original=args.original.resolve();directory=args.directory.resolve()
assert not directory.exists();directory.mkdir()
case=load('mre-object');copy_inputs(case,directory)
shutil.copyfile(original/'config.json',directory/'config.json')
binary=PROJECT/'build/native-trace/CL.EXE';build_path=Path(str(binary)+'.build.json');build_raw=build_path.read_bytes();build=json.loads(build_raw)
verify_native_build(build,sha(binary),PROJECT);(directory/'native-build.json').write_bytes(build_raw)
provider=Provider(original,directory,signature_candidate=args.signature_candidate,selector_candidate=args.selector_candidate)
argv=vector(original/'argv.bin')[1];assert argv[1:]==case['arguments']
env=os.environ.copy()
for key,value in environment(provider.environment_text).items():
    if value is None:env.pop(key,None)
    else:env[key]=value
for key in('CL_DEPENDENCY_FD','CL_DEPENDENCY_CONFIG','CL_LINK_BINDING','CL_LINK_RESPONSE_ID','RECON_LOCAL_CLOCK','RECON_SPAWN_FD'):env.pop(key,None)
env.update(provider.environment());env['CL_ENTRY_PROFILE']='mre-object-canonical-msvcrt-v1'
env['RECON_PROGRAM_PATH']=(original/'pgmptr.bin').read_bytes()[:-1].decode('latin1')
env['RECON_RESOURCE_PROVIDER']=str(directory/'resources.bin');shutil.copyfile(PROJECT/'build/resources.bin',directory/'resources.bin')
env['RECON_DRIVE_R']=str(ROOT)
# Same fixed logical S: path, backed by this isolated native directory only.
original_cwd=provider.packet['calls'][0]['before']['cwd'].rstrip('\\')
assert original_cwd.startswith('S:\\vc5.10\\cl\\')
mirror=directory/'drive-s'/Path(*original_cwd[3:].split('\\'));mirror.parent.mkdir(parents=True);mirror.symlink_to(directory,target_is_directory=True)
env['RECON_DRIVE_S']=str(directory/'drive-s')
c_drive=prepare_c_drive(original,directory)
if c_drive is not None:env['RECON_DRIVE_C']=str(c_drive)
temporary=provider.packet['entry_and_mktemp'];call=temporary['calls'][0]
env['RECON_MKTEMP_TEMPLATE']=call['template'];env['RECON_MKTEMP_SUFFIX']=call['result'][-6:]
env['RECON_MKTEMP_PRESTATE']=','.join(str(call['before'][key])for key in('errno','doserrno','last_error'))
(directory/'mktemp-expected.json').write_text(json.dumps(temporary,indent=2)+'\n')
for name in('gdb_entry.py','gdb_streams.py','gdb_mktemp.py','gdb_mre.py'):
    shutil.copyfile(PROJECT/'scripts'/name,directory/name)
wrapper=directory/'exec-wrapper.py'
wrapper.write_text('#!/usr/bin/python3\nimport os,sys\n'+
    'fd=os.open('+repr(str(directory/'trace.txt'))+',os.O_CREAT|os.O_WRONLY|os.O_TRUNC,0o600)\nos.set_inheritable(fd,True)\nos.environ["RECON_TRACE_FD"]=str(fd)\n'+
    'os.execv(sys.argv[1],'+repr(argv)+')\n');wrapper.chmod(0o755)
config=json.loads((original/'config.json').read_text());names=[r['native_symbol']for r in config['driver']if r['name']in config['accepted']]
(directory/'trace-filter.txt').write_text(''.join(name+'\n'for name in names));(directory/'trace-seen.txt').write_text('')
env['RECON_TRACE_FILTER']=str(directory/'trace-filter.txt');env['RECON_TRACE_SEEN']=str(directory/'trace-seen.txt')
entry=directory/'gdb-entry.py';entry.write_text('import sys\nsys.path.insert(0,'+repr(str(directory))+')\nfrom gdb_entry import capture\ncapture('+repr(str(directory))+')\nfrom gdb_mktemp import install\ninstall('+repr(str(directory))+')\nfrom gdb_mre import install\ninstall('+repr(str(directory))+')\n')
frontier_name='compare_option_strings'if args.selector_candidate else'option_matching_selector'if args.signature_candidate else'option_signature'
frontier=directory/'gdb-frontier.py';frontier.write_text('from gdb_mre import observe\nimport gdb\nassert gdb.newest_frame().name()=='+repr(frontier_name)+'\nprint("NATIVE_FRONTIER_SYMBOL="+gdb.newest_frame().name())\nobserve('+repr(str(directory))+',"mre-frontier",{})\n')
command=['/usr/bin/gdb','-q','-nx','-batch','-ex','set pagination off','-ex','set exec-wrapper '+str(wrapper),'-ex','set args '+subprocess.list2cmdline(case['arguments']),
    '-ex','tbreak *main','-ex','run > '+str(directory/'stdout.bin')+' 2> '+str(directory/'stderr.bin'),'-ex','source '+str(entry),'-ex','continue','-ex','source '+str(frontier),'--args',str(binary)]
execution={'kind':'Native MRE execution binding','signature_candidate':args.signature_candidate,'selector_candidate':args.selector_candidate,
    'native_sha256':sha(binary),'rebuilt_sha256':sha(PROJECT/'build/cl.exe'),
    'native_build_sha256':sha(directory/'native-build.json'),'config_sha256':sha(directory/'config.json'),
    'resource_sha256':sha(directory/'resources.bin'),
    'original_capture':{'path':str(original/'capture.json'),'sha256':sha(original/'capture.json')},
    'sources':{name:sha(PROJECT/'scripts'/name)for name in('mre_native.py','mre_state.py','mre_elf.py','selector_native_state.py','native-mre-prefix.py','gdb_mre.py')}}
(directory/'native-execution.json').write_text(json.dumps(execution,indent=2)+'\n')
started=time.perf_counter();error=None
try:
    result=subprocess.run(command,cwd=directory,env=env,stdin=subprocess.DEVNULL,capture_output=True,pass_fds=provider.descriptors(),timeout=30)
    (directory/'debugger.txt').write_bytes(result.stdout+result.stderr)
    pid=json.loads((directory/'native-process.json').read_bytes())['pid'];provider.finish(pid)
    assert (directory/'native-mre-frontier-gdb.json').exists(),'native real signature frontier missing'
    assert (directory/'stdout.bin').read_bytes()==b''and(directory/'stderr.bin').read_bytes()==(original/'stderr.bin').read_bytes()
    from mre_state import verify
    verification=verify(original,directory,provider,signature_candidate=args.signature_candidate,selector_candidate=args.selector_candidate)
    (directory/'native-state-verification.json').write_text(json.dumps(verification,indent=2)+'\n')
except Exception as failure:error=repr(failure)
finally:provider.close()
report={'kind':'Fresh native selector-candidate observation'if args.selector_candidate else'Fresh native signature-candidate observation'if args.signature_candidate else'Initial native MRE prefix experiment, NOT acceptance','seconds':time.perf_counter()-started,
    'original_capture_sha256':sha(original/'capture.json'),'native_sha256':sha(binary),'native_build_sha256':sha(directory/'native-build.json'),
    'sources':{name:sha(PROJECT/'scripts'/name)for name in('mre_native.py','mre-provider.c','mre-host.c','native-host.c','native-mre-prefix.py','gdb_mre.py','mre_state.py','selector_native_state.py')},'error':error}
assert sha(binary)==execution['native_sha256']and sha(PROJECT/'build/cl.exe')==execution['rebuilt_sha256']and build_path.read_bytes()==build_raw,'paired build changed during native capture'
assert sha(directory/'resources.bin')==execution['resource_sha256']
(directory/'native-prefix.json').write_text(json.dumps(report,indent=2)+'\n')
capture={'schema':1,'platform':'native',**case,'native_sha256':execution['native_sha256'],
    'rebuilt_sha256':execution['rebuilt_sha256'],'config_sha256':execution['config_sha256'],
    'native_build':{'path':str(directory/'native-build.json'),'sha256':execution['native_build_sha256']},
    'controller_sha256':sha(Path(__file__)),'gdb_status':result.returncode,'seconds':report['seconds'],
    'controller_error':error,'mre_signature_response':args.signature_candidate,'mre_selector_response':args.selector_candidate,
    'files':{p.relative_to(directory).as_posix():{'sha256':sha(p),'bytes':p.stat().st_size}for p in directory.rglob('*')if p.is_file()}}
(directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
print(json.dumps(report,indent=2));assert error is None,error
