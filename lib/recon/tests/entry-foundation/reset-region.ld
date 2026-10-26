/* Test-only selection of one contiguous writable native data+BSS span. */
SECTIONS
{
 .recon_reset_data : ALIGN(16)
 {
  recon_test_state_start = .;
  KEEP(*(.data.recon_reset))
  . = ALIGN(64);
 }
 .recon_reset_bss (NOLOAD) :
 {
  KEEP(*(.bss.recon_reset))
  . = ALIGN(32);
  recon_test_state_end = .;
 }
}
INSERT AFTER .data;
