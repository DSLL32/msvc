#!/usr/bin/env python3
"""Prepare encountered CL string resources for original RC and native providers."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
from binary import PE

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def extract(identifier):
    pe=PE(ROOT/'toolchains/5.10/bin/CL.EXE')
    root=pe.offset(pe.base+struct.unpack_from('<I',pe.data,pe.optional+112)[0])
    def child(directory,wanted):
        at=root+directory;named,numbered=struct.unpack_from('<HH',pe.data,at+12)
        for i in range(named+numbered):
            name,value=struct.unpack_from('<II',pe.data,at+16+8*i)
            if name==wanted:return value&0x7fffffff
        raise AssertionError('encountered resource absent: '+str(wanted))
    entry=child(child(child(0,6),identifier//16+1),0x409)
    rva,size,codepage,reserved=struct.unpack_from('<4I',pe.data,root+entry)
    offset=pe.offset(pe.base+rva)
    for _ in range(identifier%16):offset+=2+2*struct.unpack_from('<H',pe.data,offset)[0]
    length=struct.unpack_from('<H',pe.data,offset)[0]
    raw=pe.data[offset+2:offset+2+2*length];text=raw.decode('utf-16le')
    text.encode('ascii')
    return {'id':identifier,'language':0x409,'type':6,'record_file_offset':offset,
        'length_utf16':length,'utf16le':raw.hex(),'text':text,'codepage':codepage}


def encounter(identifier,capture):
    capture=Path(capture)
    state=json.loads((capture/'state.json').read_text())
    terminal=state['fields'].get('frontier')=='load_string' and state['fields'].get('parameters',{}).get('id')==identifier
    actual_call=re.search(r'^call sequence=(\d+) id='+str(identifier)+r' module=00400000 output=00409698 capacity=256$',(capture/'resource-events.txt').read_text(),re.M) if (capture/'resource-events.txt').exists() else None
    assert terminal or actual_call,'not an encountered LoadString frontier or actual API call'
    path=PROJECT/'cases/help/resources.json'
    evidence=json.loads(path.read_text()) if path.exists() else {'schema':1,
        'target_sha256':sha(ROOT/'toolchains/5.10/bin/CL.EXE'),'resources':[]}
    assert identifier not in {r['id']for r in evidence['resources']},'resource already defined'
    item=extract(identifier)
    item['encounter']={'state_path':str(capture/'state.json'),'state_sha256':sha(capture/'state.json'),
        'context_sha256':sha(capture/'context.bin'),'memory_sha256':sha(capture/'frontier-memory.json')}
    if actual_call:
        sequence=int(actual_call[1]);before=capture/('resource-%04d-before.bin'%sequence);after=capture/('resource-%04d-after.bin'%sequence)
        initial=before.read_bytes();final=after.read_bytes();text=item['text'].encode('ascii')
        assert re.search(r'^return sequence=%d id=%d value=%d$'%(sequence,identifier,len(text)),(capture/'resource-events.txt').read_text(),re.M)
        assert len(initial)==len(final)==256 and final[:len(text)+1]==text+b'\0' and final[len(text)+1:]==initial[len(text)+1:]
        item['encounter'].update(api_events_sha256=sha(capture/'resource-events.txt'),sequence=sequence,before_sha256=sha(before),after_sha256=sha(after))
    evidence['resources'].append(item)
    path.write_text(json.dumps(evidence,indent=2)+'\n')
    rc='/* Encountered CL diagnostic/help string resources. */\nLANGUAGE 9, 1\nSTRINGTABLE\nBEGIN\n'
    for item in evidence['resources']:rc+='    %d %s\n'%(item['id'],json.dumps(item['text'],ensure_ascii=True))
    (PROJECT/'src/messages.rc').write_text(rc+'END\n')
    print('defined encountered string',identifier,repr(item['text']))


def provider_bytes():
    proof=json.loads((PROJECT/'cases/help/resources.json').read_text())
    assert proof['target_sha256']==sha(ROOT/'toolchains/5.10/bin/CL.EXE')
    literals={int(identifier):json.loads('"'+text+'"') for identifier,text in
        re.findall(r'^\s*(\d+)\s+"((?:[^"\\]|\\.)*)"\s*$',(PROJECT/'src/messages.rc').read_text(),re.M)}
    entries=[]
    for item in proof['resources']:
        assert extract(item['id'])=={key:value for key,value in item.items()if key!='encounter'},'resource source bytes/identity changed'
        assert literals[item['id']]==item['text'],'Win32 resource source differs from encountered payload'
        entries.append({'kind':'string','id':item['id'],'language':item['language'],'text':item['text']})
    assert set(literals)=={e['id']for e in entries},'unencountered resource source entry'
    spec=importlib.util.spec_from_file_location('shared_resource_encoder',ROOT/'lib/recon/resources.py')
    encoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(encoder)
    return encoder.encode_resources(proof['target_sha256'],entries)


def verify_api_capture(directory):
    directory=Path(directory)
    lines=(directory/'resource-events.txt').read_text().splitlines()
    assert len(lines)%2==0,'incomplete resource API observation'
    proof=json.loads((PROJECT/'cases/help/resources.json').read_text())
    entries={e['id']:e for e in proof['resources']};calls=[]
    for n in range(0,len(lines),2):
        call=dict((key,int(value,16) if key in ('module','output') else int(value))
            for key,value in re.findall(r'(\w+)=(\w+)',lines[n]))
        result={key:int(value)for key,value in re.findall(r'(\w+)=(\w+)',lines[n+1])}
        assert lines[n].startswith('call ') and lines[n+1].startswith('return ')
        assert call['sequence']==result['sequence']==n//2+1 and call['id']==result['id']
        assert call['module']==0x400000 and call['output']==0x409698 and call['capacity']==256
        entry=entries[call['id']];text=entry['text'].encode('ascii')
        before=directory/('resource-%04d-before.bin'%call['sequence'])
        after=directory/('resource-%04d-after.bin'%call['sequence'])
        initial=before.read_bytes();final=after.read_bytes()
        assert len(initial)==len(final)==256 and result['value']==len(text)
        assert final[:len(text)+1]==text+b'\0' and final[len(text)+1:]==initial[len(text)+1:]
        calls.append({'call':call,'return':result,'before':{'path':str(before),'sha256':sha(before)},
            'after':{'path':str(after),'sha256':sha(after)}})
    return {'events':{'path':str(directory/'resource-events.txt'),'sha256':sha(directory/'resource-events.txt')},'calls':calls}


def verify_rebuilt():
    # Verify exact contextual .rc payloads in the real rebuilt Win32 artifact.
    pe=PE(PROJECT/'build/cl.exe')
    original=ROOT/'toolchains/5.10/bin/CL.EXE'
    # The extraction helper uses identical PE directory layout on both inputs.
    proof=json.loads((PROJECT/'cases/help/resources.json').read_text())
    root=pe.offset(pe.base+struct.unpack_from('<I',pe.data,pe.optional+112)[0])
    def child(directory,wanted):
        at=root+directory;a,b=struct.unpack_from('<HH',pe.data,at+12)
        for i in range(a+b):
            name,value=struct.unpack_from('<II',pe.data,at+16+8*i)
            if name==wanted:return value&0x7fffffff
        raise AssertionError('rebuilt resource missing '+str(wanted))
    for item in proof['resources']:
        identifier=item['id'];entry=child(child(child(0,6),identifier//16+1),0x409)
        rva,size,codepage,reserved=struct.unpack_from('<4I',pe.data,root+entry);at=pe.offset(pe.base+rva)
        for _ in range(identifier%16):at+=2+2*struct.unpack_from('<H',pe.data,at)[0]
        length=struct.unpack_from('<H',pe.data,at)[0]
        assert pe.data[at+2:at+2+2*length].hex()==item['utf16le'],'Win32 embedded resource payload differs'
    return len(proof['resources'])


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--encounter',type=int);parser.add_argument('--capture',type=Path)
    parser.add_argument('--confirm-api',type=Path)
    parser.add_argument('--prepare',type=Path);parser.add_argument('--verify-rebuilt',action='store_true')
    args=parser.parse_args()
    if args.encounter is not None:encounter(args.encounter,args.capture)
    if args.confirm_api:
        record=verify_api_capture(args.confirm_api)
        (PROJECT/'cases/help/resource-api-evidence.json').write_text(json.dumps(record,indent=2)+'\n')
        print('verified observed bounded LoadStringA calls:',len(record['calls']))
    if args.prepare:args.prepare.parent.mkdir(parents=True,exist_ok=True);args.prepare.write_bytes(provider_bytes())
    if args.verify_rebuilt:print('verified embedded Win32 strings:',verify_rebuilt())
