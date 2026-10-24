#!/usr/bin/env python3
"""Own an explicit original-only MRE analysis as ordinary files and path/hash refs."""
import argparse,hashlib,json,os,shutil
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
sha=lambda raw:hashlib.sha256(raw).hexdigest()
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('--label',required=True);parser.add_argument('--transport-failure',action='store_true');parser.add_argument('--observed-prefix',action='store_true');args=parser.parse_args()
assert not(args.transport_failure and args.observed_prefix)
assert Path(args.label).name==args.label and args.label not in('.','..')
capture=json.loads((args.directory/'capture.json').read_bytes())
if args.observed_prefix:
 assert capture['case']=='mre-object'and capture['baseline']is False and capture.get('mre_response')is True and capture['helper_status']==0 and capture['controller_error']is None
 kind='Fresh masked original-only MRE object prefix with intermediate file-preservation evidence; no native response, complete lifecycle or application credit'
elif args.transport_failure:
 assert capture['case']=='mre-object'and capture['baseline']is False and capture.get('mre_response')is True
 assert capture['helper_status']==183 or(capture['helper_status']==176 and capture['controller_error']in("AssertionError('unexpected original file-copy request')","BrokenPipeError(32, 'Broken pipe')"))
 kind='Original-only masked MRE file-transport failure; partial observations/bytes are not a complete contract, native success or reconstruction acceptance'
else:
 assert capture['baseline']is True and capture['mre_analysis']is True
 kind='Explicit original-only analysis; no reconstruction acceptance or native response oracle'
case=PROJECT/'cases'/capture['case'];destination=case/'analysis'/args.label
assert not destination.exists();destination.mkdir(parents=True)
originals={}
for p in sorted((ROOT/'toolchains/5.10').rglob('*')):
 if p.is_file():originals.setdefault(sha(p.read_bytes()),p)
records={}
def own(source,name,expected=None):
 raw=source.read_bytes();digest=sha(raw)
 if expected:assert expected=={'bytes':len(raw),'sha256':digest}
 target=destination/name;target.parent.mkdir(parents=True,exist_ok=True)
 record={'bytes':len(raw),'sha256':digest}
 if digest in originals:
  original=originals[digest];target.symlink_to(os.path.relpath(original,target.parent))
  assert not target.readlink().is_absolute()and target.resolve()==original.resolve()and target.resolve().is_relative_to(ROOT/'toolchains/5.10')
  record['repository_original']=str(original.relative_to(ROOT))
 else:target.write_bytes(raw);record['owned_file']=name
 assert sha(target.read_bytes())==digest;records[name]=record
own(args.directory/'capture.json','original/capture.json')
for name,item in capture['files'].items():own(args.directory/name,'original/'+name,item)
for field in('controller_source','helper_source','mre_transport_source'):
 if not capture.get(field):continue
 item=capture[field];own(Path(item['path']),'sources/'+field+'.c'if field=='helper_source'else'sources/'+field+'.py',{'bytes':item['bytes'],'sha256':item['sha256']})
manifest={'schema':1,'kind':kind,
 'case':capture['case'],'historical_source_directory':str(args.directory.resolve()),'capture_sha256':sha((args.directory/'capture.json').read_bytes()),
 'helper_status':capture['helper_status'],'controller_error':capture['controller_error'],'timing':capture['timing'],
 'records':records,'owned_bytes':sum(r['bytes']for r in records.values()if'owned_file'in r),
 'retention_scope':'Exact raw process inputs/observations/helper/controller; metadata native/rebuilt/build-dependency hashes were informational bookkeeping, not inputs to this original execution. Existing repo originals remain relative path+SHA references.'}
p=destination/'manifest.json';p.write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({'manifest':str(p),'sha256':sha(p.read_bytes()),'records':len(records),'owned_bytes':manifest['owned_bytes']}))
