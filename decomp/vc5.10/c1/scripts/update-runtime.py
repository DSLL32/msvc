#!/usr/bin/env python3
"""Bind manually identified pending callees to their actual rebuilt MAP entries."""
import json,re
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[1]
def main():
 text=(PROJECT/'build/c1.map').read_text();records=[]
 for row in json.loads((PROJECT/'runtime.json').read_text()):
  symbol=row.get('symbol',row['name'])
  match=re.search(r'^\s+0001:[0-9a-fA-F]+\s+'+re.escape(symbol)+r'\s+([0-9a-fA-F]+)',text,re.M)
  assert match,'missing actual MAP symbol '+symbol
  start=int(match[1],16)
  records.append({'name':row['name'],'start':start,'end':start+1,'pending_extent':True,'evidence':'unaccepted named INT3 entry identity only; not reconstructed extent'})
 (PROJECT/'runtime-rebuilt.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
