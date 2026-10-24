"""Derived immutable binding for the three retained next-block observer files."""
import json
from pathlib import Path
from state import BINARY,sha

REVIEW=BINARY/'cases/hello-debug-dll/evidence/0113-next-hash-block-observed/chain-review.json'
EXTRAS=('hash-chain-gdb.py','first-next-block-config.json','first-next-block-state.json')


def bind(directory,config):
    receipt=json.loads((directory/'receipt.json').read_text());state=json.loads((directory/'state.json').read_text());review=json.loads(REVIEW.read_text())
    records={row['path']:row['sha256']for row in review['files']}
    outputs={row['name']:row['sha256']for row in receipt['outputs']}
    assert outputs['state.json']==sha(directory/'state.json') and outputs['gdb-state.py']==sha(directory/'gdb-state.py'),'native observer actual state/driver output changed'
    assert (directory/'configuration.json').read_bytes()==(json.dumps(config,indent=2)+'\n').encode(),'native observer actual trial differs'
    assert receipt['configuration_sha256']==sha(directory/'configuration.json'),'native observer receipt trial differs'
    assert sha(directory/'gdb-state.py')==records['scripts/gdb-state.py'],'native observer executed GDB driver differs from reviewed source'
    assert sha(directory/'hash-chain-gdb.py')==records['scripts/hash-chain-gdb.py'],'native observer executed chain module differs from reviewed source'
    assert sha(directory/'hash-chain-gdb.py')==records['cases/hello-debug-dll/evidence/0113-next-hash-block-observed/native/hash-chain-gdb.py'],'native observer reviewed module artifact differs'
    phase_config=json.loads((directory/'first-next-block-config.json').read_text());assert phase_config==dict(config,observe_next_hash_output_block=False),'native observer first-next exact phase configuration differs'
    phase=json.loads((directory/'first-next-block-state.json').read_text());assert phase==state['hash_chain_entry_phase'],'native observer first-next state differs from actual captured phase'
    anchor=state['hash_chain']['phase'];call=state['hash_chain']['calls'][0]
    assert phase['function']=='next_hash_output_block' and phase['pc']==anchor['typed_pc'] and phase['entry_esp']==anchor['typed_esp'] and call['entry_pc']==anchor['physical_pc'] and call['entry_esp']==anchor['physical_esp'],'native observer first-next physical/typed anchor differs'
    # Paths here are historical command provenance only. Load every file from
    # the owned supplied directory; never reopen an old scratch/capture path.
    transport=Path(state['hash_chain']['directory']);commands=(directory/'commands.gdb').read_text();driver=(directory/'gdb-state.py').read_text()
    assert 'helper.install('+repr(str(transport/'configuration.json'))+')'in commands and 'helper.capture('+repr(str(transport/'configuration.json'))+','+repr(str(transport/'data.json'))+','+repr(str(transport/'state.json'))+')'in commands,'native observer actual commands/phase directory differ'
    assert "Path(configuration).parent/'hash-chain-gdb.py'"in driver and 'hash_chain_module.install(globals(),config,configuration)'in driver,'native observer actual module import differs'
    present=[name in outputs for name in EXTRAS]
    assert all(present)or not any(present),'native observer incomplete receipt extra inventory'
    inventory=receipt.get('observer_inventory',1)
    if inventory==2:
        assert all(present),'native observer current receipt omits extras'
    else:assert inventory==1 and not any(present),'native observer unknown inventory version'
    if all(present):assert all(outputs[name]==sha(directory/name)for name in EXTRAS),'native observer receipt extra hash differs'
    files={name:sha(directory/name)for name in EXTRAS+('commands.gdb','gdb-state.py','configuration.json','state.json')}
    return dict(schema=1,kind='derived observer binding; immutable capture receipt unchanged',receipt_inventory=inventory,receipt_sha256=sha(directory/'receipt.json'),files=files,review_path=str(REVIEW.relative_to(BINARY)),review_sha256=sha(REVIEW),validator_sha256=sha(Path(__file__)),scope='Only reviewed executed chain module and exact first-next trial/physical+typed state; no new execution or raw relabel')
