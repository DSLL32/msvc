"""Emit GNU Make dependencies for project-local quoted C includes."""
from pathlib import Path
import re


def headers(path, seen):
    for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"', path.read_text(), re.M):
        header = Path(name)
        if header.is_file() and header not in seen:
            seen.add(header)
            headers(header, seen)
    return seen


if __name__ == '__main__':
    lines = []
    for source in sorted(Path('.').glob('*.c')):
        dependencies = sorted(headers(source, set()))
        lines.append('build/%s.obj: %s' % (source.stem, ' '.join(map(str, dependencies))))
    Path('build/dependencies.mk').write_text('\n'.join(lines) + '\n')
