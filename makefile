SUBDIRS = main/PLDM_PKG_HEADER
SUBDIRS_decode = main/decode

ifeq ($(OS),Windows_NT)
EXEEXT = .exe
else
EXEEXT =
endif

ENCODE = pldm_encode$(EXEEXT)
DECODE = parse_pldm$(EXEEXT)
SPECS = 1.0 1.1 1.2 1.3

all:
	(cd $(SUBDIRS) && make);
	(cd main && make);
decode:
	(cd $(SUBDIRS) && make);
	(cd main && make);
	(cd $(SUBDIRS_decode) && make);
	@for v in $(SPECS); do \
		echo "===== encode $$v ====="; \
		(cd main && ./$(ENCODE) img_0.bin 1.0.0 $$v) || exit 1; \
		echo "===== decode $$v ====="; \
		(cd main/decode && ./$(DECODE) ../pldm$$v-img_0.bin) || exit 1; \
	done
	@echo "decode test OK for $(SPECS)"
encode:
	(cd main && ./$(ENCODE) img_0.bin 1.0.0 1.0 && make mv_file);
clean:
	(cd $(SUBDIRS) && make clean);
	(cd main && make clean);
	(cd main/decode && make clean);
