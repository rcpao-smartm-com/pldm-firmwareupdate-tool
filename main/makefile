ifeq ($(OS),Windows_NT)
EXEEXT = .exe
else
EXEEXT =
endif

ENCODE = pldm_encode$(EXEEXT)

$(ENCODE): PLDM_PKG_HEADER/libpldm.a
	gcc main.c PLDM_PKG_HEADER/libpldm.a -lz -ljson-c -o $(ENCODE)
mv_file: pldm_update_header.bin
	cp pldm_update_header.bin decode
clean:
	rm -f pldm_encode pldm_encode.exe pldm_update_header.bin pldm_update_pkg.bin
