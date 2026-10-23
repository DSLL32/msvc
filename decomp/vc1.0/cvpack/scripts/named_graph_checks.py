"""The known named-type switch filler/extra JMP must remain accounted for."""
from state import BINARY
from binary import PE
from audit import audited_graph,publics

def check(config,original,reject):
    record=next(r for r in config['functions']if r['name']=='find_named_type')
    start,end=record['extent'];audited_graph(original,start,end,padding=record['alignment_padding'])
    reject('named lookup undeclared original flag-writing filler',lambda:audited_graph(original,start,end))
    rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();start=linked[record['symbol']];end=min(v for v in linked.values()if v>start)
    audited_graph(rebuilt,start,end,roots=[start+offset for offset in record['rebuilt_extra_roots']])
    reject('named lookup missing explicit canonical switch-exit JMP root',lambda:audited_graph(rebuilt,start,end))


def check_global(config,original,reject):
    record=next(r for r in config['functions']if r['name']=='find_global_type')
    start,end=record['extent'];audited_graph(original,start,end)
    rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();start=linked[record['symbol']];end=min(v for v in linked.values()if v>start)
    audited_graph(rebuilt,start,end,roots=[start+offset for offset in record['rebuilt_extra_roots']])
    reject('global lookup missing explicit canonical loop-step JMP root',lambda:audited_graph(rebuilt,start,end))
