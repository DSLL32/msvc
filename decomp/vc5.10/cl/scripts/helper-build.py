#!/usr/bin/env python3
"""Freeze the original-only debugger's actual compiled source closure."""
import hashlib
import json
from pathlib import Path

project=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
files=['scripts/reference-debugger.c','scripts/dependency-observer.inc','scripts/clock-observer.inc','scripts/mre-observer.inc','scripts/mre-file-observer.inc','scripts/mre-sort-observer.inc','scripts/export-reader.h']
receipt={'binary':{'path':'build/reference-debugger.exe','sha256':sha(project/'build/reference-debugger.exe')},
    'sources':{name:sha(project/name)for name in files},
    'compiler':{'path':'toolchains/5.10/bin/CL.EXE','sha256':sha(project.parents[2]/'toolchains/5.10/bin/CL.EXE')}}
(project/'build/reference-debugger-build.json').write_text(json.dumps(receipt,indent=2)+'\n')
