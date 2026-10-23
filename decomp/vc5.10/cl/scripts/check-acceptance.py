#!/usr/bin/env python3
"""Validator protocol tests, using synthetic observations; never accepts functions."""
import copy
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import struct
from accept import validate,trial_config,identity,CRT_SHA
from mask import construct
from audit import input_paths,FLAGS
from state import project as decode_state
from build_inputs import store_blob

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def write(path,value):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(value,indent=2)+'\n')
    return {'path':str(path),'sha256':sha(path)}


def refresh(path):return {'path':str(path),'sha256':sha(path)}


scratch=Path('/root/.ai-tmp/msvc2/vc5.10/cl');scratch.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='accept-protocol-',dir=scratch) as directory:
    project=Path(directory)
    for name in ('Makefile','build/native-trace/CL.EXE','build/cl.exe','build/cl.map','runtime.json','build/runtime.json','graph-metadata.json','scripts/audit.py','scripts/import_residuals.py','cases/help/import-residuals.json','src/test.c'):
        p=project/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'protocol fixture: '+name.encode())
    base=json.loads((PROJECT/'functions.json').read_text())
    base['accepted']=[]
    base['acceptance_reviewed']=True
    base['driver'][0]['evidence']={'kind':'synthetic validator test only'}
    if not any(r['name']=='_install_control_handler' for r in base['driver']):
        base['driver'].append({'name':'_install_control_handler','start':0x4057b4,'end':0x4057c2})
    trial=trial_config(base,'_main')
    expected={'candidate':'_main','target_sha256':sha(ROOT/base['target']['path']),
        'native_sha256':sha(project/'build/native-trace/CL.EXE'),'rebuilt_sha256':sha(project/'build/cl.exe'),
        'trial_mask_sha256':hashlib.sha256(construct(trial)).hexdigest()}
    def prepare():
        config=write(project/'trial.json',trial)
        report=write(project/'audit.json',{'discovery':'cfg','functions':{'_main':{}},'checked_functions':1})
        audit_inputs={key:{'path':str(path),'sha256':sha(path)} for key,path in input_paths(project,ROOT,Path(config['path'])).items()}
        native_build=write(project/'native-build.json',{'kind':'linked-native',
            'output_sha256':expected['native_sha256'],'builder_source':store_blob(b'synthetic build controller'),
            'sources':{str(project/'src/test.c'):store_blob((project/'src/test.c').read_bytes())},
            'objects':{},'command':['clang','-m32','-O0','-g']})
        receipt={**expected,'base_identity':identity(base),'trial_identity':identity(trial),
            'sources':{'src/test.c':sha(project/'src/test.c')},'native_build':native_build,
            'graph_audit':{'config_snapshot':config,'trial_identity':identity(trial),'status':0,'report':report,'inputs':audit_inputs,'flags':FLAGS}}
        for platform in ('original','native'):
            d=project/platform;d.mkdir(exist_ok=True)
            files={}
            (d/'trace.txt').write_text('main\n');files['trace']=refresh(d/'trace.txt')
            for role in ('stdout','stderr'):
                (d/role).write_bytes(b'');files[role]=refresh(d/role)
            program=b'S:\\test\\CL.EXE\0'
            def raw_vector(pointer,strings):
                return struct.pack('<II',pointer,len(strings))+b''.join(struct.pack('<II',0x5000+100*i,len(value))+value for i,value in enumerate(strings))
            (d/'argv.bin').write_bytes(raw_vector(0x1000,[program,b'/?\0']))
            (d/'environment.bin').write_bytes(raw_vector(0x2000,[]))
            (d/'pgmptr.bin').write_bytes(program)
            if platform=='original':
                from binary import PE
                pe=PE(ROOT/trial['target']['path'])
                (d/'entries.txt').write_text(''.join('%x %d %s\n'%(r['start']-pe.base,pe.read(r['start'],1)[0],r['native_symbol'])for r in trial['driver']if r['name'] in trial['accepted']))
                files['entry_guards']=refresh(d/'entries.txt')
                (d/'events').write_text('entry event=0 address=00401743 name=main\ntrap event=1 address=004057b4 rva=000057b4 eax=00000000\nexit event=2 status=28673\n')
                files['events']=refresh(d/'events')
                context=bytearray(268);struct.pack_into('<I',context,0,8);struct.pack_into('<I',context,28,0x27f)
                struct.pack_into('<4I',context,204,0x1234,2,0x1000,0x2000)
                (d/'context.bin').write_bytes(context);(d/'globals.bin').write_bytes(bytes(2000))
                crt=bytearray(132);struct.pack_into('<6I',crt,0,0x78000000,0,0,1,0x3000,0x4000)
                for i,value in enumerate((0xc1,0x81,0x81)):crt[24+36*i+4]=value
                (d/'crt-state.bin').write_bytes(crt)
                files['loaded_msvcrt']={'path' :str(ROOT/'toolchains/5.10/bin/MSVCRT.DLL'),'sha256':CRT_SHA}
            else:
                (d/'trace-filter.txt').write_text('main\n');files['raw:trace-filter.txt']=refresh(d/'trace-filter.txt')
                (d/'debugger.txt').write_text('Program received signal SIGTRAP\nNATIVE_FRONTIER_SYMBOL=install_control_handler\n')
                files['debugger']=refresh(d/'debugger.txt')
                write(d/'native-crt.json',{'argc':2,'argv_address':0x1000,'envp_address':0x2000,'environ_address':0x2000,'fmode':0,'commode':0,'mb_cur_max':1,'osfile':[0xc1,0x81,0x81],'x87_control_word':0x27f})
                (d/'gdb-capture.py').write_text('# synthetic GDB protocol inputs only\n')
            files['state']=write(d/'state.json',decode_state(platform,d))
            for raw_path in d.iterdir():
                if raw_path.is_file() and raw_path.name not in ('observation.json','state.json'):files['raw:'+raw_path.name]=refresh(raw_path)
            observation={**expected,'platform':platform,'trial_identity':identity(trial),
                'config_snapshot':config,'case':'help','arguments':['/?'],'files':files}
            if platform=='native':observation['native_build']=native_build
            receipt[platform]=write(d/'observation.json',observation)
        return receipt
    validate(base,prepare(),'_main',project,ROOT)
    passed=[]
    def negative(name,change):
        receipt=prepare();change(receipt)
        try:validate(base,receipt,'_main',project,ROOT)
        except (AssertionError,KeyError,ValueError):passed.append(name)
        else:raise AssertionError('accepted negative protocol: '+name)
    negative('changed native build receipt',lambda r:r['native_build'].update(sha256='0'*64))
    def wrong_build_binary(r):
        path=Path(r['native_build']['path']);value=json.loads(path.read_text());value['output_sha256']='0'*64
        r['native_build']=write(path,value)
    negative('build receipt for different executable',wrong_build_binary)
    def wrong_compiled_source(r):
        path=Path(r['native_build']['path']);value=json.loads(path.read_text())
        value['sources'][str(project/'src/test.c')]=store_blob(b'old application body')
        r['native_build']=write(path,value)
    negative('compiled application source is stale',wrong_compiled_source)
    negative('old mask without candidate',lambda r:r.update(trial_mask_sha256=hashlib.sha256(construct(base)).hexdigest()))
    negative('stale audited rebuilt executable',lambda r:r['graph_audit']['inputs']['rebuilt'].update(sha256='0'*64))
    negative('changed audited MAP',lambda r:r['graph_audit']['inputs']['map'].update(sha256='0'*64))
    negative('changed audited runtime index',lambda r:r['graph_audit']['inputs']['runtime'].update(sha256='0'*64))
    negative('changed audited graph metadata',lambda r:r['graph_audit']['inputs']['metadata'].update(sha256='0'*64))
    negative('changed audited graph tool',lambda r:r['graph_audit']['inputs']['tool'].update(sha256='0'*64))
    negative('omitted audit flags',lambda r:r['graph_audit'].update(flags=[]))
    negative('trial omits candidate' ,lambda r:r.update(trial_identity=identity(base)))
    def stale_audit(r):
        r['graph_audit']['config_snapshot']=write(project/'audit-stale.json',base)
    negative('audit config omits candidate',stale_audit)
    negative('audit did not check candidate',lambda r:r['graph_audit'].update(report=write(project/'audit.json',{'discovery':'cfg','functions':{}})))
    def observations(r,change):
        for platform in ('original','native'):
            p=Path(r[platform]['path']);o=json.loads(p.read_text());change(platform,o)
            r[platform]=write(p,o)
    def stale_both(r):
        stale=copy.deepcopy(trial);stale['initial_frontier']['original']+=1
        entry=write(project/'stale-both.json',stale)
        observations(r,lambda platform,o:o.update(config_snapshot=entry))
    negative('mutually stale snapshots',stale_both)
    negative('changed native binary',lambda r:(project/'build/native-trace/CL.EXE').write_bytes(b'changed'))
    (project/'build/native-trace/CL.EXE').write_bytes(b'protocol fixture: build/native-trace/CL.EXE')
    def raw_change(r,role,content,platform='original'):
        p=Path(r[platform]['path']);o=json.loads(p.read_text());f=Path(o['files'][role]['path']);f.write_text(content)
        o['files'][role]=refresh(f);r[platform]=write(p,o)
    negative('wrong unknown original frontier',lambda r:raw_change(r,'events','trap event=1 address=00401234 rva=00001234 eax=00000000\n'))
    negative('unequal native frontier',lambda r:raw_change(r,'debugger','Program received signal SIGTRAP\nNATIVE_FRONTIER_SYMBOL=main\n','native'))
    def never_executed(r):
        raw_change(r,'events','trap event=1 address=00401743 rva=00001743 eax=00000000\n')
        raw_change(r,'debugger','Program received signal SIGTRAP\nNATIVE_FRONTIER_SYMBOL=main\n','native')
    negative('both stuck at candidate entry',never_executed)
    negative('two missing outcomes',lambda r:observations(r,lambda platform,o:o['files'].pop('events' if platform=='original' else 'debugger')))
    negative('candidate missing from trace',lambda r:raw_change(r,'trace',''))
    negative('trace changed',lambda r:raw_change(r,'trace','install_control_handler\n','native'))
    negative('raw stream altered with refreshed hashes',lambda r:raw_change(r,'stdout','changed output'))
    negative('raw typed state altered with refreshed hashes',lambda r:raw_change(r,'state','{"projection":"synthetic protocol scalar","fields":{"mode":2}}\n'))
    def changed_capture(r):
        Path(r['native']['path']).write_text('{}\n')
    negative('raw capture metadata changed',changed_capture)
    def raw_memory(r,filename,mutate):
        p=Path(r['original']['path']);o=json.loads(p.read_text())
        file=Path(o['files']['raw:'+filename]['path']);data=bytearray(file.read_bytes());mutate(data);file.write_bytes(data)
        o['files']['raw:'+filename]=refresh(file);r['original']=write(p,o)
    negative('original raw argc changed',lambda r:raw_memory(r,'context.bin',lambda b:struct.pack_into('<I',b,208,3)))
    negative('original raw descriptor flags changed',lambda r:raw_memory(r,'crt-state.bin',lambda b:b.__setitem__(24+2*36+4,0xc1)))
    def native_crt_changed(r):
        p=Path(r['native']['path']);o=json.loads(p.read_text());file=Path(o['files']['raw:native-crt.json']['path'])
        value=json.loads(file.read_text());value['fmode']=1
        o['files']['raw:native-crt.json']=write(file,value);r['native']=write(p,o)
    negative('native actual CRT value changed',native_crt_changed)
    print('acceptance protocol positive control and %d negative controls pass; no persisted acceptance'%len(passed))
