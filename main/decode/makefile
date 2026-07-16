# Compiler and flags
CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11

ifeq ($(OS),Windows_NT)
EXEEXT = .exe
else
EXEEXT =
endif

DECODE = parse_pldm$(EXEEXT)

# Objects and target library
OBJECTS = pldm_fw_update_parse.o main.o
LIBPARSE = libparse.a

# Default build target
all: $(DECODE)

# Rule to create the final executable
$(DECODE): $(LIBPARSE) main.c
	$(CC) $(CFLAGS) main.c $(LIBPARSE) -lz -ljson-c -o $(DECODE)

# Archive the object(s) into libparse.a
$(LIBPARSE): $(OBJECTS)
	ar -rc $(LIBPARSE) $(OBJECTS)

# Compile pldm_fw_update_parse.c into an object
pldm_fw_update_parse.o: pldm_fw_update_parse.c pldm_fw_update_parse.h
	$(CC) $(CFLAGS) -c pldm_fw_update_parse.c -o pldm_fw_update_parse.o

# Clean up build outputs
clean:
	rm -f $(LIBPARSE) $(OBJECTS) parse_pldm parse_pldm.exe
