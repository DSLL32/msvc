"""Explicit original driver/phase capture for the approved C1XX queue.

Writes new staging fixtures under fixed S:, never modifies active/shared inputs.
No fixture imports or invokes this reference operation.
"""
import argparse
import copy
import fcntl
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time
import tempfile
from case_paths import BINARY, ROOT, SCRATCH, CAPTURE_ROOT, windows

SPECS = {
    'cpp-basic': ('c1xx-cpp-basic', 'cpp-basic.cpp'),
    'virtual-diamond': ('c1xx-virtual-diamond', 'virtual.cpp'),
    'copy-assignment': ('c1xx-copy-assignment', 'copy.cpp'),
    'stack-lifetime': ('c1xx-stack-lifetime', 'lifetime.cpp'),
    'new-delete': ('c1xx-new-delete', 'allocation.cpp'),
    'state-machine-debug': ('c1xx-state-machine-debug', 'state-machine.cpp'),
    'state-machine-defined-debug': ('c1xx-state-machine-defined-debug', 'state-machine.cpp'),
    'member-registry-browser': ('c1xx-member-registry-browser', 'registry.cpp'),
}


CASE_FLAGS = {
    'state-machine-debug': '/Od /G5 /Z7',
    'state-machine-defined-debug': '/Od /G5 /Z7',
    'member-registry-browser': '/Od /G5 /Z7 /vmg /FRregistry.sbr',
}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path, root, output=None, volatile=None):
    item = dict(path=str(path.relative_to(root)), bytes=path.stat().st_size, sha256=sha(path))
    if output: item['output_name'] = output
    if volatile:
        item['ignored_fields'] = volatile
        data = bytearray(path.read_bytes())
        for field in volatile: data[field['offset']:field['offset']+field['bytes']] = bytes(field['bytes'])
        item['normalized_sha256'] = hashlib.sha256(data).hexdigest()
    return item



def pe_clock_fields(data):
    """Only observed PE COFF and IMAGE_DEBUG_DIRECTORY timestamps.

    Debug entries are file-backed 28-byte records. The encountered COFF,
    CodeView and FPO entries carry the same link clock as the PE header.
    Other entry kinds or unrelated values require explicit investigation.
    """
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe+4] == b'PE\0\0'
    fields = [dict(offset=pe+8, bytes=4, name='PE COFF TimeDateStamp')]
    stamp = struct.unpack_from('<I', data, pe+8)[0]
    optional = pe+24
    assert struct.unpack_from('<H',data,optional)[0] == 0x10b
    rva,size = struct.unpack_from('<II',data,optional+96+6*8)
    if not size:
        assert not rva
        return fields
    assert rva and size % 28 == 0
    sections = optional+struct.unpack_from('<H',data,pe+20)[0]
    offsets = []
    for index in range(struct.unpack_from('<H',data,pe+6)[0]):
        row=sections+40*index
        start,raw_size,raw_offset=struct.unpack_from('<III',data,row+12)
        if start <= rva and rva+size <= start+raw_size:
            offsets.append(raw_offset+rva-start)
    assert len(offsets) == 1
    start=offsets[0]
    assert start+size <= len(data)
    for offset in range(start,start+size,28):
        kind=struct.unpack_from('<I',data,offset+12)[0]
        assert kind in (1,2,3), 'unreviewed IMAGE_DEBUG_DIRECTORY kind'
        assert struct.unpack_from('<I',data,offset+4)[0] == stamp, 'unreviewed debug timestamp value'
        fields.append(dict(offset=offset+4,bytes=4,
            name='IMAGE_DEBUG_DIRECTORY type%d TimeDateStamp'%kind))
    return fields


def clear_work(name):
    path = CAPTURE_ROOT/name
    path.mkdir(parents=True, exist_ok=True)
    for child in path.iterdir():
        assert child.is_file() and not child.is_symlink(), 'unexpected capture cwd entry: '+str(child)
        child.unlink()
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', action='append', choices=SPECS)
    parser.add_argument('--stage-root',type=Path,help='new staging directory under fixed R: or S:; never overwritten')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('shared_original_tool', BINARY/'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(spec); spec.loader.exec_module(tool)
    env = tool.environment()
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'): env.pop(key, None)
    CAPTURE_ROOT.mkdir(parents=True, exist_ok=True)
    lease = (CAPTURE_ROOT/'capture.lock').open('a'); fcntl.flock(lease, fcntl.LOCK_EX)
    stages=args.stage_root.resolve() if args.stage_root else Path(tempfile.mkdtemp(prefix='fixture-stage-',dir=CAPTURE_ROOT))
    windows(stages)
    if args.stage_root:stages.mkdir(parents=True,exist_ok=False)
    summary = []
    summary_path = stages/'fixture-capture-accounting.json'

    for slug in args.case or SPECS:
        folder, filename = SPECS[slug]
        driver_flags = '/nologo ' + CASE_FLAGS.get(slug, '/Od') + ' '
        ir_extensions = ('ex','sy','in','gl') + (('db',) if '/Z7' in driver_flags else ())
        source_fixture = BINARY.parent/'fixtures'/folder
        source = source_fixture/'c1'/filename
        template = source_fixture/'manifest.json'
        if slug in ('state-machine-defined-debug','member-registry-browser'):
            source = BINARY/'cases/proposals'/slug/filename
            template = BINARY.parent/'fixtures/c1xx-state-machine-debug/manifest.json'
        sources = [source]
        if slug == 'member-registry-browser': sources.append(source.with_name('registry.h'))
        original_manifest = json.loads(template.read_text())
        stage = stages/('c1xx-'+slug); stage.mkdir(exist_ok=False)
        for name in ('c1','c2','link','expected','dependencies','capture','validation'): (stage/name).mkdir()
        for input_source in sources: shutil.copyfile(input_source, stage/'c1'/input_source.name)
        stem = Path(filename).stem
        operations = []
        receipt_path = stage/'capture/driver-receipt.json'

        def run(label, command, cwd, run_env=env, prefix=stage/'capture'):
            started = time.perf_counter()
            result = subprocess.run(command, cwd=cwd, env=run_env, stdin=subprocess.DEVNULL,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=90)
            duration = time.perf_counter()-started
            (prefix/(label+'-stdout.bin')).write_bytes(result.stdout)
            (prefix/(label+'-stderr.bin')).write_bytes(result.stderr)
            row = dict(operation=label, command=list(map(str,command)), cwd=windows(cwd),
                       status=result.returncode, execution_seconds=duration,
                       stdout_sha256=hashlib.sha256(result.stdout).hexdigest(),
                       stderr_sha256=hashlib.sha256(result.stderr).hexdigest())
            operations.append(row)
            receipt_path.write_text(json.dumps(dict(case=slug,operations=operations),indent=2)+'\n')
            assert result.returncode == 0, (slug,label,result.returncode,result.stderr.decode('latin1'))
            print(slug, label, '%.6fs'%duration, flush=True)
            return result,row

        def driver(suffix, arguments):
            work = clear_work(slug+'-driver'+suffix)
            for input_source in sources: shutil.copyfile(input_source,work/input_source.name)
            captured = stage/'capture'/('driver'+suffix); captured.mkdir()
            command = ['wine',windows(BINARY/'build/reference-capture-driver.exe'),
                       windows(tool.TC/'bin/CL.EXE'),'11000',windows(captured),windows(work),
                       arguments,stem+'.obj']
            result,row = run('driver'+suffix,command,work)
            count = 2 if '/c ' in arguments else 3
            assert ('captured %d launches'%count).encode() in result.stdout
            return work,captured

        compiled = None
        if slug == 'cpp-basic':
            compiled = driver('-compile-only','/nologo /Od /c '+filename)
        work,captured = driver('',driver_flags+filename)
        if compiled:
            for suffix in ir_extensions:
                assert (compiled[1]/('c2-input.'+suffix)).read_bytes() == (captured/('c2-input.'+suffix)).read_bytes()
        assert {p.suffix[1:] for p in captured.glob('c2-input.*')} == set(ir_extensions), 'unexpected/missing original C2 input snapshot'
        for suffix in ir_extensions:
            shutil.copyfile(captured/('c2-input.'+suffix),stage/'c2'/('ir'+suffix))
        extra_frontend_outputs = ['registry.sbr'] if slug == 'member-registry-browser' else []
        for extra in extra_frontend_outputs: shutil.copyfile(work/extra, stage/'expected'/extra)
        shutil.copyfile(captured/'link-input.obj',stage/'link'/(stem+'.obj'))
        shutil.copyfile(captured/'link-input.obj',stage/'expected'/(stem+'.obj'))
        shutil.copyfile(captured/'link-input.rsp',stage/'link/link.rsp')
        shutil.copyfile(work/(stem+'.exe'),stage/'expected'/(stem+'.exe'))
        result,_ = run('driver-program',['wine',windows(work/(stem+'.exe'))],work)
        assert not result.stdout and not result.stderr

        manifest = copy.deepcopy(original_manifest)
        manifest['case'] = 'CL.EXE '+driver_flags+('/c ' if compiled else '')+filename
        manifest['capture'] = dict(method='Fresh original CL CreateProcessA breakpoint; shared fixed R:/S: prefix',
            CL_sha256=sha(tool.TC/'bin/CL.EXE'),arguments=driver_flags+('/c ' if compiled else '')+filename,
            downstream_capture_arguments=driver_flags+filename,source_sha256=sha(source),
            inherited_working_directory=windows(work),child_current_directory_parameter=None,
            raw_boundary_files='capture/driver/',stdin='closed',compiler_environment_keys=['MSC_CMD_FLAGS'],
            wine_prefix=str(tool.PREFIX),fixed_drives={'R:':str(ROOT),'S:':str(SCRATCH)},
            timing_receipt='capture/driver-receipt.json')
        if compiled:
            manifest['capture']['frontend_outputs_identical_with_and_without_driver_c_switch'] = True
            manifest['capture']['compile_only_raw_boundary_files'] = 'capture/driver-compile-only/'
        for number,phase in ((1,'c1'),(2,'c2'),(3,'link')):
            item = manifest['phases'][phase]
            item['captured_application'] = (captured/('spawn-%d.application'%number)).read_bytes().decode('latin1')
            item['captured_command_line'] = (captured/('spawn-%d.command'%number)).read_bytes().decode('latin1')
            assert not (captured/('spawn-%d.cwd'%number)).read_bytes()
            if number < 3:
                raw = (captured/('spawn-%d.environment'%number)).read_bytes()
                pairs = [x.decode('latin1').split('=',1) for x in raw.split(b'\0') if x]
                assert len(pairs)==1 and pairs[0][0]=='MSC_CMD_FLAGS'
                flags = pairs[0][1]; item['captured_MSC_CMD_FLAGS'] = flags
                flags,count = re.subn(r'(-il )\S+',r'\1ir',flags); assert count==1
                flags = flags.replace(windows(tool.TC/'bin'),'{bin}').replace(windows(tool.TC/'include'),'{include}')
                assert 'Z:' not in flags and 'N:' not in flags
                item['environment'] = {'MSC_CMD_FLAGS':flags}
                item['command_line_template'] = item['captured_command_line'].replace(windows(tool.TC/'bin'),'{bin}')
                assert item['captured_command_line'] == item['captured_application']
            else:
                item['captured_LIB'] = (captured/'spawn-3.LIB').read_bytes().decode('latin1')
                item['environment'] = {'LIB':'{lib}'}
                item['command_line_template'] = '{bin}\\link.exe @link.rsp'
                item['arguments'] = ['@link.rsp']
            case_cwd = slug + ('-driver' if slug == 'member-registry-browser' and phase == 'c1' else ('' if phase == 'c1' else '-'+phase))
            item['canonical_working_directory'] = 'S:\\vc1.0\\c1xx\\'+case_cwd
            item['expected_exit']=0; item['expected_stdout']=''; item['expected_stderr']=''
        c1,c2,link = (manifest['phases'][p] for p in ('c1','c2','link'))
        c1['inputs'] = [record(stage/'c1'/input_source.name,stage) for input_source in sources]
        c1['outputs'] = [record(stage/'c2'/('ir'+s),stage,'ir'+s) for s in ir_extensions]
        c2['inputs'] = [{k:v for k,v in r.items() if k!='output_name'} for r in c1['outputs']]
        c1['outputs'] += [record(stage/'expected'/extra,stage,extra) for extra in extra_frontend_outputs]
        c2['outputs'] = [record(stage/'expected'/(stem+'.obj'),stage,stem+'.obj',[dict(offset=4,bytes=4,name='COFF TimeDateStamp')])]
        link['inputs'] = [record(stage/'link'/(stem+'.obj'),stage),record(stage/'link/link.rsp',stage)]
        clocks = pe_clock_fields((stage/'expected'/(stem+'.exe')).read_bytes())
        link['outputs'] = [record(stage/'expected'/(stem+'.exe'),stage,stem+'.exe',clocks)]
        link['generated_program']['path'] = stem+'.exe'
        for phase in (c1,c2):
            for item in phase.get('toolchain_dependencies',[]):
                source_dependency = ROOT/item['path']
                target = stage/'dependencies'/source_dependency.name
                shutil.copyfile(source_dependency,target)
                item.update(bytes=target.stat().st_size,sha256=sha(target),cached_path=str(target.relative_to(stage)))
        (stage/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        validation = {}
        for phase,item in manifest['phases'].items():
            case_cwd = slug + ('-driver' if slug == 'member-registry-browser' and phase == 'c1' else ('' if phase == 'c1' else '-'+phase))
            phase_work = clear_work(case_cwd)
            for entry in item['inputs']:
                path=stage/entry['path']; assert sha(path)==entry['sha256']; shutil.copyfile(path,phase_work/path.name)
            phase_env=env.copy()
            phase_env.update({k:v.format(bin=windows(tool.TC/'bin'),include=windows(tool.TC/'include'),lib=windows(tool.TC/'lib')) for k,v in item['environment'].items()})
            command=['wine',windows(BINARY/'build/reference-run-phase.exe'),windows(ROOT/item['target']),
                     ' '.join(item['arguments']),item['command_line_template'].format(bin=windows(tool.TC/'bin')),
                     item['canonical_working_directory']]
            result,row=run('independent-'+phase,command,phase_work,phase_env,stage/'validation')
            assert not result.stdout and not result.stderr,(slug,phase,result.stdout,result.stderr)
            outputs=[]
            for entry in item['outputs']:
                original=(stage/entry['path']).read_bytes(); replay=(phase_work/entry['output_name']).read_bytes()
                assert len(original)==len(replay),(slug,phase,entry['output_name'],'size differs')
                differences=[i for i,(a,b) in enumerate(zip(original,replay)) if a!=b]
                permitted={i for field in entry.get('ignored_fields',[]) for i in range(field['offset'],field['offset']+field['bytes'])}
                assert set(differences)<=permitted,(slug,phase,entry['output_name'],differences)
                outputs.append(dict(file=entry['output_name'],bytes=len(replay),actual_sha256=hashlib.sha256(replay).hexdigest(),
                                    differing_byte_offsets=differences,permitted_volatile_fields=entry.get('ignored_fields',[])))
            validation[phase]=dict(status=0,execution_seconds=row['execution_seconds'],outputs=outputs,stdout='',stderr='')
            if phase=='link':
                result,row=run('independent-program',['wine',windows(phase_work/(stem+'.exe'))],phase_work)
                assert not result.stdout and not result.stderr
                validation['program']=dict(status=0,execution_seconds=row['execution_seconds'],stdout='',stderr='')
        (stage/'validation.json').write_text(json.dumps(validation,indent=2)+'\n')
        (stage/'README.txt').write_text('Fresh original-validated '+slug+' fixture in the shared fixed R:/S: Wine prefix.\n\nRaw CL child application/command/environment bytes and phase input snapshots\nare in capture/. Compiler phases use argv0 plus explicit MSC_CMD_FLAGS.\nIndependent frontend output is byte exact; downstream differences are limited\nto the separately identified four-byte COFF/PE timestamp fields. Both generated\nprogram invocations emit nothing and exit0. Native fixture runs never invoke\nWine. Separate measured process times are retained in capture/driver-receipt.json\nand validation.json; helper build time is separately recorded or marked unmeasured.\n')
        summary.append(dict(case=slug,stage=windows(stage),operations=operations,status='pass'))
        summary_path.write_text(json.dumps(dict(cases=summary,wine_prefix=str(tool.PREFIX),fixed_drives={'R:':str(ROOT),'S:':str(SCRATCH)}),indent=2)+'\n')
        print(slug+': complete driver and independent phases verified',flush=True)
    print(summary_path)


if __name__=='__main__': main()
