CVPACK process-boundary reference

hello-debug-dll/before.dll and after.dll are one independently captured original
CVPACK transition. The reconstructed LINK computes its own entire prepack input;
only a complete match under the declared structural clock fields permits replay
of the captured response. Unknown commands or changed non-clock input fail.
These are test reference artifacts, not reconstruction implementation data.

The missing prepack state required one parent-coordinated original invocation.
The manifest pins original LINK/CVPACK, helper source/executable, capture events,
pre/post hashes, timestamp fields, checksum sentinel, command/status/streams and
separate elapsed times. The captured final outputs were verified against all
four existing immutable link-debug-dll references before this data was trusted.
Native regression never captures references or invokes Wine/original programs.

Implementation: scripts/native-processes.py. Rejection checks:
    python3 scripts/check-native-boundary.py
Final 18-case evidence: cases/native-all-validation.json.
