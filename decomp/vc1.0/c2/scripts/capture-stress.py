"""Explicit original CL/C1/C2 capture under fixed shared R:/S: mappings.

Never invoked by native fixtures. Install replacements only after independently
verifying the original standalone backend against the captured pipeline object.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
import pe_indexes as idx
from case_paths import C2_SCRATCH, capture_lock, windows


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--stem', required=True)
    parser.add_argument('--case', required=True, help='Stable S:\\vc1.0\\c2\\case cwd component')
    parser.add_argument('--flags', nargs='*', default=['/O2', '/G5'])
    parser.add_argument('--replace', action='store_true')
    args = parser.parse_args()
    assert re.fullmatch('[a-z][a-z0-9_-]*', args.case)
    assert re.fullmatch('[a-z][a-z0-9_]*', args.stem)
    assert all(flag in {'/Od','/O1','/O2','/Oi','/Ob0','/G3','/G4','/G5'} for flag in args.flags)
    fixture = args.fixture.resolve()
    assert fixture.is_relative_to(idx.wt.ROOT/'decomp/vc1.0/fixtures')
    assert args.replace or not (fixture/'manifest.json').exists()
    source = fixture/'source'/(args.stem+'.c')
    assert source.is_file()
    total_started = time.perf_counter()
    with capture_lock():
        scratch = Path(tempfile.mkdtemp(prefix=args.case+'-capture-', dir=C2_SCRATCH))
        work = C2_SCRATCH/args.case
        work.mkdir(exist_ok=True)
        def clear_work():
            for path in work.iterdir():
                assert path.is_file() and not path.is_symlink(), 'unexpected capture cwd entry'
                path.unlink()
        clear_work()
        raw = scratch/'raw'
        raw.mkdir()
        stage = scratch/'fixture'
        for child in ['source','capture','expected','c2']:
            (stage/child).mkdir(parents=True)
        shutil.copyfile(source, stage/'source'/source.name)
        shutil.copyfile(source, work/source.name)
        env = idx.wt.environment()
        for key in ['MSC_CMD_FLAGS','LINK','_LINK_']:
            env.pop(key, None)
        timings = {}
        def invoke(tag, command):
            started = time.perf_counter()
            result = subprocess.run(['wine', *map(str,command)], env=env, cwd=work,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, timeout=90)
            (scratch/(tag+'.stdout')).write_bytes(result.stdout)
            (scratch/(tag+'.stderr')).write_bytes(result.stderr)
            timings[tag] = dict(seconds=time.perf_counter()-started, returncode=result.returncode,
                stdout_sha256=hashlib.sha256(result.stdout).hexdigest(), stderr_sha256=hashlib.sha256(result.stderr).hexdigest())
            assert not result.returncode, (tag,result.stdout,result.stderr)
            return result
        cl_arguments = ['/nologo','/c',*args.flags,source.name]
        result = invoke('pipeline', [windows(Path('build/capture-cl.exe')),windows(idx.wt.TC/'bin/CL.EXE'),
            '11000',windows(raw),windows(work),' '.join(cl_arguments),args.stem+'.obj'])
        assert b'exit 0; captured 2 launches' in result.stdout
        for extension in ['ex','sy','in','gl']:
            shutil.copyfile(raw/('c2-input.'+extension),stage/('c2/ir'+extension))
        shutil.copyfile(work/(args.stem+'.obj'),stage/('expected/'+args.stem+'.obj'))
        for path in raw.iterdir():
            if not path.name.startswith('c2-input'):
                shutil.copyfile(path,stage/'capture'/path.name)
        for stream in ['stdout','stderr']:
            shutil.copyfile(scratch/('pipeline.'+stream),stage/('capture/pipeline.'+stream))
        block = (raw/'spawn-2.environment').read_bytes()
        entries = block.rstrip(b'\0').split(b'\0')
        assert len(entries)==1 and entries[0].startswith(b'MSC_CMD_FLAGS=')
        flags = entries[0][14:].decode('ascii')
        portable = flags.replace(windows(idx.wt.TC/'bin'),'{bin}')
        portable = re.sub(r'-il [^ ]+ ', '-il ir ', portable)
        def record(path):
            return dict(path=path,bytes=(stage/path).stat().st_size,sha256=sha(stage/path))
        output = record('expected/'+args.stem+'.obj')
        normalized = bytearray((stage/output['path']).read_bytes())
        normalized[4:8] = b'\0'*4
        output.update(output_name=args.stem+'.obj',ignored_fields=[dict(offset=4,bytes=4,name='COFF TimeDateStamp')],
                      normalized_sha256=hashlib.sha256(normalized).hexdigest())
        phase = dict(target='toolchains/1.0-nt/bin/C23232.EXE',sha256=sha(idx.wt.TC/'bin/C23232.EXE'),
            arguments=[],environment={'MSC_CMD_FLAGS':portable},environment_inheritance=False,
            canonical_working_directory=windows(work),working_directory='c2',
            captured_application=(raw/'spawn-2.application').read_text(),captured_command_line=(raw/'spawn-2.command').read_text(),
            captured_MSC_CMD_FLAGS=flags,expected_exit=0,expected_stdout='',expected_stderr='',
            inputs=[record('c2/ir'+ext) for ext in ['ex','sy','in','gl']],outputs=[output],
            toolchain_dependencies=[dict(path='toolchains/1.0-nt/bin/C23.ERR',sha256=sha(idx.wt.TC/'bin/C23.ERR'))])
        dependencies = [idx.wt.TC/'bin'/name for name in ['CL.EXE','CL.ERR','C13232.EXE','C23232.EXE','C1.ERR','C23.ERR']]
        if '#include' in source.read_text():
            dependencies += sorted(path for path in (idx.wt.TC/'include').rglob('*') if path.is_file())
        manifest = dict(project='vc1.0',case=fixture.name,description='Original CL '+' '.join(cl_arguments),
            source=record('source/'+source.name),capture=dict(helper='decomp/vc1.0/c2/src/capture-cl.c',
                helper_sha256=sha(Path('src/capture-cl.c')),helper_executable_sha256=sha(Path('build/capture-cl.exe')),
                cl_arguments=cl_arguments,environment={k:env[k] for k in ['INCLUDE','LIB','WINEPATH']},
                canonical_working_directory=windows(work),temporary_directory=windows(work),
                frontend=dict(application=(raw/'spawn-1.application').read_text(),command_line=(raw/'spawn-1.command').read_text(),
                    environment_hex=(raw/'spawn-1.environment').read_bytes().hex(),cwd_parameter=(raw/'spawn-1.cwd').read_text()),
                dependency_policy='Original frontend only; complete toolchain header tree pinned for header-using source',
                dependencies=[dict(path=str(path.relative_to(idx.wt.ROOT)),sha256=sha(path)) for path in dependencies]),phases={'c2':phase})
        clear_work()
        for item in phase['inputs']:
            shutil.copyfile(stage/item['path'],work/Path(item['path']).name)
        env['MSC_CMD_FLAGS'] = portable.format(bin=windows(idx.wt.TC/'bin'))
        invoke('standalone',[windows(Path('build/debug-run.exe')),windows(idx.wt.TC/'bin/C23232.EXE'),'',
            *[windows(scratch/('standalone.'+name)) for name in ['stdout','stderr','events']],windows(work)])
        assert (scratch/'standalone.events').read_text() == 'exit 0\n'
        assert not (scratch/'standalone.stdout').read_bytes() and not (scratch/'standalone.stderr').read_bytes()
        actual = bytearray((work/(args.stem+'.obj')).read_bytes())
        actual[4:8] = b'\0'*4
        assert actual == normalized
        for name in ['stdout','stderr','events']:
            shutil.copyfile(scratch/('standalone.'+name),stage/('capture/standalone.'+name))
        (stage/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        validation = dict(timing=timings,standalone='exit0, empty streams, object matches pipeline after only timestamp',
            wine_prefix=str(idx.wt.PREFIX),fixed_drives={'R:':str(idx.wt.ROOT),'S:':str(C2_SCRATCH.parents[1])},
            canonical_working_directory=windows(work),capture_lock=windows(C2_SCRATCH/'capture.lock'),
            total_seconds=time.perf_counter()-total_started)
        (stage/'capture-validation.json').write_text(json.dumps(validation,indent=2)+'\n')
        # Install only the completely checked replacement; historical raw bundles
        # are removed, never rewritten into invented shared-path evidence.
        for name in ['capture','expected','c2']:
            if (fixture/name).exists(): shutil.rmtree(fixture/name)
            shutil.move(str(stage/name),str(fixture/name))
        for name in ['manifest.json','capture-validation.json']:
            shutil.copyfile(stage/name,fixture/name)
        (fixture/'README.txt').write_text('Original-only capture under shared fixed R:/S: drives.\nNative fixture runs use captured IR, object and trace references and never Wine.\nActual CL/C1/C2 argv/environment and inputs are pinned in manifest.json.\nOnly COFF timestamp bytes4..7 are normalized.\nCanonical cwd: '+windows(work)+'\n')
        print(json.dumps(dict(fixture=str(fixture),output=output,timing=validation),indent=2))


if __name__=='__main__':
    main()
