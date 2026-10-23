#!/usr/bin/env python3
"""Record immutable native compiled-source content at the final link."""
import json
from pathlib import Path
import subprocess
import sys
from common import HERE,ROOT,sha

output=Path(sys.argv[1]);command=sys.argv[3:]
assert sys.argv[2]=='--' and command
paths=set((HERE/'src').glob('*.c'))|set((HERE/'src').glob('*.h'))
paths|={HERE/'scripts'/name for name in ['native-host.c','native-boundary.c','native-fixture.h']}
for folder,pattern in [('src','*.c'),('include','*.h'),('native','*.h'),('native/sys','*.h')]:
    paths.update((ROOT/'lib/recon'/folder).glob(pattern))
contents={str(p.relative_to(ROOT)):p.read_text() for p in sorted(paths)}
status=subprocess.call(command)
if status:raise SystemExit(status)
assert contents=={str(p.relative_to(ROOT)):p.read_text() for p in sorted(paths)},'Native input changed during final compilation/link'
objects={arg:sha(arg) for arg in command if arg.endswith('.o')}
proof={'binary_sha256':sha(output),'command':command,'objects':objects,'compiled_sources':contents}
output.with_suffix('.inputs.json').write_text(json.dumps(proof,indent=2)+'\n')
