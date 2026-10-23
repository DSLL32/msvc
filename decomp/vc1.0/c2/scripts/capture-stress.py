"""Explicit original CL/C1/C2 capture for the approved optimizer queue.

Never invoked by fixture checks. The fixture must contain source/<stem>.c and
no manifest yet. Original launches, environment, intermediate IR and object are
captured through CL's CreateProcessA thunk; portable backend is independently
verified with the original executable before registration.
"""
from pathlib import Path
import sys,json,hashlib,subprocess,shutil,time,re,argparse,tempfile
import pe_indexes as idx
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--fixture',type=Path,required=True)
parser.add_argument('--stem',required=True)
parser.add_argument('--flags',nargs='+',default=['/O2','/G5'],
                    help='Original CL optimization/CPU flags, recorded verbatim')
args=parser.parse_args()
root=Path(__file__).resolve().parents[4]
fixture=args.fixture.resolve();stem=args.stem
assert fixture.name in {'c2-optimizer-loops','c2-optimizer-register-pressure','c2-optimizer-aliasing','c2-optimizer-floating-point',
                        'c2-i386-integer','c2-x87-stack','c2-i486-floating'}
assert all(flag in {'/O1','/O2','/Oi','/G3','/G4','/G5'} for flag in args.flags)
assert re.fullmatch('[a-z][a-z0-9_]*',stem) and not (fixture/'manifest.json').exists()
scratch=Path(tempfile.mkdtemp(prefix=fixture.name+'-capture-',dir=Path.home()/'.ai-tmp/vc1.0-c2'))
work=scratch/'work';work.mkdir();raw=scratch/'raw';raw.mkdir();tc=idx.wt.TC;env=idx.wt.environment();env.pop('MSC_CMD_FLAGS',None);env.pop('LINK',None);env.pop('_LINK_',None)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def invoke(tag,args,**kw):
 t=time.perf_counter();v=subprocess.run(['wine',*map(str,args)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,stdin=subprocess.DEVNULL,timeout=90,**kw);(scratch/(tag+'.stdout')).write_bytes(v.stdout);(scratch/(tag+'.stderr')).write_bytes(v.stderr);timings[tag]=dict(seconds=time.perf_counter()-t,returncode=v.returncode,stdout_sha256=hashlib.sha256(v.stdout).hexdigest(),stderr_sha256=hashlib.sha256(v.stderr).hexdigest());assert not v.returncode,(tag,v.stdout,v.stderr);return v
w=idx.wt.windows;timings={};helper=root/'decomp/vc1.0/fixtures/minimal-c/capture-cl.c'
invoke('capture-compile',[tc/'bin/CL.EXE','/nologo','/c','/Od','/G3','/Fo'+w(scratch/'capture.obj'),w(helper)],cwd=scratch)
invoke('capture-link',[tc/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+w(scratch/'capture.exe'),w(scratch/'capture.obj')],cwd=scratch)
shutil.copyfile(fixture/('source/'+stem+'.c'),work/(stem+'.c'))
cl_arguments=['/nologo','/c',*args.flags,stem+'.c']
result=invoke('pipeline',[scratch/'capture.exe',w(tc/'bin/CL.EXE'),'11000',w(raw),w(work),' '.join(cl_arguments),stem+'.obj'],cwd=work)
assert b'exit 0; captured 2 launches' in result.stdout
(fixture/'c2').mkdir();(fixture/'expected').mkdir();(fixture/'capture').mkdir()
for ext in ['ex','sy','in','gl']:shutil.copyfile(raw/('c2-input.'+ext),fixture/('c2/ir'+ext))
shutil.copyfile(work/(stem+'.obj'),fixture/('expected/'+stem+'.obj'))
for p in raw.iterdir():
 if not p.name.startswith('c2-input'):shutil.copyfile(p,fixture/'capture'/p.name)
for kind in ['stdout','stderr']:shutil.copyfile(scratch/('pipeline.'+kind),fixture/('capture/pipeline.'+kind))
block=(raw/'spawn-2.environment').read_bytes();entries=block.rstrip(b'\0').split(b'\0');assert len(entries)==1 and entries[0].startswith(b'MSC_CMD_FLAGS=');flags=entries[0][14:].decode('ascii');portable=flags.replace(w(tc/'bin'),'{bin}');portable=re.sub(r'-il [^ ]+ ', '-il ir ',portable)
def record(path):return dict(path=path,bytes=(fixture/path).stat().st_size,sha256=sha(fixture/path))
obj=record('expected/'+stem+'.obj');norm=bytearray((fixture/obj['path']).read_bytes());norm[4:8]=b'\0'*4;obj.update(output_name=stem+'.obj',ignored_fields=[dict(offset=4,bytes=4,name='COFF TimeDateStamp')],normalized_sha256=hashlib.sha256(norm).hexdigest())
phase=dict(target='toolchains/1.0-nt/bin/C23232.EXE',sha256=sha(tc/'bin/C23232.EXE'),arguments=[],environment={'MSC_CMD_FLAGS':portable},environment_inheritance=False,working_directory='c2',captured_application=(raw/'spawn-2.application').read_text(),captured_command_line=(raw/'spawn-2.command').read_text(),captured_MSC_CMD_FLAGS=flags,expected_exit=0,expected_stdout='',expected_stderr='',inputs=[record('c2/ir'+ext) for ext in ['ex','sy','in','gl']],outputs=[obj],toolchain_dependencies=[dict(path=str(p.relative_to(root)),sha256=sha(p)) for p in [tc/'bin/C23.ERR']])
m=dict(project='vc1.0',case=fixture.name,description='Original CL '+ ' '.join(cl_arguments[:-1])+' optimizer stress: '+fixture.name,source=record(('source/'+stem+'.c')),capture=dict(helper=str(helper.relative_to(root)),helper_sha256=sha(helper),cl_arguments=cl_arguments,environment={k:env[k] for k in ['INCLUDE','LIB','WINEPATH']},working_directory=str(work),temporary_directory=str(work),frontend=dict(application=(raw/'spawn-1.application').read_text(),command_line=(raw/'spawn-1.command').read_text(),environment_hex=(raw/'spawn-1.environment').read_bytes().hex(),cwd=(raw/'spawn-1.cwd').read_text()),dependency_policy='No source headers; original CL/C1/C2 and error files are pinned; no reconstructed frontend involved',dependencies=[dict(path=str(p.relative_to(root)),sha256=sha(p)) for p in [tc/'bin/CL.EXE',tc/'bin/CL.ERR',tc/'bin/C13232.EXE',tc/'bin/C23232.EXE',tc/'bin/C1.ERR',tc/'bin/C23.ERR']]),phases={'c2':phase})
# Verify the portable relative IR prefix by an independent original backend run.
verify=scratch/'standalone';verify.mkdir()
for source in phase['inputs']:shutil.copyfile(fixture/source['path'],verify/Path(source['path']).name)
env['MSC_CMD_FLAGS']=portable.format(bin=w(tc/'bin'))
r=invoke('standalone',[root/'decomp/vc1.0/c2/build/debug-run.exe',w(tc/'bin/C23232.EXE'),'',w(scratch/'standalone.stdout'),w(scratch/'standalone.stderr'),w(scratch/'standalone.events'),w(verify)],cwd=verify)
assert (scratch/'standalone.events').read_text()=='exit 0\n';assert not (scratch/'standalone.stdout').read_bytes() and not (scratch/'standalone.stderr').read_bytes();actual=bytearray((verify/(stem+'.obj')).read_bytes());actual[4:8]=b'\0'*4;assert actual==norm
(fixture/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');(fixture/'capture-validation.json').write_text(json.dumps(dict(timing=timings,standalone='exit0, empty streams, object matches pipeline after only timestamp',scratch=str(scratch)),indent=2)+'\n');print(json.dumps(phase,indent=2))
