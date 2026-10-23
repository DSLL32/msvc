"""Persist observed original C++ driver boundaries and replay each phase.

Explicit reference operation only. Native fixtures never import or invoke this.
The input capture root comes from the original CreateProcessA debugger, whose
exact command and helper provenance are retained with each fixture.
"""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
CASES=('c1xx-virtual-diamond','c1xx-copy-assignment','c1xx-stack-lifetime','c1xx-new-delete')


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path,root,output=None,volatile=None):
    data=path.read_bytes()
    result=dict(path=str(path.relative_to(root)),bytes=len(data),sha256=sha(path))
    if output:result['output_name']=output
    if volatile:
        result['ignored_fields']=volatile
        normalized=bytearray(data)
        for field in volatile:
            offset=field['offset'];normalized[offset:offset+field['bytes']]=bytes(field['bytes'])
        result['normalized_sha256']=hashlib.sha256(normalized).hexdigest()
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture_root',type=Path)
    args=parser.parse_args()
    spec=importlib.util.spec_from_file_location('c1xx_reference_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    base=json.loads((BINARY.parent/'fixtures/cpp-basic/manifest.json').read_text())
    env=tool.environment()
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'):env.pop(key,None)
    for name in CASES:
        fixture=BINARY.parent/'fixtures'/name
        source=next((fixture/'c1').glob('*.cpp'))
        stem=source.stem
        captured=args.capture_root/name
        driver=json.loads((captured/'driver-receipt.json').read_text())
        assert driver['status']==driver['program']['status']==0 and not driver['program']['stdout'] and not driver['program']['stderr']
        assert driver['source_sha256']==sha(source)
        for folder in ('capture','c2','link','expected','dependencies','validation'):(fixture/folder).mkdir(exist_ok=True)
        for old in (captured/'capture').iterdir():shutil.copyfile(old,fixture/'capture'/old.name)
        for filename in ('driver-receipt.json','driver.stdout','driver.stderr'):shutil.copyfile(captured/filename,fixture/'capture'/filename)
        shutil.copyfile(args.capture_root/'helper-builds.json',fixture/'capture/helper-builds.json')
        for suffix in ('ex','sy','in','gl'):shutil.copyfile(captured/'capture'/('c2-input.'+suffix),fixture/'c2'/('ir'+suffix))
        shutil.copyfile(captured/'capture/link-input.obj',fixture/'link'/(stem+'.obj'))
        shutil.copyfile(captured/'capture/link-input.obj',fixture/'expected'/(stem+'.obj'))
        shutil.copyfile(captured/'capture/link-input.rsp',fixture/'link/link.rsp')
        shutil.copyfile(captured/(stem+'.exe'),fixture/'expected'/(stem+'.exe'))
        manifest=copy.deepcopy(base)
        manifest['case']='CL.EXE /nologo /Od '+source.name
        manifest['capture'].update(arguments='/nologo /Od '+source.name,downstream_capture_arguments='/nologo /Od '+source.name,
            inherited_working_directory=str(captured),source_sha256=sha(source),raw_boundary_files='capture/',
            program=driver['program'],timing_receipt='capture/driver-receipt.json')
        manifest['capture'].pop('frontend_outputs_identical_with_and_without_driver_c_switch',None)
        for number,phase in ((1,'c1'),(2,'c2'),(3,'link')):
            item=manifest['phases'][phase]
            for suffix,key in [('application','captured_application'),('command','captured_command_line')]:
                item[key]=(captured/'capture'/('spawn-%d.'%number+suffix)).read_bytes().decode('latin1')
            assert not (captured/'capture'/('spawn-%d.cwd'%number)).read_bytes()
            if number<3:
                raw=(captured/'capture'/('spawn-%d.environment'%number)).read_bytes()
                pairs=[x.decode('latin1').split('=',1) for x in raw.split(b'\0') if x]
                assert len(pairs)==1 and pairs[0][0]=='MSC_CMD_FLAGS'
                flags=pairs[0][1];item['captured_MSC_CMD_FLAGS']=flags
                flags=re.sub(r'(-il )\S+',r'\1ir',flags)
                flags=flags.replace(tool.windows(tool.TC/'bin'),'{bin}').replace(tool.windows(tool.TC/'include'),'{include}')
                item['environment']={'MSC_CMD_FLAGS':flags}
            item['expected_stdout']='';item['expected_stderr']=''
        c1,c2,link=(manifest['phases'][p] for p in ('c1','c2','link'))
        c1['inputs']=[record(source,fixture)]
        c1['outputs']=[record(fixture/'c2'/('ir'+s),fixture,'ir'+s) for s in ('ex','sy','in','gl')]
        c2['inputs']=[{k:v for k,v in r.items() if k!='output_name'} for r in c1['outputs']]
        c2['outputs']=[record(fixture/'expected'/(stem+'.obj'),fixture,stem+'.obj',[dict(offset=4,bytes=4,name='COFF TimeDateStamp')])]
        link['captured_LIB']=(captured/'capture/spawn-3.LIB').read_bytes().decode('latin1')
        link['inputs']=[record(fixture/'link'/(stem+'.obj'),fixture),record(fixture/'link/link.rsp',fixture)]
        executable=fixture/'expected'/(stem+'.exe')
        timestamp=struct.unpack_from('<I',executable.read_bytes(),0x3c)[0]+8
        link['outputs']=[record(executable,fixture,stem+'.exe',[dict(offset=timestamp,bytes=4,name='PE COFF TimeDateStamp')])]
        link['generated_program']['path']=stem+'.exe'
        c1['toolchain_dependencies']=[];c2['toolchain_dependencies']=[]
        for phase,filename in ((c1,'bin/C1.ERR'),(c2,'bin/C23.ERR')):
            original=tool.TC/filename
            if not original.exists():original=original.with_name(original.name.lower())
            target=fixture/'dependencies'/original.name
            shutil.copyfile(original,target)
            phase['toolchain_dependencies'].append(dict(path=str(original.relative_to(ROOT)),bytes=original.stat().st_size,sha256=sha(original),cached_path=str(target.relative_to(fixture))))
        if name=='c1xx-new-delete':
            original=tool.TC/'include/NEW.H';target=fixture/'dependencies/NEW.H';shutil.copyfile(original,target)
            c1['toolchain_dependencies'].append(dict(path=str(original.relative_to(ROOT)),bytes=original.stat().st_size,sha256=sha(original),cached_path=str(target.relative_to(fixture))))
        (fixture/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        validation={}
        for phase,item in manifest['phases'].items():
            work=args.capture_root/name/('independent-'+phase);work.mkdir(exist_ok=False)
            for entry in item['inputs']:shutil.copyfile(fixture/entry['path'],work/Path(entry['path']).name)
            phase_env=env.copy()
            phase_env.update({k:v.format(bin=tool.windows(tool.TC/'bin'),include=tool.windows(tool.TC/'include'),lib=tool.windows(tool.TC/'lib')) for k,v in item['environment'].items()})
            command=['wine',str(args.capture_root/'run-phase.exe'),tool.windows(ROOT/item['target']),' '.join(item['arguments']),item['command_line_template'].format(bin=tool.windows(tool.TC/'bin'))]
            started=time.perf_counter();result=subprocess.run(command,cwd=work,env=phase_env,stdin=subprocess.DEVNULL,capture_output=True,timeout=90)
            seconds=time.perf_counter()-started
            (fixture/'validation'/(phase+'-stdout.bin')).write_bytes(result.stdout);(fixture/'validation'/(phase+'-stderr.bin')).write_bytes(result.stderr)
            assert result.returncode==0 and not result.stdout and not result.stderr,(name,phase,result.returncode,result.stdout,result.stderr)
            outputs=[]
            for entry in item['outputs']:
                left=(fixture/entry['path']).read_bytes();right=(work/entry['output_name']).read_bytes()
                assert len(left)==len(right)
                differences=[i for i,(a,b) in enumerate(zip(left,right)) if a!=b]
                permitted={i for f in entry.get('ignored_fields',[]) for i in range(f['offset'],f['offset']+f['bytes'])}
                assert set(differences)<=permitted,(name,phase,entry['output_name'],differences)
                outputs.append(dict(file=entry['output_name'],bytes=len(right),actual_sha256=hashlib.sha256(right).hexdigest(),differing_byte_offsets=differences,permitted_volatile_fields=entry.get('ignored_fields',[])))
            assert {p.name for p in work.iterdir()}=={Path(e['path']).name for e in item['inputs']}|{e['output_name'] for e in item['outputs']}
            validation[phase]=dict(status=0,seconds=seconds,command=command,outputs=outputs,stdout='',stderr='')
            if phase=='link':
                started=time.perf_counter();program=subprocess.run(['wine',str(work/(stem+'.exe'))],cwd=work,env=env,stdin=subprocess.DEVNULL,capture_output=True,timeout=90)
                assert program.returncode==0 and not program.stdout and not program.stderr
                validation['program']=dict(status=0,seconds=time.perf_counter()-started,stdout='',stderr='')
        (fixture/'validation.json').write_text(json.dumps(validation,indent=2)+'\n')
        (fixture/'README.txt').write_text('Original-validated C1XX follow-on case: '+name+'\n\nSource and exact original driver arguments are in manifest.json. Raw child\napplications/command lines/environment blocks and all phase inputs are retained.\nCompiler phases have argv0 only and an explicit MSC_CMD_FLAGS environment.\nIndependent C1XX, C2 and LINK replay passed; the generated program exits0 with\nempty streams. C1XX outputs are exact; only independently demonstrated four-byte\nCOFF/PE timestamps are permitted for downstream replay. Validation JSON records\nactual differing offsets and split elapsed times. This is cached reference data;\nno fixture executes Wine. Reference registration lives in C1XX scripts/.\n')
        print(name+': independent original phases and silent exit0 verified',flush=True)


if __name__=='__main__':main()
