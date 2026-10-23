# Early pass option scan

The historical logical name initialize_intermediate_files now owns79 hot bytes
38d07..38d56 plus84 cold bytes720a8..720fc. Actual behavior scans argv[1..NULL],
carrying the integer state returned by process_early_option. It then reads
MSC_CMD_FLAGS and tokenizes it into a514-byte automatic buffer, restarting state0.
Both helpers have actual named cdecl stubs with all formals: argument/state and
source/output/capacity. No lexer/category data is inferred before that consumer.

Natural first parser entry is argv[1] ("-il"), state0; original caller38d28 and
native actual CALL ownership are proven separately. Entry/formal/raw-stack
observations anchor the scanner argv and first parser. Complete original BSS and
both8520-byte original initialized/saved images remain unchanged; every native
modeled field/storage map/caller state is checked. There are no new data fields.
Original capture5.48s/native GDB4.04s;17 new affected controls, unchanged allocation
and reset controls reused explicitly. See early-options-trial.json.

The source name still follows the preexisting logical frontier for this exact
capture. A descriptive source-name projection can be introduced at the next build
boundary, retaining immutable historical identifiers/evidence without relabeling.
