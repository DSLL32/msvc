#!/usr/bin/env python3
"""Explicit original-only DLL capture. Never imported by a native fixture."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('original_tools',HERE/'scripts/wine-tool.py')
tools=importlib.util.module_from_spec(spec);spec.loader.exec_module(tools)
SHA_CRT='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--mode',choices=['baseline','masked'],required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--timeout',type=float,default=45);p.add_argument('--case',choices=['help','resources-rich'],default='help')
    p.add_argument('--heap-observations',type=int,default=1);p.add_argument('--free-observations',type=int,default=0);p.add_argument('--character-observations',type=int,default=0);p.add_argument('--api-census',action='store_true')
    a=p.parse_args();out=a.output.resolve()
    limits=[a.heap_observations,a.free_observations,a.character_observations]
    if a.api_census:limits=[8192,8192,8192]
    if not all(0<=value<=8192 for value in limits):p.error('observer limits must be 0..8192')
    out.relative_to(Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')
    if out.exists():p.error('output must be a new explicit scratch directory')
    out.mkdir(parents=True)
    target_bytes=(ROOT/'toolchains/5.10/bin/RCDLL.DLL').read_bytes()
    pe=struct.unpack_from('<I',target_bytes,0x3c)[0];opt=pe+24
    assert struct.unpack_from('<II',target_bytes,opt+96+9*8)==(0,0),'TLS unsupported'
    sections=[]
    for n in range(struct.unpack_from('<H',target_bytes,pe+6)[0]):
        q=opt+struct.unpack_from('<H',target_bytes,pe+20)[0]+n*40
        sections.append(struct.unpack_from('<IIII',target_bytes,q+8))
    def offset(rva):
        for vs,start,size,raw in sections:
            if start<=rva<start+size:return raw+rva-start
        raise ValueError('unbacked RVA')
    reloc,size=struct.unpack_from('<II',target_bytes,opt+96+5*8)
    at,end=offset(reloc),offset(reloc)+size
    relocation_sites=[]
    while at<end:
        page,length=struct.unpack_from('<II',target_bytes,at)
        assert length>=8 and at+length<=end
        for word in struct.unpack_from('<%dH'%((length-8)//2),target_bytes,at+8):
            kind,delta=word>>12,word&0xfff
            if not kind:continue
            assert kind==3
            site=page+delta
            assert not site<=0x12d20<site+4,'entry guard overlaps relocation'
            relocation_sites.append(site)
        at+=length
    assert target_bytes[offset(0x12d20)]==0x53
    tc=ROOT/'toolchains/5.10/bin'
    assert sha(tc/'MSVCRT.DLL')==SHA_CRT
    for name in ['RC.EXE','RCDLL.DLL','MSVCRT.DLL']:
        shutil.copyfile(tc/name,out/name)
    shutil.copyfile(HERE/'build/debug-original.exe',out/'debug-original.exe')
    shutil.copyfile(HERE/'build/debug-original-source.json',out/'helper-compiled-source.json')
    case_args=['/help'];child_cwd=None
    if a.case=='resources-rich':
        case_args=['/r','/dCASE_FROM_COMMAND=7','/iinclude','/foresources.res','resources.rc']
        shutil.copytree(HERE/'cases/resources-rich/source',out/'source');child_cwd=tools.windows(out/'source')
        (out/'case-args.txt').write_text(subprocess.list2cmdline(case_args))
    from protocol import candidate,validation_versions
    (out/'candidate.json').write_text(json.dumps(candidate(),indent=2)+'\n')
    (out/'validation-versions.json').write_text(json.dumps(validation_versions(),indent=2)+'\n')
    config=json.loads((HERE/'functions.json').read_text())
    ranges=[]
    for f in config['functions']:
        if f['name'] in config['accepted']+[config.get('trial_function')]:
            ranges.append('%x %x 1\n'%(int(f['start'],16),int(f['end'],16)))
            for r in f.get('text_data',[]):ranges.append('%x %x 0\n'%(int(r[0],16),int(r[1],16)))
    for f in config.get('support_imports',[]):
        if f['accepted']:ranges.append('%x %x 1\n'%(int(f['start'],16),int(f['end'],16)))
    (out/'ranges.txt').write_text(''.join(ranges))
    shutil.copyfile(HERE/'build/original-source.json',out/'compiled-source.json')
    (out/'functions.json').write_bytes((HERE/'functions.json').read_bytes())
    env=tools.environment()
    if a.case=='resources-rich':
        # Freeze a small public launcher environment; the actual Windows child
        # block is captured from its PEB, rather than assuming Wine translation.
        keep={'PATH','HOME','USER','LOGNAME','LANG','LC_ALL','LC_CTYPE','DISPLAY',
              'XAUTHORITY','WINEPREFIX','WINEARCH','WINEDEBUG','WINESERVER',
              'WINEDLLOVERRIDES','INCLUDE','LIB','WINEPATH'}
        env={key:value for key,value in env.items() if key in keep}
    env.update({key:str(value)for key,value in zip(['RCDLL_CAPTURE_HEAP_LIMIT','RCDLL_CAPTURE_FREE_LIMIT','RCDLL_CAPTURE_CHARACTER_LIMIT'],limits)})
    command=['wine',str(out/'debug-original.exe'),a.mode,tools.windows(out/'RC.EXE'),'ranges.txt']
    if child_cwd:command += ['case-args.txt',child_cwd]
    started=time.perf_counter()
    with (out/'stdout.bin').open('wb') as stdout,(out/'stderr.bin').open('wb') as stderr:
        try:
            result=subprocess.run(command,cwd=out,env=env,stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr,timeout=a.timeout)
            status=result.returncode;timed_out=False
        except subprocess.TimeoutExpired:
            status=None;timed_out=True
    elapsed=time.perf_counter()-started
    report=dict(preflight=dict(tls=False,entry_guard_nonrelocated=True,relocation_sites=len(relocation_sites),loader_data_ranges=[['0x1000','0x114c'],['0x12e2c','0x13371']],import_cycle_scope='target has only canonical MSVCRT and Wine system KERNEL32/USER32 imports; canonical MSVCRT imports no RCDLL'),case=a.case,args=case_args,child_cwd=child_cwd,environment={key:env[key]for key in ['INCLUDE','LIB','WINEPATH']},mode=a.mode,command=command,status=status,timed_out=timed_out,elapsed_seconds=elapsed,wineprefix=env['WINEPREFIX'],wine_override=env['WINEDLLOVERRIDES'],argv0=tools.windows(out/'RC.EXE'),target_sha256=sha(tc/'RCDLL.DLL'),crt_expected_sha256=SHA_CRT,candidate_sha256=sha(HERE/'functions.json'),debugger_sha256=sha(out/'debug-original.exe'),files={})
    report['observation_limits']=dict(zip(['heap','free','character'],limits));report['api_census']=a.api_census
    for name in ['helper-compiled-source.json','case-args.txt','environment-utf16.bin','compiled-source.json','candidate.json','functions.json','ranges.txt','validation-versions.json','stdout.bin','stderr.bin','events.txt','relocated-text.bin','masked-text.bin','globals.bin','completion-globals.bin','loaded-target.bin','loaded-crt.bin','loaded-driver.bin']:
        f=out/name
        if f.exists():report['files'][name]=dict(bytes=f.stat().st_size,sha256=sha(f))
    if a.case=='resources-rich':
        report['case_files']={str(f.relative_to(out/'source')):dict(bytes=f.stat().st_size,sha256=sha(f))for f in (out/'source').rglob('*')if f.is_file()}
    report['actual_target_verified']=report['files'].get('loaded-target.bin',{}).get('sha256')==report['target_sha256']
    report['actual_crt_verified']=report['files'].get('loaded-crt.bin',{}).get('sha256')==SHA_CRT
    (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    if timed_out or not report['actual_target_verified'] or not report['actual_crt_verified']:return 125
    return 0 if status in [0,124] else status
if __name__=='__main__':raise SystemExit(main())
