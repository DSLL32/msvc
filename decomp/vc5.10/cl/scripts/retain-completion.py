#!/usr/bin/env python3
"""Retain a new case proof as ordinary files; existing archives are untouched."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
SCRATCH=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('case');parser.add_argument('pair',type=Path)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
parser.add_argument('--milestone',help='Retain an accepted prefix before later case builds replace its artifacts')
args=parser.parse_args()
assert args.milestone is None or (args.milestone not in ('.','..') and Path(args.milestone).name==args.milestone),'invalid milestone name'
destination=PROJECT/'cases'/args.case/('milestones/'+args.milestone if args.milestone else 'completion')
assert not destination.exists(),'never replace retained execution evidence'
destination.mkdir(parents=True);records={};seen=set();repository={}
case_files={}
for p in sorted((PROJECT/'cases'/args.case/'reference').rglob('*')):
    if p.is_file():case_files.setdefault(sha(p),p)
for p in sorted((ROOT/'toolchains/5.10').rglob('*')):
    if p.is_file():repository.setdefault(sha(p),p)
tool=ROOT/'tooling/graph-audit/target/release/graph-audit'
tool_revision=subprocess.check_output(['git','log','-1','--format=%H','--','tooling/graph-audit'],cwd=ROOT,text=True).strip()
tool_sources={}
for relative in subprocess.check_output(['git','ls-files','tooling/graph-audit'],cwd=ROOT,text=True).splitlines():
    p=ROOT/relative
    assert p.read_bytes()==subprocess.check_output(['git','show',tool_revision+':'+relative],cwd=ROOT),'auditor source differs from pinned commit'
    tool_sources[relative]=sha(p)
auditor={'path':str(tool.relative_to(ROOT)),'sha256':sha(tool),'source_commit':tool_revision,'source_files':tool_sources}

def add(path):
    path=Path(path).absolute()
    if path in seen:return
    assert path.is_file(),str(path)
    seen.add(path);digest=sha(path);record={'sha256':digest,'bytes':path.stat().st_size}
    if digest in repository:
        record['repository_original']=str(repository[digest].relative_to(ROOT))
    elif path==tool:record['pinned_auditor']=auditor
    elif digest in case_files:
        record['owned_case_file']=str(case_files[digest].relative_to(PROJECT))
    else:
        name=('project/'+path.relative_to(PROJECT).as_posix()if path.is_relative_to(PROJECT)else
            'scratch/'+path.relative_to(SCRATCH).as_posix()if path.is_relative_to(SCRATCH)else
            'external/'+str(path).lstrip('/'))
        out=destination/name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,out)
        assert sha(out)==digest;record['file']=name
    records[str(path)]=record
    def walk(value):
        if isinstance(value,dict):
            if isinstance(value.get('path'),str)and value.get('sha256'):
                target=Path(value['path'])
                if target.is_absolute():
                    assert target.is_file(),'missing bound input '+str(target)
                    assert sha(target)==value['sha256'],'changed bound input '+str(target)
                    add(target)
            for v in value.values():walk(v)
        elif isinstance(value,list):
            for v in value:walk(v)
    if path.suffix=='.json'or path.parent.name=='build-source-blobs':
        try:value=json.loads(path.read_bytes())
        except (ValueError,UnicodeDecodeError):return
        walk(value)

add(args.pair)
for directory in (args.original,args.native):
    add(directory/'capture.json')
    capture=json.loads((directory/'capture.json').read_text())
    for name,item in capture['files'].items():
        p=directory/name;assert sha(p)==item['sha256'];add(p)
for p in sorted((PROJECT/'src').iterdir()):
    if p.is_file():add(p)
for name in ('build/cl.exe','build/cl.map','build/native-trace/CL.EXE','Makefile'):
    add(PROJECT/name)
review=args.pair.parent/'orchestrator-review.json'
if review.exists():add(review)
manifest={'schema':1,'kind':'ordinary hash-bound completion evidence; historical provenance paths unchanged',
    'case':args.case,'pair_sha256':sha(args.pair),'records':records,
    'owned_payload_bytes':sum(r['bytes']for r in records.values()if 'file'in r)}
path=destination/'manifest.json';path.write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'manifest':str(path),'sha256':sha(path),'records':len(records),'owned_payload_bytes':manifest['owned_payload_bytes']}))
