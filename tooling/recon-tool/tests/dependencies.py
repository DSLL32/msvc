#!/usr/bin/env python3
"""Check Windows include resolution without running an original toolchain."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile


def main():
    scratch = Path.home() / '.ai-tmp'
    scratch.mkdir(exist_ok=True)
    scanner = Path(__file__).resolve().parents[1] / 'original-deps.py'
    repo = Path(os.environ.get('RECON_TOOL_TEST_REPO', scanner.parents[2]))
    with tempfile.TemporaryDirectory(prefix='recon-deps-', dir=scratch) as temporary:
        root = Path(temporary)
        source, sdk = root / 'src', root / 'SDK'
        (source / 'Mixed Dir').mkdir(parents=True)
        (sdk / 'Sys').mkdir(parents=True)
        (source / 'main.c').write_text(
            '#include <stdio.h>\n#include "local.h"\n'
            '#include "mixed dir\\leaf.h"\n#include <generated.h>\n')
        (source / 'LOCAL.H').write_text('#include <stdio.h>\n')
        (source / 'Mixed Dir' / 'LEAF.H').write_text('')
        (sdk / 'STDIO.H').write_text(
            '#include <sys\\types.h>\n#include "../src/local.h"\n')
        (sdk / 'Sys' / 'TYPES.H').write_text('')
        (sdk / 'local.h').write_text('')
        output = root / 'deps.mk'
        command = [sys.executable, str(scanner), '--object-root', 'obj',
                   '--output', str(output), '--include', str(sdk), 'src/main.c']
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        dependencies = output.read_text()
        expected = [source / 'main.c', source / 'LOCAL.H',
                    source / 'Mixed Dir' / 'LEAF.H', sdk / 'STDIO.H',
                    sdk / 'Sys' / 'TYPES.H']
        for path in expected:
            assert str(path).replace(' ', r'\ ') in dependencies, path
        assert str(sdk / 'local.h') not in dependencies
        assert dependencies.count(str(source / 'LOCAL.H')) == 3
        for collision, directory in [(sdk / 'stdio.h', False), (sdk / 'SYS', True)]:
            output.unlink(missing_ok=True)
            if directory:
                collision.mkdir()
            else:
                collision.write_text('')
            result = subprocess.run(command, cwd=root, capture_output=True, text=True)
            assert result.returncode == 2 and 'ambiguous Windows include path' in result.stderr
            assert not output.exists()
            if directory:
                collision.rmdir()
            else:
                collision.unlink()
        # Use actual shared header layout, with the actual distinct search roots.
        (source / 'header-split.c').write_text('#include <recon.h>\n')
        shared = repo / 'lib/recon'
        command = [sys.executable, str(scanner), '--object-root', 'obj',
                   '--output', str(output), '--include', str(shared / 'include'),
                   'src/header-split.c']
        subprocess.run(command, cwd=root, check=True)
        dependencies = output.read_text()
        for name in ['recon.h', 'trace-abi.h', 'recon-inline.h']:
            assert str(shared / 'include' / name) in dependencies, name
        assert 'recon-remaps.h' not in dependencies and 'win32.h' not in dependencies
        # No conditional evaluator or special exclusion: if made available in
        # the supplied search roots, the guarded native include is still scanned.
        command[-1:-1] = ['--include', str(shared / 'native')]
        subprocess.run(command, cwd=root, check=True)
        dependencies = output.read_text()
        assert str(shared / 'native/recon-remaps.h') in dependencies
        assert str(shared / 'include/win32.h') in dependencies
    print('PASS Windows include dependencies, search order, cycles and ambiguity rejection')


if __name__ == '__main__':
    main()
