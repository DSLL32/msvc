#!/usr/bin/env python3
"""Linux-only completed compile regression; no GDB, Wine or PE execution."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import time
from build_inputs import verify_native_build
from case_inputs import load,copy_inputs,copy_dependencies,verify_dependencies,prepare_c_drive
from dependency_provider import Provider,verify

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
sha=lambda b:hashlib.sha256(b).hexdigest()


def shared(name,revision,directory):
    path=directory/(name+'_reader.py')
    path.write_bytes(subprocess.check_output(['git','show',revision+':lib/recon/'+name+'.py'],cwd=ROOT))
    spec=importlib.util.spec_from_file_location('fixture_'+name,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module,path


def original(reference,master):
    item=master['owned_original'];archive=reference/item['file']
    assert archive.stat().st_size==item['bytes'] and sha(archive.read_bytes())==item['sha256']
    root=PROJECT/'build/reference-cache'/item['sha256']
    if not (root/'original/capture.json').exists():
        root.mkdir(parents=True,exist_ok=True)
        with tarfile.open(archive,'r:gz')as tar:
            members=tar.getmembers();assert len(members)==item['files']
            for member in members:
                path=Path(member.name)
                assert member.isfile()and not path.is_absolute()and'..'not in path.parts and path.parts[0]=='original'
                output=root/path;output.parent.mkdir(parents=True,exist_ok=True)
                output.write_bytes(tar.extractfile(member).read())
    raw=root/'original'
    assert sha((raw/'capture.json').read_bytes())==master['capture_sha256']
    return raw


def run(config_path=None):
    started=time.perf_counter();setup=started
    reference=PROJECT/'cases/minimal-c/reference';master=json.loads((reference/'fixture.json').read_text())
    config=json.loads((config_path or PROJECT/'functions.json').read_text())
    assert master['target_sha256']==config['target']['sha256'] and set(master['required_accepted'])<=set(config['accepted'])
    binary=PROJECT/'build/native-trace/CL.EXE';binary_hash=sha(binary.read_bytes())
    build=json.loads(Path(str(binary)+'.build.json').read_text());verify_native_build(build,binary_hash,PROJECT)
    raw=original(reference,master);case=load('minimal-c')
    scratch=Path('/root/.ai-tmp/msvc2/vc5.10/cl');scratch.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='fixture-minimal-',dir=scratch)as work:
        directory=Path(work);copy_inputs(case,directory)
        copy_dependencies(verify_dependencies(raw),directory)
        (directory/'native-build.json').write_text(json.dumps(build)+'\n')
        status_reader,_=shared('status',build['shared_revision'],directory)
        temporary_reader,_=shared('mktemp',build['shared_revision'],directory)
        _,spawn_path=shared('spawn',build['shared_revision'],directory)
        provider=Provider(raw,directory,spawn_path,cached_fixture=True)
        try:
            environment=os.environ.copy()
            for key,value in master['environment'].items():
                if value is None:environment.pop(key,None)
                else:environment[key]=value
            assert master['dos_cwd'].startswith('S:\\')
            mirror=directory/'drive-s'/Path(*master['dos_cwd'][3:].rstrip('\\').split('\\'))
            mirror.parent.mkdir(parents=True,exist_ok=True);mirror.symlink_to(directory,target_is_directory=True)
            c_drive=prepare_c_drive(raw,directory)
            shutil.copyfile(PROJECT/'build/resources.bin',directory/'resources.bin')
            assert sha((directory/'resources.bin').read_bytes())==master['native_resource_provider_sha256']
            (directory/'filter').write_text(''.join(row['native_symbol']+'\n'for row in config['driver']))
            (directory/'seen').write_text('')
            environment.update(provider.environment(),CL_ENTRY_PROFILE=master['entry_profile'],
                RECON_PROGRAM_PATH=master['program_path'],RECON_RESOURCE_PROVIDER=str(directory/'resources.bin'),
                RECON_DRIVE_R=str(ROOT),RECON_DRIVE_S=str(directory/'drive-s'),RECON_DRIVE_C=str(c_drive),
                RECON_TRACE_FILTER=str(directory/'filter'),RECON_TRACE_SEEN=str(directory/'seen'))
            call=master['temporary_name_boundary']['calls'][0]
            prestate=[call['before'][key]for key in ('errno','doserrno','last_error')]
            with status_reader.TerminalStatus()as terminal,temporary_reader.MktempObservation(call['template'],call['result'][-6:],prestate)as temporary:
                environment.update(terminal.environment());environment.update(temporary.environment())
                before={p.name for p in directory.iterdir()}
                with (directory/'trace.txt').open('wb')as trace,(directory/'stdout.bin').open('wb')as out,(directory/'stderr.bin').open('wb')as err:
                    environment['RECON_TRACE_FD']=str(trace.fileno());setup=time.perf_counter()-setup
                    execution=time.perf_counter()
                    process=subprocess.Popen(master['argv'],executable=str(binary),cwd=directory,env=environment,
                        stdin=subprocess.DEVNULL,stdout=out,stderr=err,
                        pass_fds=(*provider.descriptors(),trace.fileno(),terminal.fd,temporary.fd))
                    process.wait(timeout=10);execution=time.perf_counter()-execution
                checks=time.perf_counter()
                (directory/'native-process.json').write_text(json.dumps({'pid':process.pid})+'\n')
                assert terminal.read(process.pid,process.returncode)==master['reference_status']==0
                observed=temporary.read(process.pid)
                assert observed['before']==call['template']and observed['after']==call['result']and observed['argument']==observed['result']
                for phase in ('before','after'):
                    assert {key:observed[phase+'_'+key]for key in ('errno','doserrno','last_error')}==call[phase]
                provider.finish(process.pid)
                try:verify(directory)
                except AssertionError:pass
                else:raise AssertionError('cached fixture was accepted as fresh boundary proof')
            for name in ('stdout','stderr'):
                expected=(reference/(name+'.bin')).read_bytes();assert sha(expected)==master['streams'][name]
                assert (directory/(name+'.bin')).read_bytes()==expected
            trace=(directory/'trace.txt').read_bytes()
            assert sha((reference/'trace.txt').read_bytes())==master['trace_sha256']and trace==(reference/'trace.txt').read_bytes()
            obj=(directory/'test.obj').read_bytes();expected=(reference/'test.obj').read_bytes()
            normalized=lambda data:data[:4]+b'\0'*4+data[8:]
            assert sha(normalized(expected))==master['object_sha256']and normalized(obj)==normalized(expected)
            assert not list(directory.glob('a0*'))and not any((directory/name).exists()for name in ('vc50.idb','vc50.pdb'))
            assert {p.name for p in directory.iterdir()}==before|{'trace.txt','stdout.bin','stderr.bin','test.obj','native-process.json','dependency-native','dependency-native.json','spawn-native.json'},'unexpected fixture output'
            assert sha(binary.read_bytes())==binary_hash
            checks=time.perf_counter()-checks
            return {'case':'minimal-c','passed':True,'mode':'cached native regression; fresh GDB proof separate',
                'application_entries':len(trace.splitlines()),'status':0,'object_bytes':len(obj),'C1_calls':1,'C2_calls':1,
                'mktemp_calls':observed['count'],'native_sha256':binary_hash,'held_trial':config_path is not None,
                'seconds':{'setup':setup,'execution':execution,'checks':checks,'total':time.perf_counter()-started}}
        finally:provider.close()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config',type=Path,help='Explicit held candidate trial for pre-promotion fixture review')
    args=parser.parse_args();report=run(args.config)
    (PROJECT/'build/fixture-minimal-times.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
