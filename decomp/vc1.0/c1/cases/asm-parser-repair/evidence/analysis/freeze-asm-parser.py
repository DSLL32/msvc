from pathlib import Path
import json,re,hashlib,shutil
B=Path('/root/msvc2/decomp/vc1.0/c1');R=B.parents[2];S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');C=B/'cases/asm-parser-repair';P=S/'transport/asm-parser-repair-second';E=C/'evidence';(C/'expected/c1').mkdir(parents=True,exist_ok=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def item(p):return dict(path=str(p.relative_to(C)),bytes=p.stat().st_size,sha256=sha(p))
r=json.loads((P/'receipt.json').read_text());child=r['children'][0];assert len(child['argv'])==1 and len(child['environment'])==1
flags=child['environment'][0].split('=',1)[1];prefix=re.search(r' -il (\S+)',flags).group(1);binroot=flags.split()[1].rsplit(chr(92),1)[0].rstrip(chr(92));incroot=flags.split(' -I ')[1].strip();template=flags.replace(prefix,'ir').replace(binroot,'{bin}').replace(incroot,'{include}');outputs=[]
for p in sorted((P/'001-after').iterdir()):
 if p.name in ['audit.c','protocol.h']:continue
 name=p.name if p.suffix=='.sbr'else'ir'+p.name[-2:];dst=C/'expected/c1'/name;shutil.copy2(p,dst);outputs.append(dict(**item(dst),output_name=name))
status,startout,endout,starterr,enderr,errno=map(int,(P/'001-result.txt').read_text().split());assert status==5 and errno==0
stdout=(P/'stdout').read_bytes()[startout:endout];stderr=(P/'stderr').read_bytes()[starterr:enderr];assert not stderr and stdout.count(b' : error ')==5 and stdout.count(b' : warning ')==0
phase=dict(target='toolchains/1.0-nt/bin/C13232.EXE',sha256=sha(R/'toolchains/1.0-nt/bin/C13232.EXE'),arguments=[],canonical_working_directory=r'S:\vc1.0\c1\asm-parser-repair',environment={'MSC_CMD_FLAGS':template},inputs=[item(p)for p in sorted((C/'inputs').iterdir())],outputs=outputs,expected_exit=status,expected_stdout=stdout.decode('latin1'),expected_stderr=stderr.decode('latin1'),expected_absent_outputs=['audit.obj','audit.exe','msvc.pdb'],toolchain_dependencies=[dict(path='toolchains/1.0-nt/bin/C1.ERR',bytes=(R/'toolchains/1.0-nt/bin/C1.ERR').stat().st_size,sha256=sha(R/'toolchains/1.0-nt/bin/C1.ERR'))],original_CL_arguments=r['CL_arguments'],transport_reference='evidence/asm-parser.CL.receipt.json',expected_function_finishes=['packet_fragment','recovered_tail'],stdio=dict(stderr_to_stdout=True,provenance='Original CL prepare_execution dup2(1,2); actual spawn stdout intervals isolate four errors from CL source banner.'))
m=dict(case='Bounded assembler operand-context and generic grammar recovery',toolchain='1.0-nt',phases={'c1':phase},execution='Native-only fixtures; explicit original captures separate.',normalization='None.',capture_policy='Two actual CL child transports/full before-after sets. C1 returns5 for five ordinary errors; CL maps that to2. Prior original finishobserver and browsermarker prove recovered_tail beforefreeze. Exact diagnostics/order/CRLF/full partial artifacts and absent object/PDB files. Relative temporary prefix alone becomes ir.',runtime='Deliberate compile-error unit; no backend invocation or runtime success claimed. Valid recovered_tail function finish must be independently observed.')
(C/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
for run in ['first','second']:
 Q=S/'transport'/('asm-parser-repair-'+run);dst=E/'transport'/run;shutil.copytree(Q,dst,ignore=shutil.ignore_patterns('host.*','config.txt'));q=json.loads((Q/'receipt.json').read_text());q['C1_boundary_file_sets']={n:[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p),mtime_ns=p.stat().st_mtime_ns)for p in sorted((Q/n).iterdir())]for n in ['001-before','001-after']};(dst/'receipt.json').write_text(json.dumps(q,indent=2)+'\n')
 if run=='second':shutil.copy2(dst/'receipt.json',E/'asm-parser.CL.receipt.json')
 for o in outputs:
  raw=Q/'001-after'/o['output_name'] if o['output_name']=='audit.sbr'else next(p for p in(Q/'001-after').iterdir()if p.name.endswith(o['output_name'][2:])and p.name not in ['audit.c','protocol.h']);assert sha(raw)==o['sha256']
 a,b,c,d,e,f=map(int,(Q/'001-result.txt').read_text().split());assert a==5 and(Q/'stdout').read_bytes()[b:c]==stdout and(Q/'stderr').read_bytes()[d:e]==stderr
(E/'expected-C1.stdout').write_bytes(stdout);(E/'expected-C1.stderr').write_bytes(stderr)
shutil.copytree(S/'asm-parser-repair-probes/draft1',E/'probes/draft1')
print('C1 exit',status,[(o['output_name'],o['bytes'])for o in outputs]);print(stdout.decode('latin1'))
