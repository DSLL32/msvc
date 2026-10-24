"""Describe the existing typed callback tables to the shared host auditor."""
import json
from pathlib import Path

BINARY=Path(__file__).resolve().parents[1]


def metadata(config):
    tables=[]
    for table in config['tables']:
        if table['kind']!='function_pointers':continue
        count=len(table['functions'])
        tables.append(dict(label=table['name'],original=[table['original'],table['original']+count*4],
            rebuilt_begin=table['name'],count=count,require_accepted=False))
    assert tables,'missing declared callback tables'
    return dict(schema=1,functions={},callback_tables=tables)


def main():
    import sys
    destination=Path(sys.argv[1])
    destination.write_text(json.dumps(metadata(json.loads((BINARY/'.build/functions.json').read_text())),indent=2)+'\n')


if __name__=='__main__':main()
