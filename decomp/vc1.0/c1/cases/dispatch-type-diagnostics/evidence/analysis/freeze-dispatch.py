from pathlib import Path
import json,hashlib,shutil
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');T=S/'dispatch-preflight-case';C=B/'cases/dispatch-type-diagnostics';E=C/'evidence';E.mkdir()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
m=json.loads((T/'manifest.json').read_text());phase=m['phases']['c1'];reference=S/'captures/dispatch-prefreeze-finishes';events=(reference/'events').read_text();finishes=[line.split()[-1]for line in events.splitlines()if line.startswith('function-symbol ')and' entry 'in line];assert finishes==['validate_header','recovered_tail'],finishes;assert events.endswith('exit 4\n')
shutil.copytree(T/'expected',C/'expected')
for run in ['first','second']:
 q=S/'transport'/('dispatch-type-diagnostics-'+run);dst=E/'transport'/run;shutil.copytree(q,dst,ignore=shutil.ignore_patterns('host.*','config.txt'));a,b,c,d,e,f=map(int,(q/'001-result.txt').read_text().split());assert a==4 and f==0;assert(q/'stdout').read_bytes()[b:c]==phase['expected_stdout'].encode('latin1')and(q/'stderr').read_bytes()[d:e]==b''
 for o in phase['outputs']:
  raw=q/'001-after'/o['output_name']if o['output_name']=='audit.sbr'else next(p for p in(q/'001-after').iterdir()if p.suffix not in['.c','.h']and p.name.endswith(o['output_name'][2:]));assert sha(raw)==o['sha256'],o['output_name']
 for item in phase['inputs']:assert (q/'001-before'/Path(item['path']).name).read_bytes()==(C/item['path']).read_bytes()
 sidecar=dict(provenance='Derived full boundary inventory from preserved raw before/after snapshots; raw receipt/events unchanged',raw_receipt_sha256=sha(dst/'receipt.json'),sets={n:[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p),mtime_ns=p.stat().st_mtime_ns)for p in sorted((q/n).iterdir())]for n in['001-before','001-after']});(dst/'file-sets-derived.json').write_text(json.dumps(sidecar,indent=2)+'\n')
 if run=='second':shutil.copy2(q/'receipt.json',E/'dispatch.CL.receipt.json')
for n in ['receipt.json','events','stdout','stderr']:shutil.copy2(reference/n,E/('prefreeze-finishes.'+n))
for draft in ['draft1','draft2']:shutil.copytree(S/'dispatch-diagnostics-preflight'/draft,E/'preflight'/draft)
m['status']='frozen original-backed expected-error case; reconstruction pending';m['capture_policy']='Two untouched actual CL transports/full beforeafter inputs/outputs match raw allsix partial files and exact fourerrors+threewarnings. Independent original function entry/finish observer proves validate_header and recovered_tail plus browser terminalmarker beforefreeze. C1exit4/CLexit2; no backend/runtime claim. Relative temporary prefix alone becomes ir; absolutecanonicalS spelling retained.';m['execution']='Native-only fixtures; original captures explicit outsidefixtures.';(C/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
p=B/'cases/approved-queue.json';q=json.loads(p.read_text());q['status']='frozen fourerrors/threewarnings; recovered_tail proved; reconstruct next genuine frontier';q['cases'][0]['status']='frozen original-backed';p.write_text(json.dumps(q,indent=2)+'\n');print('Frozen4errors3warnings sixpartialfiles, bothfinishes proved')
