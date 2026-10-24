"""Encountered parser storage, with individual typed default/alias witnesses.

This is an interpreted field inventory, not a section snapshot. The original
6548-byte image still has unmodeled fields and no native byte-image analogue.
"""
import struct

# name, original RVA, C type, interpreted default. All pointer defaults below
# have independently identified string storage; other pointers are NULL.
DATA = [
 ('argument_allocation',0x5e390,'char **',0),
 ('packing_stack',0x5e410,'struct CompilerPacking *',0),
 ('include_aliases',0x5e414,'struct CompilerIncludeAliases *',0),
 ('warning_levels',0x5e9ec,'unsigned char[1000]',[0]*1000),
 ('lowest_warning',0x5edd4,'unsigned',1000),
 ('highest_warning',0x5edd8,'unsigned',700),
 ('preprocess_only',0x5eef8,'int',0),
 ('language_extensions',0x5eefc,'int',0),
 ('option_zbe',0x5ef00,'int',0),
 ('japanese_identifiers',0x5ef0c,'int',0),
 ('unsigned_char',0x5ef10,'int',0),
 ('dll_runtime',0x5ef2c,'int',0),
 ('multithread_runtime',0x5ef30,'int',0),
 ('pascal_calls',0x5ef34,'int',0),
 ('debug_info',0x5ef38,'int',0),
 ('pch_debug',0x5ef3c,'int',0),
 ('preprocess_no_lines',0x5ef44,'int',0),
 ('option_pf',0x5ef48,'int',0),
 ('option_bu',0x5ef4c,'int',175),
 ('reproducible',0x5ef50,'int',0),
 ('intrinsics',0x5ef58,'int',0),
 ('stack_checks',0x5ef5c,'int',1),
 ('cdecl_calls',0x5ef68,'int',0),
 ('stdcall_calls',0x5ef6c,'int',0),
 ('pool_strings',0x5ef70,'int',0),
 ('readonly_strings',0x5ef78,'int',0),
 ('code_based_strings',0x5ef80,'int',0),
 ('processor',0x5ef84,'int',0),
 ('minimal_rebuild',0x5ef88,'int',1),
 ('inline_level',0x5ef8c,'int',0),
 ('dependency_output',0x5efa0,'int',0),
 ('optimize_size',0x5efac,'int',1),
 ('assume_no_alias',0x5efb0,'int',0),
 ('assume_no_global_alias',0x5efb4,'int',0),
 ('global_optimize',0x5efb8,'int',0),
 ('precise_float',0x5efbc,'int',0),
 ('omit_frame_pointer',0x5efc0,'int',0),
 ('line_records',0x5efc4,'int',1),
 ('database_path',0x5efc8,'char[256]',[0]*256),
 ('pch_link_path',0x5f0c8,'char[256]',[0]*256),
 ('option_oq',0x5f2cc,'int',0),
 ('option_ov',0x5f2d4,'int',0),
 ('fastcall_calls',0x5ef08,'int',0),
 ('incremental_compile',0x5f2e4,'int',0),
 ('debug_types',0x5f2e8,'int',0),
 ('function_records',0x5f2f0,'int',1),
 ('split_pdbs',0x5f2f4,'int',1),
 ('no_eh_for_new',0x5f2f8,'int',0),
 ('pch_use',0x5f300,'int',0),
 ('pch_use_file',0x5f304,'char *',0),
 ('pch_create',0x5f308,'int',0),
 ('pch_create_file',0x5f30c,'char *',0),
 ('pch_auto',0x5f310,'int',0),
 ('pch_auto_file',0x5f314,'char *',0),
 ('pch_link_name',0x5f318,'char *',0),
 ('pch_file',0x5f31c,'char *',0),
 ('pch_no_preprocess',0x5f334,'int',1),
 ('new_version',0x5f344,'char *',0),
 ('object_file',0x5f354,'char *',0),
 ('database_file',0x5f358,'char *',{'symbol':'c1_default_database','rva':0x62ad4,'text':'vc50.pdb'}),
 ('warning_level',0x5f360,'int',1),
 ('path_separators',0x5f390,'char *',{'symbol':'c1_default_path_separators','rva':0x62ad0,'text':'\\:/'}),
 ('il_prefix',0x5f394,'char *',{'symbol':'c1_empty_string','rva':0x601c1,'text':''}),
 ('packing_alignment',0x5f398,'int',-1),
 ('no_expand_inline',0x5f3b4,'int',0),
 ('unrecognized_argument',0x5f3e0,'char *',0),
 ('debug_old_format',0x5f3e4,'int',0),
 ('move_pch',0x5f974,'int',0),
]
BSS = [
 ('edit_and_continue',0x53ff4,'int',0),
 ('option_zn',0x54010,'int',0),
 ('include_directories',0x54168,'struct CompilerVector *',0),
 ('source_file',0x54170,'char *',0),
 ('option_zf',0x54a58,'int',0),
 ('version_string',0x54b10,'char *',0),
 ('forced_includes',0x54b38,'struct CompilerVector *',0),
 ('show_includes',0x54b3c,'int',0),
 ('defines',0x54c74,'struct CompilerVector *',0),
 ('prototypes_only',0x54c88,'int',0),
 ('preprocessed_file',0x54d9c,'char *',0),
 ('browser_file',0x54de8,'char *',0),
 ('preprocess_stdout',0x54f28,'int',0),
 ('browser_local_file',0x54f2c,'char *',0),
 ('preserve_comments',0x5505c,'int',0),
 ('dump_options',0x55070,'int',0),
 ('option_xc',0x55094,'int',0),
 ('warnings_are_errors',0x550c0,'int',0),
 ('suppress_banner',0x550ec,'int',0),
 ('option_zc',0x550f4,'int',0),
 ('fat',0x550f8,'int',0),
 ('pch_memory_limit',0x55c30,'int',0),
 ('response_file',0x55c40,'char *',0),
 ('error_file',0x57e40,'char *',0),
]

def width(kind):
 return int(kind.rsplit('[',1)[1][:-1]) if '[' in kind else 4
def offsets(rows,start):
 result={}
 for name,rva,kind,default in rows:
  result[name]=start;start+=width(kind)
 return result,start
DATA_OFFSETS,INITIALIZED_SIZE=offsets(DATA,64)
BSS_OFFSETS,BSS_SIZE=offsets(BSS,996)
KNOWN_INITIALIZED_BYTES=INITIALIZED_SIZE
UNKNOWN_INITIALIZED_BYTES=6548-KNOWN_INITIALIZED_BYTES
KNOWN_BSS_BYTES=BSS_SIZE-27
DEFAULTS={name:value for name,rva,kind,value in DATA}
BSS_DEFAULTS={name:value for name,rva,kind,value in BSS}
ALIASES={0x5f2e0:'c1_data.c9_il',0x5f2fc:'c1_data.internal_bm_flags',0x5ef04:'c1_data.internal_zb_value',0x55c34:'c1_bss.debug_break'}

def native_field(name,rows):
 return ('c1_data.' if rows is DATA else 'c1_bss.')+'options.'+name
def targets():
 result=dict(ALIASES)
 for rows in (DATA,BSS):
  for name,rva,kind,default in rows:
   if '[' not in kind:result[rva]=native_field(name,rows)
 return result

def read(raw,rows,base):
 values={}
 for name,rva,kind,default in rows:
  at=rva-base
  values[name]=list(raw[at:at+width(kind)]) if '[' in kind else struct.unpack_from('<i' if kind=='int' else '<I',raw,at)[0]
 return values

def seed(which,saved):
 values={}
 for index,(name,rva,kind,default) in enumerate(DATA):
  if '[' in kind:
   values[name]=[((index+1)*7+i+(11 if which=='snapshot' else 31))&255 for i in range(width(kind))]
  elif '*' in kind:
   # Alias root must restore to NULL to retain the genuine first natural helper.
   values[name]=0 if which=='snapshot' and name=='include_aliases' else saved+5200+index*4+(0 if which=='snapshot' else 400)
  else:values[name]=101+index*3+(0 if which=='snapshot' else 1000)
 return values

def seed_bss(saved):
 return {name:saved+5800+i*4 if '*' in kind else 301+i*3 for i,(name,rva,kind,default) in enumerate(BSS)}

def field_rows(rows):
 out=[];positions=DATA_OFFSETS if rows is DATA else BSS_OFFSETS
 for name,rva,kind,default in rows:
  record=dict(name='options.'+name,original_rva=rva,bytes=width(kind),native_field=native_field(name,rows),native_offset=positions[name],default=default,
   c_type=kind,default_proof='canonical C1 loaded pre-entry storage; parser/table own exact operands and semantic widths; no initialized gap credit')
  if rows is DATA:record.update(snapshot_original_offset=rva-0x5e000,snapshot_native_offset=positions[name])
  out.append(record)
 return out


def seed_metadata():
 return ''.join('%s %x %x %d %d\n'%(region,rva,width(kind),2 if '[' in kind else 1 if '*' in kind else 0,index)
  for region,rows in [('D',DATA),('B',BSS)]for index,(name,rva,kind,default) in enumerate(rows))
