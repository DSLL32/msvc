extern int weak_name(void),lazy_name(void),alias_name(void);
extern int absolute_value;
int default_target(void) { return 7; }
int entry(void)
{
    return weak_name()+lazy_name()+alias_name()+(int)&absolute_value;
}
