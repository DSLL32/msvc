"""One checked scalar-context type refinement, never a general replay waiver."""
import hashlib,json,re,struct
from pathlib import Path
from allocation_imports import pe_read

P=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(value,message):
    if not value:raise ValueError('options ABI refinement: '+message)


def sources(directory):
    result={};pending=[('final-link',json.loads((directory/'build-provenance.json').read_text()))]
    while pending:
        owner,item=pending.pop()
        consumed=[v for v in item['command']if isinstance(v,str)and v.endswith(('.c','.cpp'))]
        for name,digest in item['source_content'].items():
            path=directory/'source-content'/digest;require(sha(path)==digest,'compiled source content changed')
            if name.endswith('.build.json'):pending.append((name,json.loads(path.read_text())))
            elif (name.endswith('.h') and ('/src/'in name or name.startswith('lib/recon/') or owner=='final-link'and name.endswith('/scripts/invoke-arguments.h'))) or any(name.endswith('/'+v)or name==v for v in consumed):
                # record-build conservatively snapshots unrelated target files
                # too. Only the command's actual TUs and headers are consumed;
                # each object keeps its own immutable historical header set.
                key=owner+'::'+name
                require(key not in result,'duplicate consumed source identity')
                result[key]=(digest,path.read_bytes())
    return result


def normalized_image(path):
    raw=bytearray(path.read_bytes());pe=struct.unpack_from('<I',raw,0x3c)[0];stamp=struct.unpack_from('<I',raw,pe+8)[0]
    export=struct.unpack_from('<I',raw,pe+24+96)[0];entry=pe_read(path,export,40);require(struct.unpack_from('<I',entry,4)[0]==stamp,'export/header clock mismatch')
    count=struct.unpack_from('<H',raw,pe+6)[0];size=struct.unpack_from('<H',raw,pe+20)[0];offsets=[pe+8]
    for index in range(count):
        pos=pe+24+size+40*index;rva,rawsize,filepos=struct.unpack_from('<III',raw,pos+12)
        if rva<=export and export+40<=rva+rawsize:offsets.append(filepos+export-rva+4)
    require(len(offsets)==2,'export directory backing differs')
    for offset in offsets:raw[offset:offset+4]=bytes(4)
    return bytes(raw),stamp,offsets


def check(original,native):
    packet=json.loads((P/'cases/minimal/options-trial.json').read_text());records=packet.get('pre_refinement_proofs',packet['proofs']);matches=[v for v in records.values()if v['original_receipt_sha256']==sha(original/'receipt.json')]
    require(len(matches)==1,'original is not an explicitly frozen pre-refinement pair');record=matches[0];old=Path(record['native'])
    require(sha(original/'receipt.json')==record['original_receipt_sha256'] and sha(old/'receipt.json')==record['native_receipt_sha256'],'old role receipt changed')
    before=json.loads((original/'trial.json').read_text());after=json.loads((native/'trial.json').read_text());prior=json.loads((old/'trial.json').read_text())
    require(before==prior and before['trial']=='_process_compiler_options','old execution/trial source pair differs')
    allowed={'build/c1n32_l.dll','build/c1xx.map','build/native/c1xx-host'}
    cloned=json.loads(json.dumps(after))
    for name in allowed:cloned['bound_inputs'][name]=before['bound_inputs'][name]
    require(cloned==before,'case/controller/config/mask/index/layout/metadata changed')
    for path,key in [('c1n32_l.dll','build/c1n32_l.dll'),('c1xx.map','build/c1xx.map'),('native-executable.elf','build/native/c1xx-host')]:
        require(sha(old/path)==before['bound_inputs'][key] and sha(native/path)==after['bound_inputs'][key],'actual compiled image/MAP binding differs')
    left,oldstamp,offsets=normalized_image(old/'c1n32_l.dll');right,newstamp,new_offsets=normalized_image(native/'c1n32_l.dll')
    require(left==right and offsets==new_offsets,'emitted complete Win32 DLL changed beyond identified clock fields')
    def normalized_map(path,stamp):
        raw=path.read_bytes();lines=raw.splitlines(keepends=True);rows=[i for i,v in enumerate(lines)if re.fullmatch(rb' Timestamp is [0-9a-f]{8} \([^\r\n]+\)\r?\n',v)]
        require(len(rows)==1 and int(re.search(rb'is ([0-9a-f]{8})',lines[rows[0]])[1],16)==stamp,'MAP clock/header mismatch');lines[rows[0]]=b'IDENTIFIED LINK TIMESTAMP\n';return b''.join(lines)
    require(normalized_map(old/'c1xx.map',oldstamp)==normalized_map(native/'c1xx.map',newstamp),'MAP symbols/layout changed beyond clock')
    a,b=sources(old),sources(native);require(set(a)==set(b),'compiled source inventory changed')
    wanted='decomp/vc5.10/c1xx/build/native/options.o.build.json::decomp/vc5.10/c1xx/src/options.c';require({name for name in a if a[name][0]!=b[name][0]}=={wanted},'compiled source changed beyond pending scalar-context prototype')
    source=a[wanted][1];require(source.count(b'push_declaration_context(void *context)')==2 and source.count(b'push_declaration_context(NULL);')==1,'old source transformation premise changed')
    corrected=source.replace(b'push_declaration_context(void *context)',b'push_declaration_context(int context)').replace(b'push_declaration_context(NULL);',b'push_declaration_context(0);')
    require(corrected==b[wanted][1],'not exact two-prototype/literal-zero type-only correction')
    target=original/'loaded-target.bin';require(pe_read(target,0x38fb7,7)==bytes.fromhex('6a00e87628ffff'),'actual original scalar-zero push/CALL changed')
    require(pe_read(target,0x2b848,18)==bytes.fromhex('8b0d80d54c108b44240489048dd0134c10c3'),'actual original callee stack4/store/RET0 differs')
    result=dict(kind='checked scalar-context formal/source-only native refresh against unchanged original execution',original_trial_sha256=sha(original/'trial.json'),native_trial_sha256=sha(native/'trial.json'),original_receipt_sha256=sha(original/'receipt.json'),prior_native=str(old),prior_native_receipt_sha256=sha(old/'receipt.json'),source_before=a[wanted][0],source_after=b[wanted][0],allowed_bound_inputs=sorted(allowed),Win32_complete_image_identical_except_clock_fields=offsets,Win32_MAP_identical_except_clock=True,source_formal_before='void *context',source_formal_after='int context',actual_scalar_argument=0,actual_stack_bytes=4,helper_body_still_INT3=True)
    return result
