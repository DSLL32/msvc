#!/usr/bin/env python3
"""Persist a validated frontier without duplicating provided DLL/EXE files."""
import argparse
import gzip
import json
from pathlib import Path
import shutil

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native',type=Path,required=True)
parser.add_argument('--name',required=True)
args=parser.parse_args()
validation=json.loads((args.native/'acceptance-validation.json').read_text())
destination=Path(__file__).resolve().parents[1]/'cases/help/frontiers'/args.name
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
(destination/'storage.json').write_text(json.dumps(dict(original_capture=str(original),
    module_copies='Actual debugger hFile copies stay in the receipt-named original capture. Their bytes/hashes remain pinned by the unmodified receipt; provided toolchain files are not duplicated here.',
    mask_encoding='gzip with mtime 0; decode before applying the original receipt hash'),indent=2)+'\n')
print(destination)
