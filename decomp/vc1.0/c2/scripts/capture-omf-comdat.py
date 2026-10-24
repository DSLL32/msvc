"""Explicit original-only derived OMF case; never called by native fixtures.

CL's ordinary /omf and /Bd /omf probes did not forward the internal-default
switch. Capture untouched CL's genuine C1 IR and C2 environment, then append
only the independently proved original C2 -omf option. No IR/global patches.
"""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,windows


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


from omf_records import records


def main():
    started=time.perf_counter();fixture=Path('../fixtures/c2-omf-comdat-communal').resolve();assert not(fixture/'manifest.json').exists(),'Never overwrite frozen evidence'
    source=fixture/'source/packets.c'
    with capture_lock():
        scratch=Path(tempfile.mkdtemp(prefix='omf-comdat-capture-',dir=C2_SCRATCH));raw=scratch/'raw';raw.mkdir();stage=scratch/'fixture';print('Capture scratch:',scratch,flush=True)
        for n in ['source','capture','c2','expected']:(stage/n).mkdir(parents=True)
        shutil.copyfile(source,stage/'source/packets.c');work=C2_SCRATCH/'packets';work.mkdir(exist_ok=True)
        def clear():
            for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
        clear();shutil.copyfile(source,work/source.name);env=idx.wt.environment()
        for n in ['MSC_CMD_FLAGS','LINK','_LINK_']:env.pop(n,None)
        timings={}
        def invoke(label,command):
            begin=time.perf_counter();r=subprocess.run(['wine',*map(str,command)],cwd=work,env=env,stdin=subprocess.DEVNULL,capture_output=True,timeout=90)
            for stream in ['stdout','stderr']:(stage/'capture'/(label+'-supervisor.'+stream)).write_bytes(getattr(r,stream))
            timings[label]=dict(seconds=time.perf_counter()-begin,returncode=r.returncode,command=list(map(str,command)));(stage/'capture'/('timing-'+label+'.json')).write_text(json.dumps(timings[label],indent=2)+'\n');assert r.returncode==0,(label,r.stdout,r.stderr)
            return r
        arguments=['/nologo','/c','/O2','/Ob0','/Gy','/G5','/Z7','packets.c']
        r=invoke('pipeline',[windows(Path('build/capture-cl.exe')),windows(idx.wt.TC/'bin/CL.EXE'),'11000',windows(raw),windows(work),' '.join(arguments),'packets.obj','-'])
        assert r.stdout==b'packets.c\r\nexit 0; captured 2 launches\r\n'and not r.stderr
        assert {p.name for p in work.iterdir()}=={'packets.c','packets.obj'}
        assert (work/'packets.obj').read_bytes()[:2]==b'L\1','Baseline must remain actual original CL COFF output'
        shutil.copyfile(work/'packets.obj',stage/'capture/baseline-coff.obj')
        for ext in ['ex','sy','in','gl','db']:shutil.copyfile(raw/('c2-input.'+ext),stage/('c2/ir'+ext))
        shutil.copyfile(source,stage/'c2/packets.c')
        for p in raw.iterdir():
            if not p.name.startswith('c2-input'):shutil.copyfile(p,stage/'capture'/p.name)
        entries=(raw/'spawn-2.environment').read_bytes().rstrip(b'\0').split(b'\0');assert len(entries)==1 and entries[0].startswith(b'MSC_CMD_FLAGS=')
        flags=entries[0][14:].decode('ascii');assert '-omf'not in flags and '-Zi 'in flags
        portable=re.sub(r'-il [^ ]+ ','-il ir ',flags.replace(windows(idx.wt.TC/'bin'),'{bin}'));assert portable.endswith(' ');derived=portable+'-omf '
        def record(path):return dict(path=path,bytes=(stage/path).stat().st_size,sha256=sha(stage/path))
        inputs=[record('c2/ir'+ext)for ext in ['ex','sy','in','gl','db']]+[record('c2/packets.c')]
        def prepare():
            clear()
            for row in inputs:shutil.copyfile(stage/row['path'],work/Path(row['path']).name)
        def standalone(label):
            prepare();env['MSC_CMD_FLAGS']=derived.format(bin=windows(idx.wt.TC/'bin'))
            invoke(label,[windows(Path('build/debug-run.exe')),windows(idx.wt.TC/'bin/C23232.EXE'),'',*[windows(stage/'capture'/(label+'.'+n))for n in ['stdout','stderr','events']],windows(work)])
            assert (stage/'capture'/(label+'.events')).read_text()=='exit 0\n'
            assert not(stage/'capture'/(label+'.stdout')).read_bytes()and not(stage/'capture'/(label+'.stderr')).read_bytes()
            assert {p.name for p in work.iterdir()}=={Path(r['path']).name for r in inputs}|{'packets.obj'}
            data=(work/'packets.obj').read_bytes();return data,records(data)
        first,omf_records=standalone('standalone');(stage/'expected/packets.obj').write_bytes(first)
        families={n:sum(r['type']in tags for r in omf_records)for n,tags in {'COMDAT':['0xc2','0xc3'],'CEXTDEF':['0xbc'],'LINSYM':['0xc4','0xc5'],'COMDEF':['0xb0'],'LIDATA':['0xa2','0xa3']}.items()}
        print('Actual original OMF families:',families,flush=True)
        (stage/'capture/actual-families.json').write_text(json.dumps(families,indent=2)+'\n')
        assert families['COMDAT']and families['CEXTDEF'],'Gy did not produce actual COMDAT/CEXTDEF; retain attempt and adjust before freeze'
        second,repeat_records=standalone('repeat');assert first==second and omf_records==repeat_records,'Unexplained clock/path-dependent OMF output'
        # Observe real parser result/branch on an untouched original, read-only.
        prepare();pe=idx.PE(idx.wt.TC/'bin/C23232.EXE');plan=stage/'capture/omf-entry-plan.txt';plan.write_text('%x %02x 256 _omf_open_module\n'%(0x6a350-pe.base,pe.read(0x6a350,1)[0]));env.update(C2_CRT_CAPTURE=windows(stage/'capture/omf-entry.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY='_omf_open_module')
        invoke('parse-state',[windows(Path('build/reference-stdio-observer.exe')),windows(idx.wt.TC/'bin/C23232.EXE'),'',*[windows(stage/'capture'/('omf-entry.'+n))for n in ['stdout','stderr','events']],windows(work)])
        entry=(stage/'capture/omf-entry.txt').read_text();assert 'enter _omf_open_module 'in entry and '\nomf_entry coff_object 0\n'in entry and (stage/'capture/omf-entry.events').read_text().endswith('exit 99\n')
        output=record('expected/packets.obj');output.update(output_name='packets.obj',ignored_fields=[],normalized_sha256=sha(stage/'expected/packets.obj'))
        phase=dict(target='toolchains/1.0-nt/bin/C23232.EXE',sha256=sha(idx.wt.TC/'bin/C23232.EXE'),arguments=[],environment={'MSC_CMD_FLAGS':derived},environment_inheritance=False,canonical_working_directory=windows(work),working_directory='c2',captured_application=(raw/'spawn-2.application').read_text(),captured_command_line=(raw/'spawn-2.command').read_text(),captured_MSC_CMD_FLAGS=flags,expected_exit=0,expected_stdout='',expected_stderr='',inputs=inputs,outputs=[output],toolchain_dependencies=[dict(path='toolchains/1.0-nt/bin/C23.ERR',sha256=sha(idx.wt.TC/'bin/C23.ERR'))],invocation_derivation=dict(baseline_environment={'MSC_CMD_FLAGS':portable},appended_option='-omf ',reason='Original CL gates ^omf to compiled-in default parsing; user argv /omf and /Bd /omf ignored. Original C2 accepts its own -omf option. Frontend IR/source unchanged; derived standalone invocation, never claimed emitted by CL.'))
        dependencies=[idx.wt.TC/'bin'/n for n in ['CL.EXE','CL.ERR','C13232.EXE','C23232.EXE','C1.ERR','C23.ERR']]
        manifest=dict(project='vc1.0',case=fixture.name,description='Derived original C2 OMF COMDAT/communal/CodeView case from actual CL '+' '.join(arguments),source=record('source/packets.c'),capture=dict(helper='decomp/vc1.0/c2/src/capture-cl.c',helper_sha256=sha('src/capture-cl.c'),helper_executable_sha256=sha('build/capture-cl.exe'),cl_arguments=arguments,environment={k:env[k]for k in ['INCLUDE','LIB','WINEPATH']},canonical_working_directory=windows(work),temporary_directory=windows(work),frontend=dict(application=(raw/'spawn-1.application').read_text(),command_line=(raw/'spawn-1.command').read_text(),environment_hex=(raw/'spawn-1.environment').read_bytes().hex(),cwd_parameter=(raw/'spawn-1.cwd').read_text()),dependencies=[dict(path=str(p.relative_to(idx.wt.ROOT)),sha256=sha(p))for p in dependencies],observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe')),phases={'c2':phase})
        (stage/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');validation=dict(timing=timings,total_seconds=time.perf_counter()-started,omf_records=omf_records,actual_record_families=families,artifacts=['packets.obj'],normalization='None; all OMF records/checksums/debug bytes exact across independent untouched original standalone runs',actual_state='Untouched original C2 reaches omf_open_module with coff_object=0; no writes/injected calls by state observer',wine_prefix=str(idx.wt.PREFIX),canonical_working_directory=windows(work));(stage/'capture-validation.json').write_text(json.dumps(validation,indent=2)+'\n')
        for n in ['capture','expected','c2']:shutil.move(str(stage/n),str(fixture/n))
        for n in ['manifest.json','capture-validation.json']:shutil.copyfile(stage/n,fixture/n)
        (fixture/'README.txt').write_text('Derived standalone original C2 -omf case, NOT emitted by CL.\nFive actual original C1 IR inputs plus owned source; baseline CL /O2 /Ob0 /Gy /G5 /Z7 produces COFF.\nOnly supported backend -omf appended; parser state observed, complete OMF object repeats byte exact, all record checksums valid.\nNo PDB/side artifact produced; no normalization. Native fixtures never invoke Wine.\n')
        print(json.dumps(dict(fixture=str(fixture),output=output,validation=validation),indent=2))

if __name__=='__main__':main()
