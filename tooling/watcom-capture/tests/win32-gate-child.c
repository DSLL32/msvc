/* Authored no-CRT Win32 capture probe, never reconstructed target source.
   Link probe_entry_ directly as AddressOfEntryPoint; compile -s -zl to omit
   stack-check helpers/default CRT libraries. The observer terminates the child.
   No setup prefix or external API call is needed after its PE entry. */
volatile unsigned probe_marker;

void probe_leaf(void)
{
    probe_marker=1;
}

void probe_entry(void)
{
    probe_leaf();
    probe_marker+=2;
    for(;;) {}
}

/* A public next-boundary anchor, never called or executed by the probe. */
void probe_end(void)
{
}
