from pathlib import Path
import json,re,hashlib,shutil,struct
B=Path('/root/msvc2/decomp/vc1.0/c1');R=B.parents[2];S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');C=B/'cases/pragma-hdrstop-pch';E=C/'evidence';phases={};repeats=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def item(p):return dict(path=str(p.relative_to(C)),bytes=p.stat().st_size,sha256=sha(p))
for tag in ['create','use']:
 P=S/'transport'/('pragma-hdrstop-pch-second-'+tag);r=json.loads((P/'receipt.json').read_text());child=r['children'][0];flags=child['environment'][0].split('=',1)[1];prefix=re.search(r' -il (\S+)',flags).group(1);binroot=flags.split()[1].rsplit(chr(92),1)[0].rstrip(chr(92));incroot=flags.split(' -I ')[1].strip();template=flags.replace(prefix,'ir').replace(binroot,'{bin}').replace(incroot,'{include}');inputs=[];outputs=[]
 for f in sorted((P/'001-before').iterdir()):
  if f.suffix in ['.c','.h']:dst=C/'inputs'/f.name;assert dst.read_bytes()==f.read_bytes()
  else:dst=C/'captured'/f.name;dst.parent.mkdir(exist_ok=True);shutil.copy2(f,dst)
  inputs.append(dict(**item(dst),mtime_ns=f.stat().st_mtime_ns))
 for f in sorted((P/'001-after').iterdir()):
  if f.suffix in ['.c','.h','.obj']:continue
  name='ir'+f.name[-2:]if f.name[-2:]in ['ex','sy','gl','in','db']else f.name
  dst=C/'expected'/('c1-'+tag)/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,dst);outputs.append(dict(**item(dst),output_name=name))
 raw=list(map(int,(P/'001-result.txt').read_text().split()));assert raw[0]==0
 stdout=(P/'stdout').read_bytes()[raw[1]:raw[2]];stderr=(P/'stderr').read_bytes()[raw[3]:raw[4]];assert not stdout and not stderr,(stdout,stderr)
 phase=dict(target='toolchains/1.0-nt/bin/C13232.EXE',sha256=sha(R/'toolchains/1.0-nt/bin/C13232.EXE'),arguments=[],canonical_working_directory=r'S:\vc1.0\c1\pragma-hdrstop-pch',environment={'MSC_CMD_FLAGS':template},inputs=inputs,outputs=outputs,expected_exit=0,expected_stdout=stdout.decode('latin1'),expected_stderr=stderr.decode('latin1'),expected_absent_outputs=['consumer.obj','check.exe','msvc.pdb']+(['producer.obj','consumer.sbr']if tag=='create'else[]),toolchain_dependencies=[dict(path='toolchains/1.0-nt/bin/C1.ERR',bytes=(R/'toolchains/1.0-nt/bin/C1.ERR').stat().st_size,sha256=sha(R/'toolchains/1.0-nt/bin/C1.ERR'))],original_CL_arguments=r['CL_arguments'],transport_reference='evidence/'+tag+'.CL.receipt.json',stdio=dict(stderr_to_stdout=True,provenance='Original CL prepare_execution dup2(1,2); actual child _spawnvpe transport captured.'))
 if tag=='use':
  phase['generated_input_dependencies']=[dict(case='hdrstop-create',output_name=n,input_name=n,clock_offset=None)for n in ['hdrstop.pch','producer.sbr']]
  phase['producer_phase']='c1-create';phase['captured_backend_dependency']=dict(object_input='producer.obj',input_outputs=[o['output_name']for o in phases['c1-create']['outputs']],provenance='Frozen untouched original C2 response object. Validate all actual native producer IR/browser/PCH artifacts before exposing it; no backend executed by fixture.')
 phases['c1-'+tag]=phase
 for run in ['first','second']:
  Q=S/'transport'/('pragma-hdrstop-pch-'+run+'-'+tag);dst=E/'transport'/(run+'-'+tag);shutil.copytree(Q,dst,ignore=shutil.ignore_patterns('host.*','config.txt'))
  if run=='second':shutil.copy2(Q/'receipt.json',E/(tag+'.CL.receipt.json'))
 first=S/'transport'/('pragma-hdrstop-pch-first-'+tag)/'001-after';second=P/'001-after';rows=[]
 for o in outputs:
  n=o['output_name'];a=first/n if n in ['hdrstop.pch','producer.sbr','consumer.sbr']else next(p for p in first.iterdir()if p.name.endswith(n[2:]));b=second/n if n in ['hdrstop.pch','producer.sbr','consumer.sbr']else next(p for p in second.iterdir()if p.name.endswith(n[2:]));assert a.read_bytes()==b.read_bytes(),n;rows.append(dict(name=n,bytes=a.stat().st_size,sha256=sha(a),changed_byte_offsets=[]))
 repeats.append(dict(phase='c1-'+tag,status='pass',files=rows))
for draft in ['draft1','draft2']:
 shutil.copytree(S/'hdrstop-preflight'/draft,E/'probes'/draft)
m=dict(case='Public pragma-hdrstop PCH create/reuse with postboundary packing reuse',toolchain='1.0-nt',phases=phases,execution='Native fixtures only; explicit original capture/build separate.',capture_policy='Two untouched original CL producer/consumer workflows freeze actual child argv/environment plus complete before/after file sets. Relative -il alone replaced with ir for independent C1. Both bareYc/Yu and filename hdrstop pragma validated untouched. Source/header mtimes pinned; observed PCH/object/browser input mtimes retained. Packing reuse occurs only afterproducer PCH emission, avoiding the known retiredpointer serialization limitation without changingtarget. Canonical S cwd is observable in browser/PCH records.',normalization='none; all browser/PCH/IR bytes exact, timestamps are deterministic input mtimes; no source timestamp macro used.',backend='Original C2 and LINK/runtime are preflight evidence only; consumer uses actual native PCH/browser producer outputs and frozen validated original producer.obj.')
(C/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');(E/'repeated-original-captures.json').write_text(json.dumps(dict(status='pass',phases=repeats),indent=2)+'\n');print([(p,[(r['output_name'],r['bytes'])for r in d['outputs']])for p,d in phases.items()])
