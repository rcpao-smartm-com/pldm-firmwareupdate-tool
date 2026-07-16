SUBDIRS = main/PLDM_PKG_HEADER
SUBDIRS_decode = main/decode

ifeq ($(OS),Windows_NT)
EXEEXT = .exe
else
EXEEXT =
endif

ENCODE = pldm_encode$(EXEEXT)
DECODE = parse_pldm$(EXEEXT)

all:
	(cd $(SUBDIRS) && make);
	(cd main && make);
decode:
	(cd $(SUBDIRS_decode) && make);
	(cd main/decode && ./$(DECODE) ../pldm1.0-img_0.bin);
encode:
	(cd main && ./$(ENCODE) img_0.bin 1.0.0 1.0 && make mv_file);
clean:
	(cd $(SUBDIRS) && make clean);
	(cd main && make clean);
	(cd main/decode && make clean);
