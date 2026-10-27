#
# Tutor - Make File
#
BPDIR=\bp
UNITDIRS=$(BPDIR)\units
CC=$(BPDIR)\bin\bpc /m /V /U$(UNITDIRS) /CW

PGM=Tutor
OBJS = 	$(PGM).pas \
	Jtools.pas \

$(PGM).exe: $(OBJS) $(PGM).res
	$(CC) $(PGM)

$(PGM).res: $(PGM).rc 
     $(BPDIR)\bin\brc -r -I$(UNITDIRS) $(PGM).rc

