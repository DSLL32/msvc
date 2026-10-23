This case preserves the rich resource source and adds one trailing space inside
the quoted `/dCASE_FROM_COMMAND=7 ` argument. The parser replaces that space
with NUL; the original terminal NUL remains inside the requested packed argument
allocation. All three copied WCHARs are therefore owned and initialized.

The fresh original result is the same 3,672-byte resource file as the no-space
case. References and raw history for that earlier input remain unchanged; its
copy step reads two bytes beyond the requested object and is unsupported.

The copy trial preserves Windows SEH in source. Native execution is limited to
validated source extents and stops before an unsupported copy. Parent/filter ABI
roles are checked separately from the shared graph auditor's RET-set comparison.
