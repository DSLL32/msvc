"""Frozen missing-percent source dependency, derived from actual original events."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
from common import HERE,ROOT,CRT_SHA,read,sha,hex_records
from rich_reference import vector

CASE=HERE/'cases/missing-percent-source'
REFERENCE=CASE/'reference'
ARGUMENTS=['/r','missing%source.rc']
ABSENT=['missing%source.rc','missing%source.res']

def boundary(reference=REFERENCE):
    text=(reference/'events.txt').read_text()
    assert not re.search(r'^(?:error|exception|frontier) ',text,re.M)
    assert re.findall(r'^exit ([0-9]+)$',text,re.M)==['1']
    assert re.findall(r'^export-result (-?[0-9]+)$',text,re.M)==['1']
    before,after=vector(text,'before',3),vector(text,'after',3)
    assert [row['text'] for row in before[1:]]==ARGUMENTS
    assert before==after,'This error did not alter its argv pointers or storage'
    for index,row in enumerate(after):row.update(original_allocation=index,offset=0)
    slots=re.findall(r'^export-args (.+)$',text,re.M);assert len(slots)==1
    slots=slots[0].split();assert len(slots)==6 and slots[:5]==['00000000','00000000','01001100','00000000','3']
    main=re.findall(r'^driver-main ([0-9a-f]{8}) caller=([0-9a-f]{8}) argc=3 argv=([0-9a-f]{8}) envp=([0-9a-f]{8})$',text,re.M)
    assert len(main)==1 and main[0][:2]==('010011d2','0100131c')
    assert int(main[0][2],16)==int(slots[5],16) and int(main[0][3],16)
    assert re.findall(r'^export-caller ([0-9a-f]{8})$',text,re.M)==['010011fd']
    arguments=re.findall(r'^callback ([0-9]+) ([0-9]+)$',text,re.M)
    returns=re.findall(r'^callback-return (-?[0-9]+)$',text,re.M)
    initial=hex_records(text,'before');final=hex_records(text,'after')
    assert arguments==[('0','0')]*3 and returns==['0']*3 and len(initial)==len(final)==3
    callbacks=[]
    for first,last in zip(initial,final):
        a,b=bytes.fromhex(first),bytes.fromhex(last);assert len(a)==len(b) and a.endswith(b'\0')
        callbacks.append({'window':0,'code':0,'before':first,'after':last,'result':0})
    assert b'missing%source.rc' in bytes.fromhex(callbacks[1]['before'])
    assert b'missing%source.rc' in bytes.fromhex(callbacks[1]['after']),'Actual percent allocation succeeded, rather than replacing percent with a space'
    expected=b''.join(bytes.fromhex(row['after']).split(b'\0')[0]+b'\n' for row in callbacks).replace(b'\n',b'\r\n')
    assert (reference/'stdout.bin').read_bytes()==expected and len(expected)==68
    assert (reference/'stderr.bin').read_bytes()==b''
    cwd=re.findall(r'^process-cwd-utf16 ([0-9]+) ([0-9a-f]+)$',text,re.M);assert len(cwd)==1
    count,data=cwd[0];assert len(bytes.fromhex(data))==int(count)
    cwd=bytes.fromhex(data).decode('utf-16le');cwd.encode('ascii');assert '\0' not in cwd
    environment=(reference/'environment-utf16.bin').read_bytes();wide=environment.decode('utf-16le');assert wide.endswith('\0\0')
    assert re.findall(r'^process-environment-bytes ([0-9]+)$',text,re.M)==[str(len(environment))]
    values=wide[:-2].split('\0');assert values and all(value and '=' in value and not value.startswith('=') for value in values)
    return {'before':before,'after':after,'callbacks':callbacks,'cwd':cwd,'environment':[value.encode('ascii')+b'\0' for value in values],'trace':['_rc_main','_rc_diagnostic@12'],'status':1}

def derive(reference=REFERENCE):
    observed=boundary(reference)
    blob=bytearray(b'RCRP\3\0\0\0')
    def word(value):blob.extend(struct.pack('<I',value))
    def data(value):word(len(value));blob.extend(value)
    def string(value):data(value.encode('ascii')+b'\0')
    def vector(values):
        word(len(values))
        for value in values:data(value)
    string('missing-percent-source');vector([bytes.fromhex(row['bytes']) for row in observed['before']]);vector(observed['environment']);string(observed['cwd'])
    word(0);word(2)
    for name in ABSENT:string(name)
    for index in range(3):word(index);word(0)
    word(3)
    for row in observed['callbacks']:
        a,b=bytes.fromhex(row['before']),bytes.fromhex(row['after'])
        for value in [0,0,len(a),0]:word(value)
        blob.extend(a);blob.extend(b)
    word(0);word(1)
    return bytes(blob)

def verify():
    config=read(CASE/'case.json');assert sha(REFERENCE/'manifest.json')==config['reference_manifest_sha256']
    manifest=read(REFERENCE/'manifest.json')
    for name,pin in manifest['files'].items():
        path=REFERENCE/name;assert path.stat().st_size==pin['bytes'] and sha(path)==pin['sha256'],'Error reference changed '+name
    for path,pin in manifest['modules'].items():assert sha(ROOT/path)==pin,'Original module changed '+path
    observed=boundary()
    assert read(REFERENCE/'callbacks.json')==observed['callbacks'],'Callback cache differs from actual raw buffers'
    assert (REFERENCE/'rcdll-response.bin').read_bytes()==derive(),'Error response not derived from raw original events'
    assert read(REFERENCE/'receipt.json')['final_case_files']=={},'Original error left no artifact'
    return manifest,observed

def freeze(capture):
    assert not REFERENCE.exists(),'Never silently replace a frozen original case'
    receipt=read(capture/'receipt.json');assert receipt['status']==1 and receipt['final_case_files']=={}
    modules={}
    for name,module in [('loaded-driver.bin','RC.EXE'),('loaded-target.bin','RCDLL.DLL'),('loaded-crt.bin','MSVCRT.DLL')]:
        path='toolchains/5.10/bin/'+module;expected=sha(ROOT/path);assert sha(capture/name)==expected and receipt['files'][name]['sha256']==expected
        modules[path]=expected
    assert modules['toolchains/5.10/bin/MSVCRT.DLL']==CRT_SHA
    REFERENCE.mkdir(parents=True)
    for name in ['events.txt','environment-utf16.bin','stdout.bin','stderr.bin','receipt.json','helper-compiled-source.json','case-args.txt']:
        if name!='receipt.json':assert sha(capture/name)==receipt['files'][name]['sha256']
        shutil.copyfile(capture/name,REFERENCE/name)
    observed=boundary()
    (REFERENCE/'callbacks.json').write_text(json.dumps(observed['callbacks'],indent=2)+'\n')
    (REFERENCE/'rcdll-response.bin').write_bytes(derive())
    manifest={'schema':3,'case':'missing-percent-source','original_capture':str(capture),'original_seconds':receipt['elapsed_seconds'],'args':ARGUMENTS,'absent_inputs_and_output':ABSENT,'modules':modules,'driver_trace_basis':'actual driver-main010011d2 and callback events at the observed01001100 callback; no DLL trace substitution','environment_projection':'lossless ASCII from actual UTF16 process environment; no separate CRT-heap claim','files':{path.name:{'bytes':path.stat().st_size,'sha256':sha(path)} for path in REFERENCE.iterdir() if path.is_file()}}
    (REFERENCE/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (CASE/'case.json').write_text(json.dumps({'id':'missing-percent-source','arguments':ARGUMENTS,'expected_status':1,'reference_manifest_sha256':sha(REFERENCE/'manifest.json'),'driver_functions_added':0,'DLL_coverage_claimed':False},indent=2)+'\n')
    verify();return manifest

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--freeze-from',type=Path);args=parser.parse_args()
    if args.freeze_from:result=freeze(args.freeze_from)
    else:
        manifest,observed=verify();result={'status':1,'callbacks':len(observed['callbacks']),'stdout_bytes':68,'artifacts':0}
    print(json.dumps(result,indent=2))
