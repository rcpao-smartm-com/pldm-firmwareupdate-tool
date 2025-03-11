# PLDM Firmware Update Parser

## 1. Overview

This project provides a command-line tool and accompanying static library to parse PLDM Firmware Update headers as defined in the DSP0267 specification. It processes the package header, device ID records, component image information, and related metadata (timestamps, descriptors, version strings, checksums, etc.). By analyzing these fields, you can verify the structure and contents of a PLDM firmware update file.

## 2. Key Components

- **`pldm_fw_update_parse.h`**  
  Declares data structures (e.g., `PLDMFirmwarePackageHeader`, `PLDMFirmwareDeviceIdRecord`, `PLDMComponentImageInfo`) and parsing helper function prototypes for endian-aware reads, printing string types, descriptor IDs, and component classifications.

- **`pldm_fw_update_parse.c`**  
  Implements the functions declared in `pldm_fw_update_parse.h`. Contains lookup tables for descriptor IDs and component classifications, and offers utilities such as byte-order extraction, reversed-hex printing, and string type printing.

- **`main.c`**  
  Includes the `parseFile()` function, which demonstrates how to:
  1. Read the PLDM firmware header size from the binary file.
  2. Parse and display the overall package header, device records, component images, and final CRC32 checksum.
  3. Print useful information (e.g., version strings, timestamps, descriptors) in a human-readable format.

- **Makefile**  
  - Compiles the tool into a static library (`libparse.a`) and final executable (`parse_pldm.exe`).
  - Includes rules for building object files and linking them together.
  - Uses `-Wall`, `-Wextra`, and `-O2` flags for warnings and optimizations.
  - Defines a `clean` target to remove build artifacts.

## 3. Dependencies

- **C Compiler** supporting C11 (e.g., GCC)
- **zlib** (`-lz`)
- **json-c** (`-ljson-c`)
- Make tool (for building via the provided Makefile)

## 4. Build Instructions

1. Ensure that `zlib` and `json-c` are installed on your system.  
2. Open a terminal in the project directory.  
3. Run `make` to build the static library (`libparse.a`) and the final executable (`parse_pldm.exe`).

This will produce:
- `pldm_fw_update_parse.o`  
- `main.o`  
- `libparse.a`  
- `parse_pldm.exe`  

## 5. Usage

./parse_pldm.exe <firmware_file.bin>

1. Provide the path to your PLDM firmware file as the argument.  
2. The program reads the binary data, extracts header information, device ID records, and component image details, then prints them in a readable format.  
3. If no argument is supplied, it will prompt you to enter the file path manually.

## 6. Example

./parse_pldm.exe my_fw_file.bin

--------------------------------------------
PLDM Firmware Update: Full Parsing
--------------------------------------------
PackageHeaderIdentifier (UUID): ...
PackageHeaderFormatRevision: 0x01
PackageHeaderSize: ...
...
Firmware Device ID Record Count: ...
...

## 7. Cleaning Up
To remove generated object files, the static library, and the executable, run:

make clean
