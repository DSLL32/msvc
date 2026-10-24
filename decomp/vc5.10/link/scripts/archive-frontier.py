#!/usr/bin/env python3
"""Persist a validated frontier without duplicating provided DLL/EXE files."""
import argparse
import hashlib
import gzip
import json
from pathlib import Path
import shutil

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native',type=Path,required=True)
parser.add_argument('--name',required=True)
args=parser.parse_args()
validation=json.loads((args.native/'acceptance-validation.json').read_text())
case=json.loads((Path(validation['original'])/'receipt.json').read_text()).get('case','help')
assert case in ('help','hello-win32','hello-dll')
destination=Path(__file__).resolve().parents[1]/'cases'/case/'frontiers'/args.name
destination.mkdir(parents=True,exist_ok=False)
shutil.copytree(args.native,destination/'native')
if validation.get('native_cleanup'):shutil.copytree(validation['native_cleanup'],destination/'native-cleanup')
original=Path(validation['original'])
output=destination/'original';output.mkdir()
modules={}
for path in original.iterdir():
    if not path.is_file():continue
    if path.name.startswith('module-'):
        modules[path.name]=validation['audit']['input_sha256_before'].get(str(path))
        continue
    if path.name=='mask.bin':
        (output/'mask.bin.gz').write_bytes(gzip.compress(path.read_bytes(),mtime=0))
    else:shutil.copyfile(path,output/path.name)
(destination/'validation.json').write_text(json.dumps(validation,indent=2)+'\n')
if validation['audit'].get('full_link_import_audit'):
    # These prospective counterpart bindings change as later functions move
    # rebuilt addresses. Preserve their exact earlier input bytes first.
    binary=Path(__file__).resolve().parents[1]
    manifest=[]
    for name,digest in validation['audit']['input_sha256_before'].items():
        source=Path(name)
        try:relative=source.relative_to(binary)
        except ValueError:continue
        selected=(relative.as_posix()in {
            'scripts/full-link-preservation.py','scripts/member_pointers.py',
            'scripts/accept.py','scripts/boundary_state.py','scripts/hello_boundary.py',
            'scripts/context_heap.py','scripts/section_state.py','scripts/group_state.py','scripts/observers.py',
            'scripts/pass-one-preservation.py','cases/hello-win32/pass-one-import-preservation.json',
            'scripts/collision-preservation.py','scripts/collision_state.py','scripts/file_open_state.py','scripts/file_move_state.py','scripts/file_mapping_state.py','scripts/mapping_completion_state.py','scripts/library_creation_state.py','scripts/input_open_state.py','scripts/mapping-preservation.py','scripts/library-finder-preservation.py','cases/hello-win32/library-finder-import-preservation.json','cases/hello-win32/mapping-import-preservation.json','scripts/mapped_input_contract.py','cases/hello-win32/mapped-open-byte-input.json','cases/hello-win32/input-loader-contract.json','cases/hello-win32/collision-import-preservation.json',
            'cases/hello-win32/full-link-import-preservation.json',
            'cases/hello-win32/full-link-closed-slot.json',
            'cases/hello-win32/full-link-audit-trial/metadata.json'}or
            (relative.parts[0]=='build'and source.suffix=='.obj'))
        if not selected:continue
        data=source.read_bytes();assert hashlib.sha256(data).hexdigest()==digest
        snapshot=destination/'full-link-proof'/relative
        snapshot.parent.mkdir(parents=True,exist_ok=True);snapshot.write_bytes(data)
        manifest.append(dict(project_path=relative.as_posix(),snapshot=str(snapshot.relative_to(destination)),sha256=digest))
    (destination/'full-link-proof/input-snapshots.json').write_text(json.dumps(manifest,indent=2)+'\n')
(destination/'storage.json').write_text(json.dumps(dict(original_capture=str(original),
    module_copies='Actual debugger hFile copies stay in the receipt-named original capture. Their bytes/hashes remain pinned by the unmodified receipt; provided toolchain files are not duplicated here.',
    mask_encoding='gzip with mtime 0; decode before applying the original receipt hash'),indent=2)+'\n')
print(destination)
