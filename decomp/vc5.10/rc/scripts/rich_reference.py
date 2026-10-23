"""Frozen rich RC dependency reference derived from the shared original capture."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
from common import HERE,ROOT,read,sha,CRT_SHA

CASE=HERE/'cases/resources-rich'
REFERENCE=CASE/'reference'
ARGUMENTS=['/r','/dCASE_FROM_COMMAND=7','/iinclude','/foresources.res','resources.rc']

def vector(text,phase):
    counts=re.findall(r'^export-'+phase+r'-argc ([0-9]+)$',text,re.M)
    assert counts==['6'],'Incomplete original argv count'
    rows=[]
    for index in range(6):
        pointers=re.findall(r'^export-'+phase+r'-pointer '+str(index)+r' ([0-9a-f]{8})$',text,re.M)
        strings=re.findall(r'^export-'+phase+r'-argv-'+str(index)+r' ([0-9]+) ([0-9a-f]+)$',text,re.M)
        assert len(pointers)==len(strings)==1,'Missing original argv element'
        length,data=strings[0];raw=bytes.fromhex(data)
        assert len(raw)==int(length) and raw.endswith(b'\0') and b'\0' not in raw[:-1]
        rows.append({'pointer':int(pointers[0],16),'bytes':raw.hex(),'text':raw[:-1].decode('ascii')})
    assert re.findall(r'^export-'+phase+r'-terminator ([0-9a-f]{8})$',text,re.M)==['00000000']
    return rows

def boundary(reference=REFERENCE):
    text=(reference/'events.txt').read_text()
    assert not re.search(r'^(?:error|exception|frontier) ',text,re.M),'Original capture did not complete normally'
    assert re.findall(r'^exit ([0-9]+)$',text,re.M)==['0']
    assert re.findall(r'^export-result (-?[0-9]+)$',text,re.M)==['0']
    assert not re.search(r'^callback(?: |-|$)',text,re.M),'This rich reference has no diagnostic callbacks'
    before,after=vector(text,'before'),vector(text,'after')
    assert [row['text'] for row in before[1:]]==ARGUMENTS
    export=re.findall(r'^export-args (.+)$',text,re.M);assert len(export)==1
    slots=export[0].split();assert len(slots)==6 and slots[:5]==['00000000','00000000','01001100','00000000','6']
    main=re.findall(r'^driver-main ([0-9a-f]{8}) caller=([0-9a-f]{8}) argc=6 argv=([0-9a-f]{8}) envp=([0-9a-f]{8})$',text,re.M)
    assert len(main)==1 and int(main[0][0],16)==0x10011d2 and int(main[0][1],16)==0x100131c
    assert int(main[0][2],16)==int(slots[5],16) and int(main[0][3],16)
    assert re.findall(r'^export-caller ([0-9a-f]{8})$',text,re.M)==['010011fd']
    for row in after:
        matches=[]
        for index,original in enumerate(before):
            data=bytes.fromhex(original['bytes']);offset=row['pointer']-original['pointer']
            if 0<=offset<len(data) and data[offset:].hex()==row['bytes']:
                matches.append((index,offset))
        assert len(matches)==1,'Unrecovered post-argv allocation/offset'
        row.update(original_allocation=matches[0][0],offset=matches[0][1])
    recorded=read(reference/'boundary.json')
    assert before==recorded['before'] and after==recorded['after'],'Authored argv boundary differs from raw capture'
    assert recorded['count']==6 and recorded['window']==recorded['unused']==0 and recorded['resource'] is None
    assert recorded['diagnostic_original']==0x1001100 and recorded['driver_main']==0x10011d2 and recorded['original_caller']==0x10011fd
    assert recorded['callbacks']==[] and recorded['result']==0
    cwd=re.findall(r'^process-cwd-utf16 ([0-9]+) ([0-9a-f]+)$',text,re.M);assert len(cwd)==1
    size,encoded=cwd[0];assert len(bytes.fromhex(encoded))==int(size) and encoded==recorded['cwd_utf16']
    cwd_text=bytes.fromhex(encoded).decode('utf-16le');assert '\0' not in cwd_text
    cwd_text.encode('ascii')
    environment=(reference/'environment-utf16.bin').read_bytes()
    assert sha(reference/'environment-utf16.bin')==recorded['environment_utf16_sha256']
    assert re.findall(r'^process-environment-bytes ([0-9]+)$',text,re.M)==[str(len(environment))]
    wide=environment.decode('utf-16le');assert wide.endswith('\0\0')
    strings=wide[:-2].split('\0');assert strings and all(s and '=' in s and not s.startswith('=') for s in strings)
    # Every captured character is ASCII: the narrow projection is lossless. This
    # is the process environment, not a claim that its CRT heap was snapshotted.
    narrow=[s.encode('ascii')+b'\0' for s in strings]
    assert (reference/'stdout.bin').read_bytes()==(reference/'stderr.bin').read_bytes()==b''
    return {'before':before,'after':after,'cwd':cwd_text,'environment':narrow,'trace':['_rc_main']}

def derive(reference=REFERENCE,source=CASE/'source'):
    provider=read(reference/'provider-manifest.json')
    observed=boundary(reference)
    blob=bytearray(b'RCRP\2\0\0\0')
    def word(value):blob.extend(struct.pack('<I',value))
    def data(raw):word(len(raw));blob.extend(raw)
    def string(value):data(value.encode('ascii')+b'\0')
    def vector(values):
        word(len(values))
        for raw in values:data(raw)
    string('resources-rich')
    vector([bytes.fromhex(row['bytes']) for row in observed['before']])
    vector(observed['environment'])
    string(observed['cwd'])
    word(len(provider['source_inputs']))
    for name,pin in sorted(provider['source_inputs'].items()):
        path=source/name;assert path.stat().st_size==pin['bytes'] and sha(path)==pin['sha256'],'Case input changed '+name
        string(name);data(path.read_bytes())
    for row in observed['after']:word(row['original_allocation']);word(row['offset'])
    word(0) # No callback is invented for the quiet successful rich compile.
    word(1);string('resources.res');data((reference/'resources.res').read_bytes())
    word(0)
    return bytes(blob)

def verify():
    config=read(CASE/'case.json');manifest=REFERENCE/'manifest.json'
    assert sha(manifest)==config['reference_manifest_sha256'],'Rich master manifest changed'
    specification=read(manifest)
    for name,pin in specification['files'].items():
        path=REFERENCE/name;assert path.stat().st_size==pin['bytes'] and sha(path)==pin['sha256'],'Frozen rich reference changed '+name
    provider=read(REFERENCE/'provider-manifest.json')
    for name,pin in provider['files'].items():assert sha(REFERENCE/name)==pin,'Shared original reference changed '+name
    assert provider['args']==ARGUMENTS and provider['status']==0
    assert provider['driver_sha256']==sha(ROOT/'toolchains/5.10/bin/RC.EXE')
    assert provider['crt_sha256']==CRT_SHA==sha(ROOT/'toolchains/5.10/bin/MSVCRT.DLL')
    assert provider['target_sha256']==sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL')
    source_names={str(path.relative_to(CASE/'source')) for path in (CASE/'source').rglob('*') if path.is_file()}
    assert source_names==set(provider['source_inputs']),'Unknown/missing rich case input'
    headers=read(REFERENCE/'include-tree.json');base=ROOT/headers['root']
    assert {str(path.relative_to(base)) for path in base.rglob('*') if path.is_file()}==set(headers['files']),'Include tree file set changed'
    for name,pin in headers['files'].items():
        path=base/name;assert path.stat().st_size==pin['bytes'] and sha(path)==pin['sha256'],'Include input changed '+name
    observed=boundary()
    assert observed['cwd'].rstrip('\\')==provider['child_cwd'].rstrip('\\'),'Captured CWD differs'
    assert (REFERENCE/'rcdll-response.bin').read_bytes()==derive(),'Replay bytes differ from raw original effects'
    assert sha(REFERENCE/'resources.res')=='ad4feb23c9a981122246969f9bdaf8bf9a4978cd722f50a67b1e8884835b9318'
    return specification,provider,headers,observed

def freeze(provider):
    assert not REFERENCE.exists(),'Rich reference already exists; never silently recapture/replace'
    original=read(provider/'manifest.json');receipt=read(provider/'receipt.json')
    for name,pin in original['files'].items():assert sha(provider/name)==pin,'Provider file changed '+name
    capture=Path(original['original_capture'])
    for name,expected in [('loaded-driver.bin',original['driver_sha256']),('loaded-crt.bin',CRT_SHA),('loaded-target.bin',original['target_sha256'])]:
        assert sha(capture/name)==expected,'Wrong actual loaded original module '+name
        assert receipt['files'][name]['sha256']==expected,'Receipt/module hash mismatch'
    REFERENCE.mkdir(parents=True)
    shutil.copyfile(provider/'manifest.json',REFERENCE/'provider-manifest.json')
    for name in original['files']:shutil.copyfile(provider/name,REFERENCE/name)
    for name,pin in original['source_inputs'].items():
        actual=capture/'source'/name;assert sha(actual)==pin['sha256']
        destination=CASE/'source'/name;destination.parent.mkdir(parents=True,exist_ok=True)
        if destination.exists():assert destination.read_bytes()==actual.read_bytes(),'Source seed differs from canonical original input'
        else:shutil.copyfile(actual,destination)
    (REFERENCE/'rcdll-response.bin').write_bytes(derive())
    specification={'schema':2,'case':'resources-rich','provider_manifest_sha256':sha(provider/'manifest.json'),
                   'source_owner':'decomp/vc5.10/rcdll/cases/resources-rich','original_capture':str(capture),
                   'environment_projection':'lossless ASCII projection of actual UTF16 process block; not a rich CRT-heap snapshot',
                   'cwd_transport':'native fixture directory is the relative-input root corresponding to the captured Windows CWD',
                   'driver_trace_basis':'actual driver-main010011d2 plus export-caller010011fd; no callback events',
                   'original_driver_sha256':original['driver_sha256'],'original_dll_sha256':original['target_sha256'],
                   'original_crt_sha256':CRT_SHA,'original_seconds':original['original_seconds'],
                   'files':{path.name:{'bytes':path.stat().st_size,'sha256':sha(path)} for path in sorted(REFERENCE.iterdir()) if path.is_file()}}
    (REFERENCE/'manifest.json').write_text(json.dumps(specification,indent=2)+'\n')
    (CASE/'case.json').write_text(json.dumps({'id':'resources-rich','arguments':ARGUMENTS,'reference_manifest_sha256':sha(REFERENCE/'manifest.json'),'driver_functions_added':0,'DLL_coverage_claimed':False},indent=2)+'\n')
    verify()
    return specification

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--freeze-from',type=Path);args=parser.parse_args()
    if args.freeze_from:result=freeze(args.freeze_from)
    else:
        specification,provider,headers,observed=verify();result={'case':'resources-rich','source_files':len(provider['source_inputs']),'conservative_headers':len(headers['files']),'output_bytes':(REFERENCE/'resources.res').stat().st_size,'post_argv_aliases':[(row['original_allocation'],row['offset']) for row in observed['after']]}
    print(json.dumps(result,indent=2))
