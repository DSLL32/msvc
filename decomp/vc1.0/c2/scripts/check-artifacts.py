"""Native-only rejection checks for object/listing/frontend-PDB artifact oracles."""
from pathlib import Path
import copy
import json
import shutil
import tempfile
import time
from artifacts import normalize
import importlib.util
spec=importlib.util.spec_from_file_location("check_native",Path(__file__).with_name("check-native.py"))
native=importlib.util.module_from_spec(spec);spec.loader.exec_module(native)
outputs,captured=native.outputs,native.captured
from case_paths import C2_SCRATCH

started=time.perf_counter()
fixture=Path('../fixtures/c2-debug-asm-listing')
phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
rejected=[]
with tempfile.TemporaryDirectory(prefix='artifact-oracles-',dir=C2_SCRATCH) as temporary:
    work=Path(temporary)
    for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
    for row in phase['outputs']:shutil.copyfile(fixture/row['path'],work/row['output_name'])
    result=outputs(work,phase,fixture)
    assert len(result['artifacts'])==3
    object_path=work/'dbglist.obj';original=object_path.read_bytes()
    changed=bytearray(original);changed[4]^=1;object_path.write_bytes(changed)
    outputs(work,phase,fixture) # The one proven clock field may differ.
    object_path.write_bytes(original)
    for row in phase['outputs']:
        path=work/row['output_name'];original=path.read_bytes()
        changed=bytearray(original);changed[-1]^=1;path.write_bytes(changed)
        try:outputs(work,phase,fixture)
        except AssertionError:rejected.append('corrupt '+path.name)
        else:raise AssertionError('Corrupt artifact accepted: '+path.name)
        path.write_bytes(original)
    path=work/'dbglist.cod';original=path.read_bytes();path.unlink()
    try:outputs(work,phase,fixture)
    except FileNotFoundError:rejected.append('missing listing')
    else:raise AssertionError('Missing artifact accepted')
    path.write_bytes(original)
    (work/'unexpected.dbg').write_bytes(b'extra')
    try:outputs(work,phase,fixture)
    except AssertionError:rejected.append('unexpected artifact')
    else:raise AssertionError('Unexpected artifact accepted')
    (work/'unexpected.dbg').unlink()
    for name,fields in [('msvc.pdb',[dict(offset=4,bytes=4)]),('dbglist.obj',[dict(offset=8,bytes=4)])]:
        row=copy.deepcopy(next(r for r in phase['outputs']if r['output_name']==name));row['ignored_fields']=fields
        try:normalize((work/name).read_bytes(),row)
        except AssertionError:rejected.append('unproven clock '+name)
        else:raise AssertionError('Unproven clock exemption accepted')
    row=next(r for r in phase['inputs']if r['path']=='c2/msvc.pdb')
    path=work/'msvc.pdb';changed=bytearray(path.read_bytes());changed[0]^=1;path.write_bytes(changed)
    try:captured(path,row)
    except AssertionError:rejected.append('changed pre-C2 PDB input')
    else:raise AssertionError('Changed captured dependency accepted')
receipt=dict(method='Synthetic native artifact-oracle checks using frozen original outputs, not target execution',
    positive_artifacts=3,clock_positive='Only COFF timestamp4..7',rejected=rejected,seconds=time.perf_counter()-started)
Path('cases/artifact-oracle-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('3 artifact positives and%d rejections pass'%len(rejected))
