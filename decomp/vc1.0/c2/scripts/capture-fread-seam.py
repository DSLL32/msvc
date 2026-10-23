"""Explicit original/native fread observation, outside all fixture execution."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import pe_indexes as index
from binary import COFF, archive, u32


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    before = json.loads(Path('cases/floating-original-942.json').read_text())
    assert sha(Path('functions.json').read_bytes()) == before['config_sha256']
    directory = Path(sys.argv[1]) if len(sys.argv)>1 else Path(tempfile.mkdtemp(prefix='fread-observation-',dir=Path.home()/'.ai-tmp/vc1.0-c2'))
    timing = {}
    def run(label, command, **kwargs):
        if len(sys.argv)>1:
            timing[label] = None
            return (directory/(label+'.log')).read_bytes()
        start = time.perf_counter()
        result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs)
        timing[label] = time.perf_counter()-start
        (directory/(label+'.log')).write_bytes(result.stdout)
        return result.stdout
    wt = index.wt
    obj, exe = directory/'observer.obj', directory/'observer.exe'
    run('compile', ['python3','scripts/wine-tool.py','CL.EXE','/nologo','/I../../../lib/recon/include','/c','/O1','/G3','/Fo'+wt.windows(obj),'src/reference-fread-observer.c'])
    run('link', ['python3','scripts/wine-tool.py','LINK.EXE','/nologo','/subsystem:console','/out:'+wt.windows(exe),wt.windows(obj)])
    original_directory = Path(before['directory'])
    assert sha((original_directory/'C23232.EXE').read_bytes()) == before['masked_sha256']
    env = wt.environment()
    env.update(MSC_CMD_FLAGS=before['environment']['MSC_CMD_FLAGS'].format(bin=wt.windows(wt.TC/'bin')),
               C2_CRT_CAPTURE=wt.windows(directory/'observations'),C2_CRT_IMAGE=wt.windows(wt.TC/'bin/C23232.EXE'))
    run('original', ['wine',str(exe),wt.windows(original_directory/'C23232.EXE'),'',*[wt.windows(directory/n) for n in ('stdout','stderr','events')],wt.windows(original_directory/'work')],env=env,timeout=90)
    observations = (directory/'observations').read_text()
    original_values = dict(line.split(' ',1) for line in observations.splitlines())
    assert original_values['return_va'] == '0006ffd7'
    gdb_script = directory/'observe.gdb'
    gdb_script.write_text('''set pagination off
break recon_fread
run
python
import gdb,json
p=int(gdb.parse_and_eval('p'))
s=int(gdb.parse_and_eval('size'))
n=int(gdb.parse_and_eval('count'))
end
finish
python
r=int(gdb.parse_and_eval('$eax'))
data=bytes(gdb.selected_inferior().read_memory(p,s*n))
print('FREAD_OBSERVATION '+json.dumps(dict(size=s,count=n,result=r,bytes_hex=data.hex())))
end
''')
    native_env = dict(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=env['MSC_CMD_FLAGS'])
    log = run('native',['gdb','-q','--batch','-x',str(gdb_script),str(Path('build/native/C23232.EXE').resolve())],cwd=original_directory/'work',env=native_env)
    native_values = json.loads(next(l.split(' ',1)[1] for l in log.decode().splitlines() if l.startswith('FREAD_OBSERVATION ')))
    record = next(r for r in json.loads(Path('runtime.json').read_text()) if r['name']=='_fread')
    library = wt.TC/'lib'/record['library']
    data = next(d for n,d in archive(library) if n == record['object'])
    text = next(s for s in COFF(data).sections if s['flags'] & 0x20)
    pe = index.PE(wt.TC/'bin/C23232.EXE')
    linked = pe.read(record['start'],record['end']-record['start'])
    relocs = []
    for offset,symbol,kind in text['relocs']:
        assert kind == 20
        target = record['start']+offset+4+u32(linked,offset)
        target &= 0xffffffff
        target_record = next(r for r in (json.loads(Path('runtime.json').read_text())+json.loads(Path('functions.json').read_text())['driver']) if r['name']==symbol['name'])
        assert target == target_record['start']
        relocs.append(dict(offset=offset,symbol=symbol['name'],kind=kind,target=target))
    receipt = dict(logical='_fread',target_sha256=before['target_sha256'],before_config_sha256=before['config_sha256'],
        original=dict(identity=dict(record=record,library_sha256=sha(library.read_bytes()),object_sha256=sha(data),text_sha256=sha(text['data']),exact_text_bytes=len(text['data']),linked_text_sha256=sha(linked),relocations=relocs),observations=observations,events=(directory/'events').read_text(),observer_source_sha256=sha(Path('src/reference-fread-observer.c').read_bytes()),observer_exe_sha256=sha(exe.read_bytes())),
        native=dict(executable_sha256=sha(Path('build/native/C23232.EXE').read_bytes()),log_sha256=sha(log)),
        observed_original=dict(size=int(original_values['size']),count=int(original_values['count']),result=int(original_values['result']),bytes_hex=original_values['bytes']),observed_native=native_values,
        caller_result_use='ignored; complete 12-byte destination is copied to Extended return value',
        caller='input_extended 0x6ffd7..0x6fff8: copies all three DWORDs, hidden structure-result pointer; stdcall RET4',
        observer_source='src/reference-fread-observer.c',native_log='cases/floating-fread-native.txt',timing=timing,
        reproduce='At captured 942 metadata, run python3 scripts/capture-fread-seam.py explicitly; requires frozen original-only masked capture work path and canonical/native builds, never called by fixtures.',
        limits=['CRT/library acceptance, not reconstructed application source','First 12-byte record only; no claim of exhaustive fread error/EOF behavior','Restores the complete indexed352-byte linked library extent, with both relocation targets verified separately'])
    assert receipt['observed_original'] == receipt['observed_native']
    Path('cases/floating-fread-native.txt').write_bytes(log)
    Path('cases/floating-crt-fread.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))


if __name__ == '__main__':
    main()
