#!/usr/bin/env python3
"""Exercise explicit CF/Watcom-MAP auditing against a frozen real trademark fixture.

The fixture is a static comparison, not an application-acceptance receipt. Supply
its project (functions.json/command.json plus the actual Watcom-built PE/MAP).
Every mutation is made in private scratch; original artifacts are read-only.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import statistics
import struct
import subprocess
import tempfile
import time


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary', required=True, type=Path)
    p.add_argument('--fixture', required=True, type=Path)
    p.add_argument('--report', type=Path)
    a = p.parse_args()
    source = json.loads((a.fixture / 'command.json').read_text())
    # command.json is arguments only; no executable is accepted from the fixture.
    assert source and source[0] == '--profile'
    repo = Path(source[source.index('--repo') + 1])
    cfg0 = json.loads((a.fixture / 'functions.json').read_text())
    runtime0 = json.loads((a.fixture / 'runtime.json').read_text())
    metadata0 = json.loads((a.fixture / 'metadata.json').read_text())
    raw0 = (repo / cfg0['target']['path']).read_bytes()
    map0 = (a.fixture / 'trademark.map').read_bytes().decode('ascii')
    rebuilt0 = (a.fixture / 'trademark.exe').read_bytes()
    results = []
    scratch = Path.home() / '.ai-tmp'
    scratch.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='graph-cf-', dir=scratch) as folder:
        work = Path(folder)
        (work/'fixture-metadata.json').write_bytes((a.fixture/'fixture-metadata.json').read_bytes())
        (work/'escaped-evidence').symlink_to(a.binary.resolve())
        def run(name, mutate=None, extra=(), remove=(), expected=None):
            cfg, runtime, metadata = copy.deepcopy(cfg0), copy.deepcopy(runtime0), copy.deepcopy(metadata0)
            raw, rebuilt = bytearray(raw0), bytearray(rebuilt0)
            state = {'map': map0}
            if mutate:
                mutate(cfg, runtime, metadata, raw, rebuilt, state)
            (work/'original.bin').write_bytes(raw)
            (work/'rebuilt.exe').write_bytes(rebuilt)
            (work/'rebuilt.map').write_bytes(state['map'].encode('ascii'))
            cfg['target']['path'] = str(work/'original.bin')
            (work/'config.json').write_text(json.dumps(cfg))
            (work/'runtime.json').write_text(json.dumps(runtime))
            (work/'metadata.json').write_text(json.dumps(metadata))
            args = list(source)
            for flag, value in [('--project', work), ('--config', work/'config.json'), ('--runtime', work/'runtime.json'), ('--metadata', work/'metadata.json'), ('--rebuilt', work/'rebuilt.exe'), ('--map', work/'rebuilt.map')]:
                args[args.index(flag)+1] = str(value)
            for flag in remove:
                i=args.index(flag)
                if i+1<len(args) and not args[i+1].startswith('--'):del args[i:i+2]
                else:del args[i]
            extra=list(extra)
            for flag in ('--profile', '--discovery', '--map-format'):
                if flag in extra and flag in args:
                    i=extra.index(flag);args[args.index(flag)+1]=extra[i+1];del extra[i:i+2]
            args.extend(extra)
            start = time.perf_counter()
            q = subprocess.run([str(a.binary), *args], capture_output=True, text=True)
            elapsed = time.perf_counter() - start
            if expected is None:
                assert q.returncode == 0, (name, q.stderr)
                report = json.loads(q.stdout)
                assert report['checked_functions'] == 1, (name, report)
                assert report['functions']['trademark_']['edges'] == ['printf_'], (name, report)
            else:
                assert q.returncode != 0 and expected in q.stderr, (name, q.returncode, expected, q.stderr)
            results.append({'name':name, 'accepted':q.returncode == 0, 'expected_error':expected, 'seconds':elapsed})
            return q

        def refresh(cfg, raw):
            cfg['target']['sha256'] = digest(raw)
            h = struct.unpack_from('<H', raw, 8)[0]*16
            _, load, size, reloc, _, _ = struct.unpack_from('<6I', raw, h)
            if load+size <= len(raw) and reloc < size:
                cfg['target']['image']['initialized_sha256'] = digest(raw[load:load+size])
                cfg['target']['image']['relocation_sha256'] = digest(raw[load+reloc:load+size])

        def image_field(key, value):
            return lambda c,r,m,b,e,s: c['target']['image'].__setitem__(key,value)
        run('real CF53B -> WatcomPE92B strict positive')
        for key, value, error in [
            ('format','pe','unsupported image format'),
            ('coordinates','preferred-va','image-offset coordinates'),
            ('analysis_base',0x8e0020,'analysis_base 0'),
            ('initialized_sha256','0'*64,'initialized image hash differs'),
            ('relocation_sha256','0'*64,'relocation stream hash differs'),
            ('unexpected',True,'unknown field'),
            ('loader',{'path':'toolchains/watcom10.0a/binnt/WASM.EXE','sha256':'0'*64},'loader hash differs'),
            ('loader',{'path':'/etc/passwd','sha256':'0'*64},'loader must be relative'),
            ('layout_evidence',{'path':'fixture-metadata.json','sha256':'0'*64},'layout evidence hash differs'),
            ('layout_evidence',{'path':'../../../../etc/passwd','sha256':'0'*64},'layout evidence'),
            ('layout_evidence',{'path':'escaped-evidence','sha256':digest(a.binary.read_bytes())},'escapes base'),
            ('phase','post-relocation','unknown field'),
        ]:
            run('definition '+key+' '+str(value),image_field(key,value),expected=error)
        run('target file identity', lambda c,r,m,b,e,s: b.__setitem__(0x2200, b[0x2200]^1), expected='initialized image hash differs')
        run('unloaded file tail still bound by whole-file identity', lambda c,r,m,b,e,s:b.__setitem__(-1,b[-1]^1), expected='target changed')
        def relocated(c,r,m,b,e,s):
            site=0x2200+0xcb79;struct.pack_into('<I',b,site,struct.unpack_from('<I',b,site)[0]+0x8e0020)
        run('pre-relocated original input forbidden',relocated,expected='initialized image hash differs')
        for offset,value,error in [(0x50,0x4644,'uncompressed Watcom CF'),(0x54,0xfffffff0,'range overflow'),(0x5c,0x28282,'entry/relocation/allocation'),(0x60,1,'entry/relocation/allocation'),(0x64,0x26bae,'entry/relocation/allocation')]:
            def change(c,r,m,b,e,s,o=offset,v=value):
                struct.pack_into('<I',b,o,v);c['target']['sha256']=digest(b)
            run('header '+hex(offset),change,expected=error)
        for name,pos,value,error in [
            ('relocation write outside',0x2200+0x26bae+2,0xffff,'relocation write outside'),
            ('relocation duplicate',0x2200+0x26bae+6,0,'duplicate/overlapping'),
            ('relocation overlapping',0x2200+0x26bae+6,1,'duplicate/overlapping'),
            ('premature terminator',0x2200+0x26bae,0,'terminator is not exact'),
        ]:
            def change(c,r,m,b,e,s,p=pos,v=value):
                struct.pack_into('<H',b,p,v);refresh(c,b)
            run(name,change,expected=error)
        run('unknown full allocation code claim',lambda c,r,m,b,e,s:c['driver'][0].__setitem__('end',0x2f000),expected='declared code outside')
        run('undeclared code chunk gap target',lambda c,r,m,b,e,s:c['driver'][0].__setitem__('code_chunks',[[0xcba0,0xcbaa]]),expected='overlapping code chunks')
        run('code/data intersection',lambda c,r,m,b,e,s:m['functions'].__setitem__('trademark_',{'original':{'text_data':[[0xcb74,0xcb75]]}}),expected='CFG enters text data')
        run('data outside code/shared ownership',lambda c,r,m,b,e,s:m['functions'].__setitem__('trademark_',{'original':{'text_data':[[0x26000,0x26004]]}}),expected='lacks explicit shared ownership')
        def callee(c,r,m,b,e,s):
            struct.pack_into('<i',b,0x2200+0xcb9a,0xcb74-(0xcb99+5));refresh(c,b)
        run('changed actual original call target',callee,expected='call graph differs')
        run('missing logical callee mapping',lambda c,r,m,b,e,s:r.clear(),expected='call graph differs')
        run('split final instruction',lambda c,r,m,b,e,s:c['driver'][0].__setitem__('end',0xcba8),expected='extent')
        run('unsupported CF pointer/null callback policy',extra=['--check-callbacks'],expected='CF callback-table checks require fixup-aware')
        run('unsupported CF known-import feature',extra=['--check-imports'],expected='CF has no PE IAT')
        run('unsupported CF profile',extra=['--profile','c2'],expected='CF images require standalone cl CFG')
        run('unsupported integrated CVPACK image view',extra=['--profile','cvpack','--map-format','msvc'],remove=['--metadata','--check-accepted-extents'],expected='explicit image views require')
        run('unsupported legacy/Watcom metadata',extra=['--discovery','legacy'],expected='optional checks/metadata require')
        for name,old,new,error in [
            ('wrong linker identity','WATCOM Linker Version 10.0','WATCOM Linker Version 11.0','not a Watcom 10'),
            ('wrong output kind','creating a PE executable','creating an OS/2 executable','not for a PE'),
            ('ambiguous anchor','BEGTEXT                CODE                          0001:00000000','BEGTEXT                CODE                          0002:00000000','ambiguous'),
            ('group disagreement','_DATA                  DATA           DGROUP         0002:','_DATA                  DATA           DGROUP         0003:','segment/group'),
            ('out of bounds public','0001:00000010+ trademark_','0001:0000ff10+ trademark_','outside actual PE'),
        ]:
            def change(c,r,m,b,e,s,x=old,y=new):
                assert x in s['map'], (name,x)
                s['map']=s['map'].replace(x,y,1)
            run('MAP '+name,change,expected=error)
        def absolute(c,r,m,b,e,s):
            c['driver'][0]['symbol']='__DOSseg__'
        run('absolute MAP public cannot be function',absolute,expected='absolute Watcom MAP symbol')
        run('absolute MAP public cannot be runtime function',lambda c,r,m,b,e,s:r.append({'name':'__DOSseg__','start':0x1d9ad,'end':0x1d9ae,'pending_extent':True}),expected='absolute Watcom MAP symbol cannot be a runtime function')
        # Distinct genuine symbols may share an address. This must not collapse
        # logical names or mutate the actual MAP into another format.
        def alias(c,r,m,b,e,s):
            line=next(v for v in s['map'].splitlines() if v.endswith(' trademark_'))
            s['map']=s['map'].replace(line,line+'\n'+line.replace('trademark_','fixture_alias_'),1)
        run('same-address different-name MAP alias',alias)
        def duplicate(c,r,m,b,e,s):
            line=next(v for v in s['map'].splitlines() if v.endswith(' trademark_'))
            s['map']=s['map'].replace(line,line+'\n'+line.replace(':00000010',':00000011'),1)
        run('conflicting same-name MAP public',duplicate,expected='duplicate/ambiguous')
        def identical(c,r,m,b,e,s):
            line=next(v for v in s['map'].splitlines() if v.endswith(' trademark_'))
            s['map']=s['map'].replace(line,line+'\n'+line,1)
        run('identical same-name MAP repeat',identical)
        for name,count,entry,reloc,size,allocation in [('WCC386',11801,0x2fe00,0x7c448,0x8209c,0x91000),('WPP386',15085,0x664f4,0xbc438,0xc3a44,0xe0000),('WASM',2915,0x1da16,0x26bae,0x28282,0x2f000)]:
            raw=(repo/'toolchains/watcom10.0a/binb'/f'{name}.EXE').read_bytes()
            cfg=copy.deepcopy(cfg0);cfg['driver']=[];cfg['accepted']=[]
            cfg['target']['path']=f'toolchains/watcom10.0a/binb/{name}.EXE'
            refresh(cfg,raw)
            image=cfg['target']['image']
            loader=f'toolchains/watcom10.0a/binnt/{name}.EXE'
            image['loader']={'path':loader,'sha256':digest((repo/loader).read_bytes())}
            packet={'kind':'parser fixture only; no known code or application acceptance','target_sha256':cfg['target']['sha256'],'expected_header':{'entry_offset':entry,'relocation_offset':reloc,'initialized_size':size,'allocation_size':allocation},'expected_fixups':count}
            packet_bytes=json.dumps(packet,sort_keys=True).encode()
            (work/'layout.json').write_bytes(packet_bytes)
            image['layout_evidence']={'path':'layout.json','sha256':digest(packet_bytes)}
            (work/'config.json').write_text(json.dumps(cfg));(work/'empty.json').write_text('[]')
            args=list(source)
            for flag,value in [('--project',work),('--config',work/'config.json'),('--runtime',work/'empty.json'),('--rebuilt-runtime',work/'empty.json')]:args[args.index(flag)+1]=str(value)
            q=subprocess.run([str(a.binary),*args],capture_output=True,text=True)
            assert q.returncode==0,(name,q.stderr)
            report=json.loads(q.stdout);view=report['image_inputs']['original']
            assert report['checked_functions']==0 and view['declared_code_ranges']==[],report
            assert view['relocation_count']==count and view['header']['entry_offset']==entry,report
            results.append({'name':name+' real format/relocations only, no code credit','accepted':True,'relocation_count':count})
        samples=[]
        for i in range(7):
            run('timed strict fixture '+str(i));samples.append(results[-1]['seconds'])
        result={'binary_sha256':digest(a.binary.read_bytes()),'fixture':str(a.fixture),'controls':results,'timing_seconds':{'median':statistics.median(samples),'min':min(samples),'max':max(samples),'scope':'full CLI with image/hash/MAP/graph/RET/extent checks; excludes builds, Wine and target execution'}}
        if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
        print(f'{len(results)} CF/MAP checks pass; strict CLI median {statistics.median(samples)*1000:.2f} ms')

if __name__=='__main__':main()
