# Compiler and flags
CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11

# Objects and target library
OBJECTS = pldm_fw_update_parse.o main.o
LIBPARSE = libparse.a

# Default build target
all: parse_pldm.exe

# Rule to create the final executable
parse_pldm.exe: $(LIBPARSE) main.c
	$(CC) $(CFLAGS) main.c $(LIBPARSE) -lz -ljson-c -o parse_pldm.exe

# Archive the object(s) into libparse.a
$(LIBPARSE): $(OBJECTS)
	ar -rc $(LIBPARSE) $(OBJECTS)

# Compile pldm_fw_update_parse.c into an object
pldm_fw_update_parse.o: pldm_fw_update_parse.c pldm_fw_update_parse.h
	$(CC) $(CFLAGS) -c pldm_fw_update_parse.c -o pldm_fw_update_parse.o

# Clean up build outputs
clean:
	rm -f $(LIBPARSE) $(OBJECTS) parse_pldm.exe