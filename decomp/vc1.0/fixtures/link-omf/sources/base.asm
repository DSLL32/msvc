%pragma obj nodepend
segment _TEXT public align=4 use32 class=CODE
extern _omf_helper
extern _comdat_value
global _omf_value
_omf_value:
    call _omf_helper
    add eax,[_data_value]
    add eax,[_comdat_value]
    ret
segment _DATA public align=4 use32 class=DATA
global _data_value
_data_value: dd 7
group FLAT _TEXT _DATA
