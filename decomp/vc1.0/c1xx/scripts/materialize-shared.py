"""Materialize reviewed shared source pins; generated files stay untracked."""
from pathlib import Path
import subprocess, sys
base, tracer, destination = sys.argv[1:]
repo = Path(__file__).resolve().parents[4]
destination = Path(destination)
for pin, paths in [(base, ['lib/recon/include', 'lib/recon/native', 'lib/recon/src']),
                   (tracer, ['lib/recon/src/trace.c', 'lib/recon/include/recon-trace.h'])]:
    rows = subprocess.check_output(['git', 'ls-tree', '-r', '-z', pin, '--', *paths], cwd=repo)
    for row in rows.split(b'\0'):
        if not row: continue
        identity, path = row.split(b'\t', 1)
        mode, kind, oid = identity.decode().split()
        assert kind == 'blob' and mode in ('100644', '100755')
        path = Path(path.decode())
        if path.suffix not in ('.c', '.h', '.inc'): continue
        data = subprocess.check_output(['git', 'cat-file', 'blob', oid], cwd=repo)
        output = destination / path.relative_to('lib/recon')
        output.parent.mkdir(parents=True, exist_ok=True)
        if not output.exists() or output.read_bytes() != data: output.write_bytes(data)
(destination / '.stamp').write_text(base + '\n' + tracer + '\n')
