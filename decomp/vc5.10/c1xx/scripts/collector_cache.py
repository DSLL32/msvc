"""Compile the identical GDB observation body once; never cache target state."""
import ast
import hashlib
import json
from pathlib import Path


def normalize(script):
    if not script.startswith('python\n') or not script.endswith('end\n'):
        raise ValueError('not a standalone Python collector')
    body=script[len('python\n'):-len('end\n')]
    files={}
    for node in ast.walk(ast.parse(body)):
        if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='open':
            if len(node.args)!=2 or not all(isinstance(arg,ast.Constant) and isinstance(arg.value,str) for arg in node.args):
                raise ValueError('collector open is not an explicit output path/mode')
            path,mode=(arg.value for arg in node.args)
            if mode in files or mode not in ('w','wb'):raise ValueError('duplicate/unknown collector output role')
            files[mode]=path
    if set(files)!=set(('w','wb')):raise ValueError('missing collector state/argv output')
    for mode,name in [('w','recon_state_path'),('wb','recon_argv_path')]:body=body.replace(repr(files[mode]),name)
    return body,files['w'],files['wb']


def startup(body_path,digest,registry):
    # This block runs once in GDB. Every observation gets a fresh namespace.
    return '''python
import hashlib
recon_body_path=%r
recon_body_sha=%r
recon_checkpoint_paths=%r
recon_body_bytes=open(recon_body_path,'rb').read()
if hashlib.sha256(recon_body_bytes).hexdigest()!=recon_body_sha:raise ValueError('collector body hash changed')
recon_body_code=compile(recon_body_bytes,recon_body_path,'exec')
def recon_collect_checkpoint(key,digest,state_path,argv_path):
    if digest!=recon_body_sha:raise ValueError('collector wrapper body hash differs')
    if key not in recon_checkpoint_paths:raise ValueError('unknown collector checkpoint')
    if [state_path,argv_path]!=recon_checkpoint_paths[key]:raise ValueError('collector output paths differ')
    namespace={'recon_state_path':state_path,'recon_argv_path':argv_path,'recon_collect_timings':recon_collect_timings}
    exec(recon_body_code,namespace)
end
'''%(str(body_path),digest,registry)


def prepare(output,collector):
    output=Path(output).resolve();expected,_,_=normalize(collector.read_text());digest=hashlib.sha256(expected.encode()).hexdigest();registry={};wrappers=[];used=set()
    for path in sorted(output.glob('collect*.gdb')):
        normalized,state,argv=normalize(path.read_text())
        if normalized!=expected:raise ValueError('collector body differs at '+path.name)
        for value in (state,argv):
            if Path(value).parent!=output or value in used:raise ValueError('collector output path escapes or aliases another checkpoint')
            used.add(value)
        registry[path.name]=[state,argv];wrappers.append((path,state,argv))
    body_path=output/'collector-body.py';body_path.write_text(expected)
    for path,state,argv in wrappers:
        path.write_text('python\nrecon_collect_checkpoint(%r,%r,%r,%r)\nend\n'%(path.name,digest,state,argv))
    (output/'collector-cache.json').write_text(json.dumps({'schema':1,'body_sha256':digest,'checkpoints':registry,'scope':'one GDB process; fresh execution namespace at every observation; no target state/address caching'},indent=2)+'\n')
    return startup(body_path,digest,registry)
