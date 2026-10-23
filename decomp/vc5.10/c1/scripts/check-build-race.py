#!/usr/bin/env python3
"""A rejected source-race build must not survive for a later Make no-op."""
import importlib.util,json,tempfile,time
from pathlib import Path
import provenance

def main():
    checker=Path(__file__).with_name('build-native-recorded.py')
    spec=importlib.util.spec_from_file_location('recorded_build',checker);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='build-race-control-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
        root=Path(temporary);source=root/'input.c';source.write_text('int unchanged;\n');(root/'build/native').mkdir(parents=True)
        (root/'Makefile').write_text(".PHONY: native\nnative:\n\tpython3 -c 'from pathlib import Path; Path(\"build/native/c1-host\").write_bytes(b\"unverified generated payload\"); Path(\"input.c\").write_text(\"int changed_during_build;\\n\")'\n")
        module.PROJECT=root;module.ROOT=root;module.native_sources=lambda:[source]
        provenance.ROOT=root;provenance.STORE=root/'source-cache'
        try:module.main()
        except AssertionError as error:assert 'rejected ELF removed'in str(error)
        else:raise AssertionError('accepted changed input')
        assert not(root/'build/native/c1-host').exists() and not(root/'build/native-build.json').exists()
    print(json.dumps(dict(status='PASS',fresh_controls=1,scope='Actual Make process changes a recorded input while producing an unverified output; recorder rejects and removes output so a later no-op cannot certify it. No reconstruction target executed.',seconds=time.perf_counter()-started,checker_sha256=provenance.sha(checker)),indent=2))
if __name__=='__main__':main()
