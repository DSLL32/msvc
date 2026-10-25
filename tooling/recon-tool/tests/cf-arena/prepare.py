#!/usr/bin/env python3
"""Build-only authored PE/CF fixture preparation; never executes either child.

PE section/relocation and public-MAP extraction here generates fixture inputs.
Instruction discovery/export belongs exclusively to the shared Rust auditor.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
TC=ROOT/'toolchains/5.10'
SCRATCH=Path.home()/'.ai-tmp/msvc2'
OWNERS=['probe_leaf','probe_unknown','probe_jump_direct','probe_jump_indirect',
        'probe_forged_return','probe_prefix','probe_bad_return_prefix',
        'probe_unknown_indirect_call','probe_unknown_indirect_jump']


def sha(data): return hashlib.sha256(data).hexdigest()
def file_sha(path): return sha(Path(path).read_bytes())
def write_json(path,value): path.write_text(json.dumps(value,indent=2)+'\n')
def windows(path):
    path=Path(path).resolve()
    for root,drive in ((ROOT,'R'),(SCRATCH,'S')):
        if path.is_relative_to(root):
            return drive+':'+chr(92)+str(path.relative_to(root)).replace('/',chr(92))
    raise ValueError('unmapped path: '+str(path))


def pe(path):
    data=path.read_bytes()
    u16=lambda at:struct.unpack_from('<H',data,at)[0]
    u32=lambda at:struct.unpack_from('<I',data,at)[0]
    nt=u32(60);assert data[:2]==b'MZ' and data[nt:nt+4]==b'PE\0\0'
    assert u16(nt+4)==0x14c
    optional=nt+24;assert u16(optional)==0x10b
    base=u32(optional+28);size=u32(optional+56);headers=u32(optional+60)
    assert 0<headers<=size<=1024*1024
    virtual=bytearray(size);virtual[:headers]=data[:headers]
    sections=[]
    for index in range(u16(nt+6)):
        at=optional+u16(nt+20)+index*40
        name=data[at:at+8].split(b'\0',1)[0].decode('ascii')
        length,rva,raw_size,raw_offset=struct.unpack_from('<4I',data,at+8)
        assert rva+max(length,raw_size)<=size and raw_offset+raw_size<=len(data)
        virtual[rva:rva+raw_size]=data[raw_offset:raw_offset+raw_size]
        sections.append(dict(name=name,rva=rva,virtual_size=length,raw_size=raw_size,
                             raw_offset=raw_offset,characteristics=u32(at+36)))
    reloc_rva,reloc_size=struct.unpack_from('<2I',data,optional+96+5*8)
    fixups=[];cursor=reloc_rva
    while cursor<reloc_rva+reloc_size:
        page,block=struct.unpack_from('<2I',virtual,cursor)
        assert block>=8 and block%2==0 and cursor+block<=reloc_rva+reloc_size
        for offset in range(cursor+8,cursor+block,2):
            item=struct.unpack_from('<H',virtual,offset)[0]
            if item>>12==0:continue
            assert item>>12==3,'only actual i386 HIGHLOW fixups are supported'
            fixups.append(page+(item&4095))
        cursor+=block
    assert cursor==reloc_rva+reloc_size
    fixups.sort()
    assert fixups and all(a+4<=b for a,b in zip(fixups,fixups[1:]))
    return dict(data=data,virtual=virtual,base=base,size=size,entry=u32(optional+16),
                fixups=fixups,sections=sections)


def publics(path):
    result={}
    for line in path.read_text().splitlines():
        match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if match:result[match[1]]=int(match[2],16)
    return result


def group_stream(fixups):
    groups=[]
    for offset in fixups:
        if not groups or offset-groups[-1][-1]>65535 or len(groups[-1])==65535:
            groups.append([])
        groups[-1].append(offset)
    raw=bytearray()
    for group in groups:
        raw.extend(struct.pack('<3H',len(group),group[0]>>16,group[0]&65535))
        for old,new in zip(group,group[1:]):raw.extend(struct.pack('<H',new-old))
    raw.extend(b'\0\0')
    return bytes(raw),[dict(count=len(g),first=g[0],last=g[-1]) for g in groups]


def config(target,image,records,accepted):
    return dict(authored_fixture_only=True,application_accepted_functions=0,
                target=dict(path=str(target.relative_to(ROOT)),sha256=file_sha(target),**({'image':image} if image else {})),
                driver=records,accepted=accepted)


def export(project,cfg,rebuilt,map_path,binary,output,records,check_ret=True):
    project.mkdir(exist_ok=True)
    write_json(project/'functions.json',cfg)
    write_json(project/'runtime.json',[])
    write_json(project/'metadata.json',dict(schema=1,functions={}))
    command=[str(binary),'--profile','cl','--discovery','cfg','--repo',str(ROOT),
             '--project',str(project),'--config',str(project/'functions.json'),
             '--runtime',str(project/'runtime.json'),'--rebuilt-runtime',str(project/'runtime.json'),
             '--metadata',str(project/'metadata.json'),'--rebuilt',str(rebuilt),
             '--map',str(map_path),'--check-ownership','--check-extents',
             '--export-instructions','--report',str(output)]
    if check_ret:command.append('--check-ret')
    write_json(project/'command.json',command)
    started=time.perf_counter()
    result=subprocess.run(command,capture_output=True,timeout=30)
    (project/'audit.stdout').write_bytes(result.stdout)
    (project/'audit.stderr').write_bytes(result.stderr)
    records.append(dict(operation='static-audit',command=command,status=result.returncode,
                        seconds=time.perf_counter()-started))
    if result.returncode:raise RuntimeError(result.stderr.decode('utf-8','replace'))
    return json.loads(result.stdout)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-only',action='store_true')
    parser.add_argument('--auditor',type=Path,default=ROOT/'tooling/graph-audit/target/release/graph-audit')
    parser.add_argument('--reuse-loader-from',type=Path)
    parser.add_argument('--reuse-payload-from',type=Path)
    args=parser.parse_args()
    if not args.build_only:parser.error('only explicit --build-only is implemented; no child execution exists')
    args.auditor=args.auditor.resolve()
    scratch_parent=SCRATCH/'recon-tool-cf-fixtures';scratch_parent.mkdir(parents=True,exist_ok=True)
    work=Path(tempfile.mkdtemp(prefix='cf-arena-build-',dir=scratch_parent))
    env=os.environ.copy()
    runner=ROOT/'tooling/recon-tool/target/i686-unknown-linux-gnu/release/recon-tool'
    for key in ('CL','_CL_','LINK','_LINK_','INCLUDE'):env.pop(key,None)
    records=[]
    receipt=dict(scope='build/static audit only; authored fixture, no child/original runtime or application acceptance',
                 work=str(work),sources={p.name:file_sha(p) for p in (HERE/'arena-loader.c',HERE/'arena-payload.c',Path(__file__))},
                 tools={str(p.relative_to(ROOT)):file_sha(p) for p in (TC/'bin/CL.EXE',TC/'bin/C1.DLL',TC/'bin/C2.EXE',TC/'bin/LINK.EXE',TC/'lib/KERNEL32.LIB',args.auditor)},
                 operations=records,child_launches=0)
    def save():write_json(work/'receipt.json',receipt)
    save();print(work,flush=True)
    def run(name,command):
        started=time.perf_counter()
        result=subprocess.run(command,env=env,cwd=work,input=b'',capture_output=True,timeout=30)
        (work/(name+'.stdout')).write_bytes(result.stdout)
        (work/(name+'.stderr')).write_bytes(result.stderr)
        records.append(dict(operation=name,command=command,status=result.returncode,seconds=time.perf_counter()-started))
        save()
        if result.returncode:raise RuntimeError(name+' failed; retained '+str(work))
    for name,entry in (('payload','probe_prefix'),('loader','probe_loader_entry')):
        source=HERE/('arena-'+name+'.c')
        reuse=args.reuse_loader_from if name=='loader' else args.reuse_payload_from
        if reuse:
            old=reuse.resolve();prior=json.loads((old/'receipt.json').read_text())
            assert prior['sources'][source.name]==file_sha(source)
            for path in ('bin/CL.EXE','bin/C1.DLL','bin/C2.EXE','bin/LINK.EXE','lib/KERNEL32.LIB'):
                assert prior['tools'][str((TC/path).relative_to(ROOT))]==file_sha(TC/path)
            for extension in ('.exe','.map'):
                shutil.copyfile(old/(name+extension),work/(name+extension))
            records.append(dict(operation=name+'-build-reused',receipt=str(old/'receipt.json'),
                                receipt_sha256=file_sha(old/'receipt.json'),source_sha256=file_sha(source),
                                pe_sha256=file_sha(work/(name+'.exe')),map_sha256=file_sha(work/(name+'.map')),rerun=False))
            save();continue
        run(name+'-compile',[str(runner),'original','tool','--repo',str(ROOT),'5.10','CL.EXE','--','/nologo','/c','/TC','/Od','/Ob0','/Gd',
             '/I'+windows(TC/'include'),'/Fo'+windows(work/(name+'.obj')),windows(source)])
        command=[str(runner),'original','tool','--repo',str(ROOT),'5.10','LINK.EXE','--','/nologo','/nodefaultlib','/incremental:no',
                 '/subsystem:console','/entry:'+entry,'/base:0x400000','/fixed:no','/opt:noref',
                 '/out:'+windows(work/(name+'.exe')),'/map:'+windows(work/(name+'.map')),windows(work/(name+'.obj'))]
        if name=='loader':command.append(windows(TC/'lib/KERNEL32.LIB'))
        run(name+'-link',command)
    for name in ('payload.exe','payload.map','loader.exe','loader.map'):
        shutil.copyfile(work/name,HERE/name)
    payload=pe(HERE/'payload.exe');loader=pe(HERE/'loader.exe')
    pm=publics(HERE/'payload.map');lm=publics(HERE/'loader.map')
    initialized=bytearray(payload['virtual'])
    for offset in payload['fixups']:
        value=struct.unpack_from('<I',initialized,offset)[0]
        assert payload['base']<=value<payload['base']+payload['size'],hex(value)
        struct.pack_into('<I',initialized,offset,value-payload['base'])
    stream,groups=group_stream(payload['fixups'])
    relocation=len(initialized);initialized.extend(stream)
    allocation=(len(initialized)+4095)&~4095
    entry=pm['_probe_prefix']-payload['base']
    container=bytearray(128);container[:2]=b'MZ';struct.pack_into('<H',container,8,4)
    fields=[0x4643,128,len(initialized),relocation,allocation,entry]
    struct.pack_into('<6I',container,64,*fields);container.extend(initialized)
    (HERE/'payload.cf').write_bytes(container)
    function_bounds={}
    for name in OWNERS:
        start=pm['_'+name];end=min(value for value in pm.values() if value>start)
        function_bounds[name]=dict(start=start-payload['base'],end=end-payload['base'],symbol='_'+name)
    layout=dict(scope='authored PE initialized virtual image; HIGHLOW addends normalized by actual ImageBase only; no synthesized CFG',
                source=dict(path='arena-payload.c',sha256=file_sha(HERE/'arena-payload.c')),
                pe=dict(path='payload.exe',sha256=file_sha(HERE/'payload.exe'),image_base=payload['base'],size_of_image=payload['size'],sections=payload['sections']),
                map=dict(path='payload.map',sha256=file_sha(HERE/'payload.map')),
                normalized_fixup_offsets=payload['fixups'],groups=groups,functions=function_bounds,
                marker=dict(rva=pm['_probe_marker']-payload['base'],size=4,type='volatile DWORD',initial_value=0,write=True),
                pointer=dict(rva=pm['_probe_pointer']-payload['base'],size=4,type='volatile near cdecl function pointer',
                             target_rva=pm['_probe_leaf']-payload['base'],write=False),
                unknown_pointer=dict(rva=pm['_probe_unknown_pointer']-payload['base'],size=4,type='volatile near cdecl function pointer',
                             target_rva=pm['_probe_unknown']-payload['base'],write=False),
                tail_abi=dict(scope='authored instrumentation only; not production audit permission',
                    helpers=['probe_jump_direct','probe_jump_indirect','probe_unknown_indirect_jump'],
                    contract='near JMP preserves live caller frame; admitted leaf ends in RET0; unknown destination is a frontier before body',
                    evidence=dict(path='arena-payload.c',sha256=file_sha(HERE/'arena-payload.c')),
                    residual='unresolved indirect-tail owners have local_ret_cleanup_set=[] and checked_ret_cleanup_set=null in full export; never checked0'))
    write_json(HERE/'fixture-layout.json',layout)
    records_cf=[dict(name='_'+name,start=row['start'],end=row['end']) for name,row in function_bounds.items()]
    image=dict(format='watcom-cf',coordinates='image-offset',analysis_base=0,
               initialized_sha256=sha(initialized),relocation_sha256=sha(stream),
               loader=dict(path=str((HERE/'loader.exe').relative_to(ROOT)),sha256=file_sha(HERE/'loader.exe')),
               layout_evidence=dict(path='fixture-layout.json',sha256=file_sha(HERE/'fixture-layout.json')))
    # The fixture project is HERE so the exact layout witness stays local.
    cfg=config(HERE/'payload.cf',image,records_cf,[r['name'] for r in records_cf])
    write_json(HERE/'functions.json',cfg)
    write_json(HERE/'runtime.json',[]);write_json(HERE/'metadata.json',dict(schema=1,functions={}))
    export(HERE,cfg,HERE/'payload.exe',HERE/'payload.map',args.auditor,HERE/'payload-export.json',records,check_ret=False)
    ret_cfg=dict(cfg)
    ret_cfg['accepted']=['_probe_leaf','_probe_unknown','_probe_jump_direct','_probe_forged_return']
    # This nested project's CF witness remains explicitly contained locally.
    ret_cfg=json.loads(json.dumps(ret_cfg));ret_cfg['target']['image']['layout_evidence']['path']='fixture-layout.json'
    ret_project=HERE/'ret-audit';ret_project.mkdir(exist_ok=True)
    shutil.copyfile(HERE/'fixture-layout.json',ret_project/'fixture-layout.json')
    export(ret_project,ret_cfg,HERE/'payload.exe',HERE/'payload.map',args.auditor,HERE/'payload-ret-export.json',records)
    ready=lm['_probe_ready'];ready_end=min(v for v in lm.values() if v>ready)
    ready_cfg=config(HERE/'loader.exe',None,[dict(name='_probe_ready',start=ready,end=ready_end)],['_probe_ready'])
    export(HERE/'ready-audit',ready_cfg,HERE/'loader.exe',HERE/'loader.map',args.auditor,HERE/'ready-export.json',records)
    profile=dict(scope='authored CF-arena controller probe preparation only; no runtime or application acceptance',
                 child=dict(path='loader.exe',sha256=file_sha(HERE/'loader.exe'),preferred_base=loader['base'],
                    entry_rva=loader['entry'],ready_rva=ready-loader['base'],ready_byte=loader['virtual'][ready-loader['base']],
                    signature_rva=lm['_probe_signature']-loader['base'],
                    globals={name:dict(rva=lm['_'+name]-loader['base'],size=4) for name in ('probe_arena','probe_arena_size','probe_arena_entry','probe_thread_marker')},
                    thread_negative=dict(raw_suffix='thread-negative',callback_rva=lm['_probe_unexpected_thread@4']-loader['base'],
                         marker_initial=0,marker_if_callback_executes=0x11223344,
                         obligation='unchanged broker holds actual CREATE_THREAD before callback execution; normal path has no extra thread')),
                 cf=dict(path='payload.cf',sha256=file_sha(HERE/'payload.cf'),header=fields,
                    initialized_sha256=sha(initialized),relocation_sha256=sha(stream),fixup_offsets=payload['fixups']),
                 functions=function_bounds,marker=layout['marker'],pointer=layout['pointer'],unknown_pointer=layout['unknown_pointer'],tail_abi=layout['tail_abi'],
                 default_entry='probe_prefix',unknown_frontier='probe_unknown',forged_return_entry='probe_bad_return_prefix',
                 entry_variants={name:row['start'] for name,row in function_bounds.items() if name!='probe_unknown'},
                 exports={'payload':'payload-export.json','checked_ret_subset':'payload-ret-export.json','ready':'ready-export.json'})
    write_json(HERE/'profile.json',profile)
    receipt['outputs']={str(p.relative_to(HERE)):file_sha(p) for p in HERE.rglob('*') if p.is_file() and p.suffix in ('.exe','.map','.cf','.json') and p.name!='build-receipt.json'}
    receipt['static_ready']=True;save();write_json(HERE/'build-receipt.json',receipt)
    print('PASS: authored fixture compiled and Rust-exported; child launches=0',flush=True)


if __name__=='__main__':main()
