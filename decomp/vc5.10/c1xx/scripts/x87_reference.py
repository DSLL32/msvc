"""Own captured earliest DLL-entry FP words, verified before native host launch."""
import hashlib,json,re,struct
from pathlib import Path


def prepare(project,original,output,trial):
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    original=original.resolve();receipt=json.loads((original/'receipt.json').read_text());binding=json.loads((output/'system-info-binding.json').read_text());reference=sha(original/'receipt.json')
    from system_info_reference import checked_trial_refresh
    refresh=checked_trial_refresh(original,output,trial)
    if binding.get('native_only_trial_refresh')!=refresh or binding.get('native_trial_sha256')!=sha(output/'trial.json'):raise ValueError('x87 native refresh lacks checked exact trial identity')
    if binding['original_directory']!=str(original) or binding['original_receipt_sha256']!=reference or binding['trial_sha256']!=sha(original/'trial.json'):raise ValueError('x87 profile is not the already verified exact original input closure')
    raw=original/'x87-context-000.bin';row=receipt['artifacts'].get(raw.name)
    if not row or row['sha256']!=sha(raw) or row['size']!=112 or raw.stat().st_size!=112:raise ValueError('x87 entry raw112 provenance differs')
    cw,sw,tag=(v&0xffff for v in struct.unpack_from('<3I',raw.read_bytes()))
    if (cw,sw,tag)!=(0x027f,0x0020,0xffff) or not re.search(r'^trap seq=0 rva=0002e3c6 ',(original/'events.txt').read_text(),re.M):raise ValueError('unreviewed actual target initial FP words or entry')
    source=Path(__file__);(output/'x87-profile-factory.py').write_bytes(source.read_bytes())
    result={'kind':'actual target earliest empty x87 entry; one host apply before lifecycle','original_directory':str(original),'original_receipt_sha256':reference,'original_float_save_path':str(raw),'original_float_save_sha256':sha(raw),'target_sha256':binding['target_sha256'],'canonical_crt_sha256':binding['canonical_crt_sha256'],'guard_rva':0x2e3c6,'cw':cw,'sw':sw,'tag':tag,'top':0,'live_registers':[],'factory_sha256':sha(source),'factory_artifact':'x87-profile-factory.py','trial_sha256':sha(original/'trial.json'),'application_point':'before first target lifecycle, after existing host precision setup; never calculator reseed'}
    (output/'x87-entry-binding.json').write_text(json.dumps(result,indent=2)+'\n')
    return reference
