
SYMSCAN is a smart object-code grep utility.  It is indispensable for 
diagnosing link errors -- unresolved externals, duplicate symbols, 
segment and group overflows, etc.  SYMSCAN is also useful for 
learning about large bodies of unfamiliar code; it has the speed and 
convenience of grep, but provides better information because it 
parses .OBJ and .LIB files. It distinguishes symbol references from 
definitions and identifies exactly WHERE in each module the 
definition/reference occurs.  Locations are shown by function name or 
even source-line number if debug information is present. SYMSCAN 
shows true sizes for every type of program object.

Execute symscan.exe with no arguments to get usage instructions.  
Execute it with a -xhelp argument and redirect the output to a file 
to get full documentation.

Copyright (c) Objects Only, Inc. 1993.  All Rights Reserved.

    Objects Only, Inc.
    Suite 257
    11919 N. Jantzen Ave.
    Portland, OR  97217

    fax: (503) 735-0106
