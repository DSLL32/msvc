"""Exact one-field reconstruction fix; never relax original execution inputs."""
import hashlib,json
from pathlib import Path


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def need(ok,message):
    if not ok:raise ValueError('filename reset refinement: '+message)


def check(original,output):
    project=Path(__file__).resolve().parents[1];packet=project/'cases/minimal/path-fold-reset-refinement.json';rule=json.loads(packet.read_text());anchor=Path(rule['anchor'])
    need(rule['originals'].get(str(original.resolve()))==sha(original/'receipt.json'),'not an authenticated unchanged original role')
    need(sha(anchor/'receipt.json')==rule['anchor_receipt_sha256']and sha(anchor/'build-provenance.json')==rule['anchor_build_provenance_sha256'],'old compiled snapshot identity')
    receipt=json.loads((anchor/'receipt.json').read_text())
    for name,row in receipt['artifacts'].items():
        p=anchor/name;need(p.is_file()and p.stat().st_size==row['size']and sha(p)==row['sha256'],'old actual artifact '+name)
    before=json.loads((original/'trial.json').read_text());after=json.loads((output/'trial.json').read_text());baseline=json.loads((anchor/'trial.json').read_text())
    need(before['trial']==after['trial']=='_initialize_lowercase_table'and before['case']==after['case']=='minimal','exact candidate scope')
    need(json.loads((original/'state-model.json').read_text())['schema']==json.loads((output/'state-model.json').read_text())['schema']==36,'exact encountered field-map schema')
    # The prior anchor was already an ELF-only native refresh of this same
    # original semantic core; its failed semantic state is not reused as proof.
    strip=lambda x:{k:v for k,v in x.items()if k!='bound_inputs'}
    need(strip(before)==strip(after),'target/case/config/accepted/extent/ABI/input semantics changed')
    # The anchor supplies compiled-source provenance only, not execution
    # results. Its declared restore mode may differ from this normal role.
    for key in ('trial','case','target','driver','accepted','ranges','imports'):
        need(baseline.get(key)==before.get(key),'old compiler anchor identity differs '+key)
    need(sha(anchor/'state-model.json')==sha(original/'state-model.json')==sha(output/'state-model.json'),'compiler anchor field/layout differs')
    old_bound,new_bound=before['bound_inputs'],after['bound_inputs'];allowed={'build/c1n32_l.dll','build/c1xx.map','build/native/c1xx-host'}
    need(set(old_bound)==set(new_bound)and {k for k in old_bound if old_bound[k]!=new_bound[k]}==allowed,'not exactly rebuilt DLL/MAP/ELF bindings')
    for key in old_bound:
        if key not in allowed:need(baseline['bound_inputs'][key]==old_bound[key],'old compiler anchor unrelated to original '+key)
    for suffix,key,oldkey,newkey in [('c1n32_l.dll','build/c1n32_l.dll','before_rebuilt_sha256','after_rebuilt_sha256'),('c1xx.map','build/c1xx.map','before_map_sha256','after_map_sha256')]:
        need(sha(original/suffix)==sha(anchor/suffix)==old_bound[key]==rule[oldkey]and sha(output/suffix)==new_bound[key]==rule[newkey],'actual old/new '+suffix)
    records=[json.loads((d/'build-provenance.json').read_text())for d in (anchor,output)]
    for d,record in zip((anchor,output),records):
        need(record['artifact_sha256']==sha(d/'native-executable.elf'),'actual compiled ELF')
        for name,digest in record['source_content'].items():
            p=d/'source-content'/digest;need(p.is_file()and sha(p)==digest,'compiled source-content '+name)
    a,b=records;need(a['command']==b['command'],'native compiler/link command changed')
    source=rule['source_key'];before_sha,after_sha=rule['source_before_sha256'],rule['source_after_sha256']
    need(a['source_content'][source]==before_sha and b['source_content'][source]==after_sha,'specific compiled target source hashes')
    old=(anchor/'source-content'/before_sha).read_text();new=(output/'source-content'/after_sha).read_text();line=rule['added_line']
    need(line=='            frontend_bss.filename_case_map[i] = 0;\n'and new==old.replace('            frontend_bss.checksum_tables.low[i] = 0;\n','            frontend_bss.checksum_tables.low[i] = 0;\n'+line)and old.count('            frontend_bss.checksum_tables.low[i] = 0;\n')==1,'not exact reset-only source insertion')
    source_paths=lambda record:{k:v for k,v in record['source_content'].items()if k.endswith(('.c','.h','.cpp'))}
    ac,bc=source_paths(a),source_paths(b);need(ac.keys()==bc.keys()and {k for k in ac if ac[k]!=bc[k]}=={source},'another application/shared compiled C/header changed')
    return dict(kind='checked filename-map reset-only compiled source correction; identical original target/mask/case/config/ABI/runtime inputs',rule_path=str(packet),rule_sha256=sha(packet),original_receipt_sha256=sha(original/'receipt.json'),original_trial_sha256=sha(original/'trial.json'),native_trial_sha256=sha(output/'trial.json'),old_compiled_source=before_sha,new_compiled_source=after_sha,allowed_build_bindings=sorted(allowed),old_ELF=sha(anchor/'native-executable.elf'),new_ELF=sha(output/'native-executable.elf'),semantic_correctness_claimed=False,current_strict_audit_required=True)
