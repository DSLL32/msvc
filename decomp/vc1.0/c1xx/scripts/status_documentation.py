"""Refresh the concise current README status after an accepted checkpoint."""
import json
from pathlib import Path
import re

BINARY=Path(__file__).resolve().parents[1]


def refresh():
    config=json.loads((BINARY/'functions.json').read_text())
    status=json.loads((BINARY/'validation.json').read_text())
    cache=json.loads((BINARY/'cases/native-traces.json').read_text())
    coverage=status['coverage']
    path=BINARY/'README.txt';text=path.read_text()
    text=re.sub(r'PARTIAL binary reconstruction: \d+ accepted frontiers, including \d+ frontend',
        'PARTIAL binary reconstruction: %d accepted frontiers, including %d frontend'%(len(config['accepted']),len(cache['accepted_application'])),text,count=1)
    name=status['active_case'];terminal=config['cases'][name]['terminal']
    address=next((r['start'] for r in config['driver'] if r['name']==terminal),None)
    if address is not None:
        text=re.sub(r'virtual_diamond currently stops at _[A-Za-z_0-9@]+,\noriginal VA 0x[0-9a-f]+\.',
            '%s currently stops at %s,\noriginal VA 0x%x.'%(name,terminal,address),text,count=1)
    text=re.sub(r'Accepted original extents cover \d+ / \d+ code-section bytes \([0-9.]+%\)\.',
        'Accepted original extents cover %d / %d code-section bytes (%.6f%%).'%(coverage['accepted_code_bytes'],coverage['total_executable_code_bytes'],coverage['accepted_code_percent']),text,count=1)
    entry=cache['cases'].get('virtual_diamond')
    if entry:
        text=re.sub(r'then expects \d+ new application entries plus its named terminal trap\.',
            'then expects %d new application entries plus its named terminal trap.'%(len(entry['expected'])-1),text,count=1)
    path.write_text(text)
