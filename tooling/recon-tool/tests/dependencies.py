#!/usr/bin/env python3
"""Check Windows include resolution without running an original toolchain."""
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    scratch = Path.home() / '.ai-tmp'
    scratch.mkdir(exist_ok=True)
    scanner = Path(__file__).resolve().parents[1] / 'original-deps.py'
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
    print('PASS Windows include dependencies, search order, cycles and ambiguity rejection')


if __name__ == '__main__':
    main()
