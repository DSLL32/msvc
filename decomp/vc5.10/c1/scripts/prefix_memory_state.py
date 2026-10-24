"""Write-once native memory snapshots at their actual prefix boundaries."""
import hashlib,json

def validate_pool(native,names,base,caller_return,event_count):
 raw=(native/'pool-final-committed-000.bin').read_bytes();assert len(raw)==65536
 meta=json.loads((native/'pool-final-memory-boundary.json').read_text())
 assert meta==dict(schema=1,boundary='parse_command_line',pc=names['parse_command_line'][0],caller='c1_main',**{'return':caller_return},owner='c1_bss.arenas[0]',base=base,bytes=65536,reads=1,pool_event_count=event_count,sha256=hashlib.sha256(raw).hexdigest())
 assert raw==bytes(65536)

def validate_alias(native,names,base,root,caller_return):
 raw=(native/'include-alias-committed-000.bin').read_bytes();assert len(raw)==65536
 meta=json.loads((native/'include-alias-memory-boundary.json').read_text())
 assert meta==dict(schema=1,boundary='initialize_checksums',pc=names['initialize_checksums'][0],caller='parse_command_line',**{'return':caller_return},owner='c1_bss.arenas[0]',base=base,bytes=65536,reads=1,root=dict(address=root,head=0,size=4),sha256=hashlib.sha256(raw).hexdigest())
 assert base<=root and root-base<=len(raw)-4 and raw==bytes(65536)
 assert(native/'include-alias-root-000.bin').read_bytes()==bytes(4)
