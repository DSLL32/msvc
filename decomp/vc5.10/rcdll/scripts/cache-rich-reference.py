#!/usr/bin/env python3
"""Freeze explicit original rich resource evidence; never run by fixtures."""
import argparse
import json
from pathlib import Path
import re
import shutil
from protocol import HERE,ROOT,sha,callbacks

p=argparse.ArgumentParser();p.add_argument('capture',type=Path);p.add_argument('--case',choices=['resources-rich','resources-rich-trailing-space'],default='resources-rich')
a=p.parse_args();capture=a.capture.resolve();out=HERE/('cases/'+a.case+'/reference')
assert not out.exists(),'reference is immutable; choose an explicit new capture directory/version'
r=json.loads((capture/'receipt.json').read_text())
assert r['case']==a.case and r['mode']=='baseline'
assert r['status']==0 and not r['timed_out']
for name,info in r['files'].items():
    f=capture/name
    assert f.stat().st_size==info['bytes'] and sha(f)==info['sha256'],name
for name,target in [('loaded-target.bin','RCDLL.DLL'),('loaded-crt.bin','MSVCRT.DLL'),('loaded-driver.bin','RC.EXE')]:
    assert sha(capture/name)==sha(ROOT/'toolchains/5.10/bin'/target),name
assert not (capture/'stdout.bin').read_bytes() and not (capture/'stderr.bin').read_bytes()
assert callbacks(capture)==[]
text=(capture/'events.txt').read_text()
assert re.findall(r'^lifecycle-return (\d+) (\d+)$',text,re.M)==[('1','1'),('0','1')]
assert re.findall(r'^export-result (\d+)$',text,re.M)==['0']
assert re.findall(r'^exit (\d+)$',text,re.M)==['0']
assert re.findall(r'^export-caller ([a-f0-9]+)$',text,re.M)==['010011fd']
assert re.findall(r'^driver-main ([a-f0-9]+) ',text,re.M)==['010011d2']
module=re.findall(r'^module-path target (.+)$',text,re.M);assert len(module)==1
cwd=re.findall(r'^process-cwd-utf16 (\d+) ([a-f0-9]+)$',text,re.M);assert len(cwd)==1
cwd_raw=bytes.fromhex(cwd[0][1]);assert len(cwd_raw)==int(cwd[0][0])
assert cwd_raw.decode('utf-16le').rstrip('\\')==r['child_cwd']

def vector(stage):
    entries=re.findall(r'^export-'+stage+r'-argv-(\d+) (\d+) ([a-f0-9]+)$',text,re.M)
    pointers=[int(n,16) for n in re.findall(r'^export-'+stage+r'-pointer \d+ ([a-f0-9]+)$',text,re.M)]
    assert len(entries)==len(pointers)==6
    values=[]
    for index,((n,size,raw),pointer) in enumerate(zip(entries,pointers)):
        value=bytes.fromhex(raw)
        assert int(n)==index and len(value)==int(size) and value[-1:]==b'\0' and b'\0'not in value[:-1]
        values.append(dict(pointer=pointer,text=value[:-1].decode('ascii'),bytes=value.hex()))
    assert re.findall(r'^export-'+stage+r'-terminator ([a-f0-9]+)$',text,re.M)==['00000000']
    return values
before,after=vector('before'),vector('after')
assert [v['text']for v in before]==[r['argv0']]+r['args']
# argv slots can alias a different original allocation after DLL option parsing.
for value in after:
    matches=[(i,value['pointer']-old['pointer'])for i,old in enumerate(before)
             if old['pointer']<=value['pointer']<old['pointer']+len(bytes.fromhex(old['bytes']))]
    assert len(matches)==1
    value['original_allocation'],value['offset']=matches[0]
for name,info in r['case_files'].items():
    f=capture/'source'/name
    assert f.stat().st_size==info['bytes'] and sha(f)==info['sha256']
    if name!='resources.res':assert f.read_bytes()==(HERE/'cases/resources-rich/source'/name).read_bytes()
environment=(capture/'environment-utf16.bin').read_bytes()
assert environment.endswith(b'\0\0\0\0') and not len(environment)%2
out.mkdir(parents=True)
for name in ['receipt.json','events.txt','stdout.bin','stderr.bin','environment-utf16.bin','case-args.txt','helper-compiled-source.json','compiled-source.json']:
    shutil.copyfile(capture/name,out/name)
shutil.copyfile(capture/'source/resources.res',out/'resources.res')
(out/'callbacks.json').write_text(json.dumps(callbacks(capture),indent=2)+'\n')
headers={str(f.relative_to(ROOT/'toolchains/5.10/include')):dict(bytes=f.stat().st_size,sha256=sha(f))
         for f in sorted((ROOT/'toolchains/5.10/include').rglob('*'))if f.is_file()}
(out/'include-tree.json').write_text(json.dumps(dict(scope='complete conservative input tree, not a claim every header was read',root='toolchains/5.10/include',files=headers),indent=2)+'\n')
boundary=dict(export='RC',abi='stdcall',slots=6,window=0,unused=0,diagnostic_original=0x01001100,
              resource=None,count=6,before=before,after=after,result=0,callbacks=[],
              original_caller=0x010011fd,driver_main=0x010011d2,cwd_utf16=cwd_raw.hex(),
              environment_utf16_sha256=sha(out/'environment-utf16.bin'))
(out/'boundary.json').write_text(json.dumps(boundary,indent=2)+'\n')
manifest=dict(schema=1,case=a.case,status=0,args=r['args'],argv0=r['argv0'],
              child_cwd=r['child_cwd'],module_path=module[0],original_capture=str(capture),
              target_sha256=r['target_sha256'],crt_sha256=r['crt_expected_sha256'],
              driver_sha256=sha(capture/'loaded-driver.bin'),original_seconds=r['elapsed_seconds'],
              source_inputs={name:info for name,info in r['case_files'].items()if name!='resources.res'},
              files={str(f.relative_to(out)):sha(f)for f in sorted(out.rglob('*'))if f.is_file()},
              trace_state='baseline boundary only; masked reconstruction trace will be recorded on completion')
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(dict(reference=str(out),res_bytes=(out/'resources.res').stat().st_size,
                     res_sha256=sha(out/'resources.res'),headers=len(headers),callbacks=0),indent=2))
