/* Original-compiled input payloads. Fixture execution never compiles them. */
#ifdef LINK_REPLACE_LEFT
extern int archive_bias;
extern int __cdecl archive_scale(int);
int __cdecl archive_sum(int left,int right)
{
    return archive_scale(left+right+1)+archive_bias;
}
const char archive_label[]="VC5 replacement object";
#endif
#ifdef LINK_REPLACE_ADDITION
extern int __cdecl archive_sum(int,int);
int __cdecl archive_mix(int value)
{
    return archive_sum(value,5)-value;
}
int archive_revision=2;
#endif
