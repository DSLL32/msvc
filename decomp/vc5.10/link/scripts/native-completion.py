#!/usr/bin/env python3
"""Fresh candidate-completion capture: native ELF only, regular streams/fullDWORD."""
import argparse,hashlib,importlib,json,shutil,subprocess,tempfile
from pathlib import Path
from accept import BINARY,bind_capture
from capture_sources import freeze
sha=lambda data:hashlib.sha256(data).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config',type=Path,required=True)
    parser.add_argument('--reference',type=Path,required=True)
    args=parser.parse_args();configuration=json.loads(args.config.read_text())
    bind_capture(args.reference,configuration,'original')
    replay=importlib.import_module('check-native')
    master=replay.completed_master(args.reference,configuration)
    subprocess.run(['make','-q','native'],cwd=BINARY,check=True)
    capture=Path(tempfile.mkdtemp(prefix='native-complete-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/link'))
    content,paths=freeze(capture)
    (capture/'functions.json').write_bytes(args.config.read_bytes());(capture/'runtime.json').write_bytes((BINARY/'runtime.json').read_bytes())
    report=replay.replay(configuration,args.reference,master)
    subprocess.run(['make','-q','native'],cwd=BINARY,check=True)
    assert all(sha(path.read_bytes())==content[name]for path,name in paths),'compiled source changed during completion'
    work=Path(report['work_directory'])
    for path in work.iterdir():
        if path.is_dir():shutil.copytree(path,capture/path.name)
        elif path.name=='receipt.json':shutil.copyfile(path,capture/'run.json')
        else:shutil.copyfile(path,capture/path.name)
    (capture/'state.json').write_text(json.dumps(dict(entries=report['application_trace'],native_pid=report['native_pid'],logical_status=report['logical_status'],posix_status=report['posix_status'],terminal_record_hex=report['terminal_record_hex']),indent=2)+'\n')
    receipt=dict(kind='native-i386-completion',status=0,configuration_sha256=sha(args.config.read_bytes()),native_sha256=sha((BINARY/'build/native/LINK.EXE').read_bytes()),rebuilt_sha256=sha((BINARY/'build/link.exe').read_bytes()),source_files={str(p.relative_to(BINARY)):sha(p.read_bytes())for p in sorted(p for p in (BINARY/'src').iterdir()if p.suffix in ('.c','.h','.cpp'))},source_content=content,reference_receipt_sha256=sha((args.reference/'receipt.json').read_bytes()),elapsed_seconds=report['elapsed_seconds'],files={str(p.relative_to(capture)):dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))for p in capture.rglob('*')if p.is_file()and 'source'not in p.relative_to(capture).parts})
    (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(capture)


if __name__=='__main__':main()
