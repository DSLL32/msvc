/* Genuine archive inputs. Compile this header as C with VC5 /c /O2 /G5 /Gy
 * /Z7 /Zl and exactly one LINK_ARCHIVE_LEFT/RIGHT definition. The captured
 * object bytes, rather than a Windows compiler, are fixture inputs. */
#ifdef LINK_ARCHIVE_LEFT
extern int archive_bias;
extern int __cdecl archive_scale(int);
int __cdecl archive_sum(int left,int right)
{
    return archive_scale(left+right)+archive_bias;
}
const char archive_label[]="VC5 object archive";
#endif
#ifdef LINK_ARCHIVE_RIGHT
int archive_bias=7;
int __cdecl archive_scale(int value)
{
    return value*3;
}
#endif
