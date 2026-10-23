"""Explicit original-only debug/listing capture, including the pre-C2 PDB."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import time
import pe_indexes as idx
from case_paths import C2_SCRATCH, capture_lock, windows


def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def normalized(data, fields):
    data=bytearray(data)
    for field in fields:data[field['offset']:field['offset']+field['bytes']]=b'\0'*field['bytes']
    return bytes(data)


def main():
    started=time.perf_counter()
    fixture=Path('../fixtures/c2-debug-asm-listing').resolve()
    assert not (fixture/'manifest.json').exists(), 'Never overwrite frozen capture'
    source=fixture/'source/dbglist.c'
    with capture_lock():
        scratch=Path(tempfile.mkdtemp(prefix='dbglist-capture-',dir=C2_SCRATCH))
        raw=scratch/'raw';raw.mkdir()
        stage=scratch/'fixture'
        for name in ['capture','source','c2','expected']:(stage/name).mkdir(parents=True)
        shutil.copyfile(source,stage/'source/dbglist.c')
        work=C2_SCRATCH/'dbglist';work.mkdir(exist_ok=True)
        # Preserve the initial support probe before clearing this owned cwd.
        prior=C2_SCRATCH/'dbglist-probe/outputs'
        if prior.parent.exists() and not prior.exists():
            prior.mkdir()
            for p in work.iterdir():
                if p.is_file() and p.name!='dbglist.c':shutil.copyfile(p,prior/p.name)
        for p in work.iterdir():
            assert p.is_file() and not p.is_symlink();p.unlink()
        shutil.copyfile(source,work/source.name)
        env=idx.wt.environment()
        for key in ['MSC_CMD_FLAGS','LINK','_LINK_']:env.pop(key,None)
        timings={}
        def invoke(label,command):
            phase=time.perf_counter()
            result=subprocess.run(['wine',*map(str,command)],cwd=work,env=env,
                stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=90)
            for stream in ['stdout','stderr']:(scratch/(label+'.'+stream)).write_bytes(getattr(result,stream))
            timings[label]=dict(seconds=time.perf_counter()-phase,returncode=result.returncode)
            assert result.returncode==0,(label,result.stdout,result.stderr)
            return result
        arguments=['/nologo','/c','/Od','/G3','/Zi','/FAcs','dbglist.c']
        result=invoke('pipeline',[windows(Path('build/capture-cl.exe')),windows(idx.wt.TC/'bin/CL.EXE'),
            '11000',windows(raw),windows(work),' '.join(arguments),'dbglist.obj','msvc.pdb'])
        assert result.stdout==b'dbglist.c\r\nexit 0; captured 2 launches\r\n' and not result.stderr
        assert {p.name for p in work.iterdir()}=={'dbglist.c','dbglist.obj','dbglist.cod','msvc.pdb'}, 'Unexpected original artifacts'
        for extension in ['ex','sy','in','gl','db']:shutil.copyfile(raw/('c2-input.'+extension),stage/('c2/ir'+extension))
        shutil.copyfile(source,stage/'c2/dbglist.c')
        shutil.copyfile(raw/'c2-dependency-before.bin',stage/'c2/msvc.pdb')
        pdb_unchanged=(raw/'c2-dependency-before.bin').read_bytes()==(work/'msvc.pdb').read_bytes()
        for name in ['dbglist.obj','dbglist.cod','msvc.pdb']:shutil.copyfile(work/name,stage/'expected'/name)
        for p in raw.iterdir():
            if not p.name.startswith('c2-input'):shutil.copyfile(p,stage/'capture'/p.name)
        for stream in ['stdout','stderr']:shutil.copyfile(scratch/('pipeline.'+stream),stage/('capture/pipeline.'+stream))
        block=(raw/'spawn-2.environment').read_bytes();entries=block.rstrip(b'\0').split(b'\0')
        assert len(entries)==1 and entries[0].startswith(b'MSC_CMD_FLAGS=')
        flags=entries[0][14:].decode('ascii');portable=flags.replace(windows(idx.wt.TC/'bin'),'{bin}')
        portable=re.sub(r'-il [^ ]+ ','-il ir ',portable)
        def record(path):return dict(path=path,bytes=(stage/path).stat().st_size,sha256=sha(stage/path))
        outputs=[]
        for name in ['dbglist.obj','dbglist.cod','msvc.pdb']:
            row=record('expected/'+name)
            fields=[dict(offset=4,bytes=4,name='COFF TimeDateStamp')] if name.endswith('.obj') else []
            row.update(output_name=name,ignored_fields=fields,normalized_sha256=hashlib.sha256(normalized((stage/row['path']).read_bytes(),fields)).hexdigest())
            if name=='msvc.pdb':row['role']='frontend dependency retained unchanged' if pdb_unchanged else 'backend-updated frontend dependency'
            outputs.append(row)
        phase=dict(target='toolchains/1.0-nt/bin/C23232.EXE',sha256=sha(idx.wt.TC/'bin/C23232.EXE'),arguments=[],
            environment={'MSC_CMD_FLAGS':portable},environment_inheritance=False,canonical_working_directory=windows(work),working_directory='c2',
            captured_application=(raw/'spawn-2.application').read_text(),captured_command_line=(raw/'spawn-2.command').read_text(),captured_MSC_CMD_FLAGS=flags,
            expected_exit=0,expected_stdout='',expected_stderr='',inputs=[record('c2/ir'+ext) for ext in ['ex','sy','in','gl','db']]+[record('c2/dbglist.c'),record('c2/msvc.pdb')],outputs=outputs,
            toolchain_dependencies=[dict(path='toolchains/1.0-nt/bin/C23.ERR',sha256=sha(idx.wt.TC/'bin/C23.ERR'))])
        # Recreate the actual pre-backend dependency state and require all outputs.
        for p in work.iterdir():p.unlink()
        for row in phase['inputs']:shutil.copyfile(stage/row['path'],work/Path(row['path']).name)
        env['MSC_CMD_FLAGS']=portable.format(bin=windows(idx.wt.TC/'bin'))
        invoke('standalone',[windows(Path('build/debug-run.exe')),windows(idx.wt.TC/'bin/C23232.EXE'),'',*[windows(scratch/('standalone.'+name)) for name in ['stdout','stderr','events']],windows(work)])
        assert (scratch/'standalone.events').read_text()=='exit 0\n'
        assert not (scratch/'standalone.stdout').read_bytes() and not (scratch/'standalone.stderr').read_bytes()
        assert {p.name for p in work.iterdir()}=={Path(row['path']).name for row in phase['inputs']}|{row['output_name'] for row in outputs}
        for row in outputs:assert normalized((work/row['output_name']).read_bytes(),row['ignored_fields'])==normalized((stage/row['path']).read_bytes(),row['ignored_fields']),row['output_name']
        for name in ['stdout','stderr','events']:shutil.copyfile(scratch/('standalone.'+name),stage/('capture/standalone.'+name))
        dependencies=[idx.wt.TC/'bin'/name for name in ['CL.EXE','CL.ERR','C13232.EXE','C23232.EXE','C1.ERR','C23.ERR']]
        manifest=dict(project='vc1.0',case=fixture.name,description='Original CL '+' '.join(arguments),source=record('source/dbglist.c'),
            capture=dict(helper='decomp/vc1.0/c2/src/capture-cl.c',helper_sha256=sha('src/capture-cl.c'),helper_executable_sha256=sha('build/capture-cl.exe'),cl_arguments=arguments,
                environment={k:env[k] for k in ['INCLUDE','LIB','WINEPATH']},canonical_working_directory=windows(work),temporary_directory=windows(work),
                frontend=dict(application=(raw/'spawn-1.application').read_text(),command_line=(raw/'spawn-1.command').read_text(),environment_hex=(raw/'spawn-1.environment').read_bytes().hex(),cwd_parameter=(raw/'spawn-1.cwd').read_text()),
                dependencies=[dict(path=str(p.relative_to(idx.wt.ROOT)),sha256=sha(p)) for p in dependencies],pdb_boundary='Exact pre-C2 spawn snapshot; never substitute final pipeline bytes for earlier input'),phases={'c2':phase})
        (stage/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        validation=dict(timing=timings,total_seconds=time.perf_counter()-started,standalone='exit0; empty streams; all object/listing/PDB artifacts match pipeline',
            pdb_unchanged_by_backend=pdb_unchanged,wine_prefix=str(idx.wt.PREFIX),canonical_working_directory=windows(work),capture_lock=windows(C2_SCRATCH/'capture.lock'),normalization='Only COFF TimeDateStamp4..7; listing/PDB byte exact')
        (stage/'capture-validation.json').write_text(json.dumps(validation,indent=2)+'\n')
        for name in ['capture','expected','c2']:shutil.move(str(stage/name),str(fixture/name))
        for name in ['manifest.json','capture-validation.json']:shutil.copyfile(stage/name,fixture/name)
        (fixture/'README.txt').write_text('Explicit original CL /Od /G3 /Zi /FAcs capture.\nActual five IR inputs plus source and pre-C2 PDB dependency are frozen.\nStandalone C2 reproduces object, annotated listing and PDB; only COFF timestamp normalized.\nNative fixtures never invoke Wine.\n')
        print(json.dumps(dict(fixture=str(fixture),outputs=outputs,validation=validation),indent=2))

if __name__=='__main__':main()
