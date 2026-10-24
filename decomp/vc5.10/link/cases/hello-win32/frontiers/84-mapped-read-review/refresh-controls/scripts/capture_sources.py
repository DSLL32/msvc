"""Freeze the actual compiled target/shared source bytes for capture provenance."""
import hashlib
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]


def freeze(capture):
    content={};paths=[]
    sources=[]
    for category,directory in [('project',BINARY/'src'),('recon',BINARY.parents[2]/'lib/recon')]:
        for source in sorted(p for p in directory.rglob('*')if p.is_file()and p.suffix in ('.c','.h','.cpp')):
            if category=='recon' and source.relative_to(directory).parts[0]not in ('src','include','native'):continue
            sources.append((source,category+'/'+str(source.relative_to(directory))))
    sources.append((BINARY/'scripts/native-host.c','harness/native-host.c'))
    for source,name in sources:
        data=source.read_bytes();destination=capture/'source'/name
        destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data)
        content[name]=hashlib.sha256(data).hexdigest();paths.append((source,name))
    for name,source in [('native-LINK.EXE',BINARY/'build/native/LINK.EXE'),
                        ('original-toolchain-link.exe',BINARY/'build/link.exe'),
                        ('original-toolchain-link.map',BINARY/'build/link.map')]:
        destination=capture/'compiled'/name
        destination.parent.mkdir(parents=True,exist_ok=True)
        destination.write_bytes(source.read_bytes())
    return content,paths
