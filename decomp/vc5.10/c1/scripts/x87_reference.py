"""Bind the architectural input observed before any original DLL instruction."""
import json,re,struct
from provenance import sha,ROOT

def bind(original,output):
    original=original.resolve()
    receipt=json.loads((original/'receipt.json').read_text())
    assert receipt['kind']=='explicit-original-capture' and receipt['mode']=='masked'
    for name,row in receipt['artifacts'].items():
        path=original/name
        assert path.is_file() and path.stat().st_size==row['size'] and sha(path)==row['sha256']
    assert receipt['target_sha256']==sha(original/'loaded-target.bin')==sha(ROOT/'toolchains/5.10/bin/C1.DLL')
    assert receipt['canonical_crt_sha256']==sha(original/'loaded-msvcrt.bin')==sha(ROOT/'toolchains/5.10/bin/MSVCRT.DLL')
    events=(original/'events.txt').read_text()
    assert re.search(r'^guard rva=0001202a original=53',events,re.M)
    assert re.search(r'^trap seq=0 rva=0001202a ',events,re.M)
    raw=(original/'fpu-000.bin').read_bytes();assert len(raw)==112
    cw,sw,tag=(value&0xffff for value in struct.unpack_from('<3I',raw))
    assert (cw,sw,tag)==(0x027f,0x0020,0xffff)
    reference=sha(original/'receipt.json')
    binding=dict(kind='once-before-lifecycle captured architectural x87 input',original_directory=str(original),
        original_receipt_sha256=reference,original_float_save_sha256=sha(original/'fpu-000.bin'),
        original_environment_sha256=sha(original/'invoke-environment.bin'),guard_rva=0x1202a,
        cw=cw,sw=sw,tag=tag,top=0,live_registers=[],
        excluded=['IP','DP','selectors','empty register payload','transport upper words','Cr0NpxState'])
    (output/'x87-entry-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
    return reference
