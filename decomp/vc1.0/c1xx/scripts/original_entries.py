"""Persist explicit original first-entry evidence; never runs during fixtures."""
import hashlib
import json
from pathlib import Path
import shutil

BINARY=Path(__file__).resolve().parents[1]
INDEX=BINARY/'cases/original-entry-references.json'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def register(case_name, directory, destination):
    destination=destination.resolve()
    receipt=json.loads((directory/'receipt.json').read_text())
    assert receipt['case']==case_name and 'first_entry_projection' in receipt
    destination.mkdir(parents=True,exist_ok=True)
    bundle={}
    for kind,filename in [('receipt','receipt.json'),('events','events.txt'),('plan','entries.plan')]:
        target=destination/('original-'+filename)
        shutil.copyfile(directory/filename,target)
        bundle[kind]=str(target.relative_to(BINARY))
        bundle[kind+'_sha256']=sha(target)
    index=json.loads(INDEX.read_text()) if INDEX.exists() else {}
    index[case_name]=bundle
    INDEX.write_text(json.dumps(index,indent=2)+'\n')
    return bundle
